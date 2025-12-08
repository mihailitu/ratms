#include "server.h"
#include "optimization_controller.h"
#include "../utils/logger.h"
#include "../optimization/metrics.h"
#include "../core/defs.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>

using json = nlohmann::json;
using namespace ratms;

namespace ratms {
namespace api {

// Helper functions for consistent API responses
static void sendError(httplib::Response& res, int status, const std::string& message) {
    json error = {
        {"success", false},
        {"error", message}
    };
    res.status = status;
    res.set_content(error.dump(2), "application/json");
}

static void sendSuccess(httplib::Response& res, const json& data) {
    json response = {
        {"success", true},
        {"data", data}
    };
    res.set_content(response.dump(2), "application/json");
}

Server::Server(int port) : port_(port) {
    LOG_INFO(LogComponent::API, "API Server initialized on port {}", port_);
}

Server::~Server() {
    // Stop simulation thread first
    if (simulation_running_) {
        simulation_should_stop_ = true;
        if (simulation_thread_ && simulation_thread_->joinable()) {
            simulation_thread_->join();
        }
    }

    stop();
}

void Server::start() {
    if (running_) {
        LOG_WARN(LogComponent::API, "Server already running");
        return;
    }

    setupMiddleware();
    setupRoutes();

    running_ = true;

    // Start server in separate thread
    server_thread_ = std::make_unique<std::thread>([this]() {
        LOG_INFO(LogComponent::API, "Starting HTTP server on http://localhost:{}", port_);
        http_server_.listen("0.0.0.0", port_);
    });

    LOG_INFO(LogComponent::API, "API Server started successfully");
}

void Server::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    http_server_.stop();

    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }

    LOG_INFO(LogComponent::API, "API Server stopped");
}

void Server::setSimulator(std::shared_ptr<simulator::Simulator> sim) {
    std::lock_guard<std::mutex> lock(sim_mutex_);
    simulator_ = sim;
    LOG_INFO(LogComponent::API, "Simulator instance attached to API server");
}

void Server::setDatabase(std::shared_ptr<data::DatabaseManager> db) {
    database_ = db;
    LOG_INFO(LogComponent::Database, "Database manager attached to API server");

    // Initialize optimization controller
    if (database_) {
        optimization_controller_ = std::make_unique<OptimizationController>(database_);
        LOG_INFO(LogComponent::Optimization, "Optimization controller initialized");
    }
}

void Server::setupMiddleware() {
    // CORS middleware for web dashboard
    http_server_.set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        // Handle preflight requests
        if (req.method == "OPTIONS") {
            res.status = 200;
            return httplib::Server::HandlerResponse::Handled;
        }

        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Logging middleware - log warnings for errors, debug for success
    http_server_.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        if (res.status >= 400) {
            LOG_WARN(LogComponent::API, "HTTP {} {} -> {}", req.method, req.path, res.status);
        } else {
            LOG_DEBUG(LogComponent::API, "HTTP {} {} -> {}", req.method, req.path, res.status);
        }
    });
}

void Server::setupRoutes() {
    // Health check endpoint
    http_server_.Get("/api/health", [this](const httplib::Request& req, httplib::Response& res) {
        handleHealth(req, res);
    });

    // Simulation control endpoints
    http_server_.Post("/api/simulation/start", [this](const httplib::Request& req, httplib::Response& res) {
        handleSimulationStart(req, res);
    });

    http_server_.Post("/api/simulation/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleSimulationStop(req, res);
    });

    http_server_.Get("/api/simulation/status", [this](const httplib::Request& req, httplib::Response& res) {
        handleSimulationStatus(req, res);
    });

    http_server_.Get("/api/simulation/stream", [this](const httplib::Request& req, httplib::Response& res) {
        handleSimulationStream(req, res);
    });

    http_server_.Get("/api/simulation/roads", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetRoads(req, res);
    });

    // Database query endpoints
    http_server_.Get("/api/simulations", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetSimulations(req, res);
    });

    http_server_.Get(R"(/api/simulations/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetSimulation(req, res);
    });

    http_server_.Get(R"(/api/simulations/(\d+)/metrics)", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetMetrics(req, res);
    });

    http_server_.Get("/api/networks", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetNetworks(req, res);
    });

    // Analytics endpoints
    http_server_.Get(R"(/api/analytics/simulations/(\d+)/statistics)", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetStatistics(req, res);
    });

    http_server_.Get(R"(/api/analytics/simulations/(\d+)/statistics/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetStatisticsByType(req, res);
    });

    http_server_.Post("/api/analytics/compare", [this](const httplib::Request& req, httplib::Response& res) {
        handleCompareSimulations(req, res);
    });

    http_server_.Get(R"(/api/analytics/simulations/(\d+)/export)", [this](const httplib::Request& req, httplib::Response& res) {
        handleExportMetrics(req, res);
    });

    http_server_.Get("/api/analytics/metric-types", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetMetricTypes(req, res);
    });

    // Register optimization routes if controller is initialized
    if (optimization_controller_) {
        optimization_controller_->registerRoutes(http_server_);
        LOG_INFO(LogComponent::API, "Optimization routes registered");
    }

    LOG_INFO(LogComponent::API, "API routes configured: {} endpoints", 15);
}

void Server::handleHealth(const httplib::Request& req, httplib::Response& res) {
    json response = {
        {"status", "healthy"},
        {"service", "RATMS API Server"},
        {"version", "0.1.0"},
        {"timestamp", std::time(nullptr)}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSimulationStart(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (simulation_running_) {
        LOG_WARN(LogComponent::API, "Start request rejected: simulation already running");
        sendError(res, 400, "Simulation already running");
        return;
    }

    if (!simulator_) {
        LOG_ERROR(LogComponent::API, "Start request failed: simulator not initialized");
        sendError(res, 500, "Simulator not initialized");
        return;
    }

    // Create database record if database is available
    if (database_ && database_->isConnected()) {
        int sim_id = database_->createSimulation(
            "API Simulation",
            "Simulation started via REST API",
            1, // Default network ID
            "{}"
        );
        if (sim_id > 0) {
            current_simulation_id_ = sim_id;
            database_->updateSimulationStatus(sim_id, "running");
            LOG_INFO(LogComponent::Database, "Simulation record created with ID: {}", sim_id);
        }
    }

    // Reset simulation state
    simulation_should_stop_ = false;
    simulation_steps_ = 0;
    simulation_time_ = 0.0;
    simulation_running_ = true;

    // Start simulation in background thread
    simulation_thread_ = std::make_unique<std::thread>(&Server::runSimulationLoop, this);

    LOG_INFO(LogComponent::Simulation, "Simulation started via API");

    json response = {
        {"message", "Simulation started successfully"},
        {"status", "running"},
        {"simulation_id", current_simulation_id_.load()},
        {"timestamp", std::time(nullptr)}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSimulationStop(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    {
        std::lock_guard<std::mutex> lock(sim_mutex_);

        if (!simulation_running_) {
            LOG_WARN(LogComponent::API, "Stop request rejected: simulation not running");
            sendError(res, 400, "Simulation not running");
            return;
        }

        // Signal simulation thread to stop
        simulation_should_stop_ = true;
        LOG_DEBUG(LogComponent::Simulation, "Sent stop signal to simulation thread");
    }

    // Wait for simulation thread to complete (outside the lock to avoid deadlock)
    if (simulation_thread_ && simulation_thread_->joinable()) {
        simulation_thread_->join();
        simulation_thread_.reset();
        LOG_DEBUG(LogComponent::Simulation, "Simulation thread joined");
    }

    {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        simulation_running_ = false;

        // Complete database record if database is available
        if (database_ && database_->isConnected() && current_simulation_id_ > 0) {
            long end_time = std::time(nullptr);
            database_->completeSimulation(current_simulation_id_, end_time, simulation_time_.load());
            LOG_INFO(LogComponent::Database, "Simulation record {} completed", current_simulation_id_.load());
            current_simulation_id_ = -1;
        }
    }

    LOG_INFO(LogComponent::Simulation, "Simulation stopped via API");

    json response = {
        {"message", "Simulation stopped successfully"},
        {"status", "stopped"},
        {"steps", simulation_steps_.load()},
        {"time", simulation_time_.load()},
        {"timestamp", std::time(nullptr)}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSimulationStatus(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(sim_mutex_);

    json response = {
        {"status", simulation_running_ ? "running" : "stopped"},
        {"simulator_initialized", simulator_ != nullptr},
        {"server_running", running_.load()},
        {"timestamp", std::time(nullptr)}
    };

    if (simulator_) {
        response["road_count"] = simulator_->cityMap.size();
    }

    if (simulation_running_) {
        response["simulation_steps"] = simulation_steps_.load();
        response["simulation_time"] = simulation_time_.load();
    }

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleGetRoads(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (!simulator_) {
        sendError(res, 500, "Simulator not initialized");
        return;
    }

    json roads = json::array();

    for (const auto& roadPair : simulator_->cityMap) {
        const auto& road = roadPair.second;
        auto startGeo = road.getStartPosGeo();
        auto endGeo = road.getEndPosGeo();

        roads.push_back({
            {"id", road.getId()},
            {"length", road.getLength()},
            {"maxSpeed", road.getMaxSpeed()},
            {"lanes", road.getLanesNo()},
            {"startLat", startGeo.second},
            {"startLon", startGeo.first},
            {"endLat", endGeo.second},
            {"endLon", endGeo.first}
        });
    }

    json response = {
        {"roads", roads},
        {"count", roads.size()}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSimulationStream(const httplib::Request& req, httplib::Response& res) {
    // Set up Server-Sent Events (SSE)
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");
    // CORS header already set by middleware

    LOG_INFO(LogComponent::SSE, "Client connected to simulation stream");

    res.set_chunked_content_provider(
        "text/event-stream",
        [this](size_t offset, httplib::DataSink &sink) {
            // Check if simulation is running
            if (!simulation_running_) {
                // Send a message that simulation is not running
                std::string msg = "event: status\ndata: {\"status\":\"stopped\"}\n\n";
                sink.write(msg.c_str(), msg.size());
                return false;  // Close connection
            }

            // Wait for new snapshot
            if (has_new_snapshot_) {
                SimulationSnapshot snapshot;
                {
                    std::lock_guard<std::mutex> lock(snapshot_mutex_);
                    snapshot = latest_snapshot_;
                    has_new_snapshot_ = false;
                }

                // Convert snapshot to JSON
                json data = {
                    {"step", snapshot.step},
                    {"time", snapshot.time},
                    {"vehicles", json::array()},
                    {"trafficLights", json::array()}
                };

                for (const auto& v : snapshot.vehicles) {
                    data["vehicles"].push_back({
                        {"id", v.id},
                        {"roadId", v.roadId},
                        {"lane", v.lane},
                        {"position", v.position},
                        {"velocity", v.velocity},
                        {"acceleration", v.acceleration}
                    });
                }

                for (const auto& tl : snapshot.trafficLights) {
                    data["trafficLights"].push_back({
                        {"roadId", tl.roadId},
                        {"lane", tl.lane},
                        {"state", std::string(1, tl.state)},
                        {"lat", tl.lat},
                        {"lon", tl.lon}
                    });
                }

                // Send as SSE
                std::string msg = "event: update\ndata: " + data.dump() + "\n\n";
                if (!sink.write(msg.c_str(), msg.size())) {
                    LOG_INFO(LogComponent::SSE, "Client disconnected from simulation stream");
                    return false;
                }
            }

            // Small sleep to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return true;  // Continue streaming
        },
        [](bool success) {
            // Cleanup callback
        }
    );

    LOG_DEBUG(LogComponent::SSE, "Simulation stream handler completed");
}

// Database query handlers
void Server::handleGetSimulations(const httplib::Request& req, httplib::Response& res) {
    if (!database_ || !database_->isConnected()) {
        json response = {
            {"error", "Database not available"},
            {"message", "Database connection not initialized"}
        };
        res.set_content(response.dump(2), "application/json");
        res.status = 503;
        return;
    }

    auto simulations = database_->getAllSimulations();
    json response = json::array();

    for (const auto& sim : simulations) {
        response.push_back({
            {"id", sim.id},
            {"name", sim.name},
            {"description", sim.description},
            {"network_id", sim.network_id},
            {"status", sim.status},
            {"start_time", sim.start_time},
            {"end_time", sim.end_time},
            {"duration_seconds", sim.duration_seconds}
        });
    }

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleGetSimulation(const httplib::Request& req, httplib::Response& res) {
    if (!database_ || !database_->isConnected()) {
        sendError(res, 503, "Database not available");
        return;
    }

    int sim_id = std::stoi(req.matches[1]);
    auto sim = database_->getSimulation(sim_id);

    if (sim.id == 0) {
        sendError(res, 404, "Simulation not found");
        return;
    }

    json response = {
        {"id", sim.id},
        {"name", sim.name},
        {"description", sim.description},
        {"network_id", sim.network_id},
        {"status", sim.status},
        {"start_time", sim.start_time},
        {"end_time", sim.end_time},
        {"duration_seconds", sim.duration_seconds},
        {"config", sim.config_json}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleGetMetrics(const httplib::Request& req, httplib::Response& res) {
    if (!database_ || !database_->isConnected()) {
        sendError(res, 503, "Database not available");
        return;
    }

    int sim_id = std::stoi(req.matches[1]);
    auto metrics = database_->getMetrics(sim_id);

    json response = json::array();
    for (const auto& metric : metrics) {
        response.push_back({
            {"id", metric.id},
            {"simulation_id", metric.simulation_id},
            {"timestamp", metric.timestamp},
            {"metric_type", metric.metric_type},
            {"road_id", metric.road_id},
            {"value", metric.value},
            {"unit", metric.unit}
        });
    }

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleGetNetworks(const httplib::Request& req, httplib::Response& res) {
    if (!database_ || !database_->isConnected()) {
        json response = {
            {"error", "Database not available"}
        };
        res.set_content(response.dump(2), "application/json");
        res.status = 503;
        return;
    }

    auto networks = database_->getAllNetworks();
    json response = json::array();

    for (const auto& net : networks) {
        response.push_back({
            {"id", net.id},
            {"name", net.name},
            {"description", net.description},
            {"road_count", net.road_count},
            {"intersection_count", net.intersection_count}
        });
    }

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::runSimulationLoop() {
    LOG_INFO(LogComponent::Simulation, "Simulation loop started");

    const double dt = 0.1;  // Time step
    const int maxSteps = 10000;  // Maximum steps
    const int metricsInterval = 10;  // Collect metrics every 10 steps
    const int dbWriteInterval = 100;  // Write to DB every 100 steps

    simulator::MetricsCollector metricsCollector;

    try {
        while (!simulation_should_stop_ && simulation_steps_ < maxSteps) {
            int currentStep = simulation_steps_;

            // PHASE 1: Update all roads and collect pending transitions
            std::vector<simulator::RoadTransition> pendingTransitions;
            {
                TIMED_SCOPE(LogComponent::Simulation, "simulation_step");
                std::lock_guard<std::mutex> lock(sim_mutex_);
                if (!simulator_) {
                    LOG_ERROR(LogComponent::Simulation, "Simulator became null during simulation");
                    break;
                }

                for (auto& roadPair : simulator_->cityMap) {
                    roadPair.second.update(dt, simulator_->cityMap, pendingTransitions);
                }
            }

            // PHASE 2: Execute road transitions
            {
                std::lock_guard<std::mutex> lock(sim_mutex_);
                if (!simulator_) break;

                for (const auto& transition : pendingTransitions) {
                    simulator::Vehicle transitioningVehicle = std::get<0>(transition);
                    simulator::roadID destRoadID = std::get<1>(transition);
                    unsigned destLane = std::get<2>(transition);

                    auto destRoadIt = simulator_->cityMap.find(destRoadID);
                    if (destRoadIt != simulator_->cityMap.end()) {
                        transitioningVehicle.setPos(0.0);
                        destRoadIt->second.addVehicle(transitioningVehicle, destLane);
                    } else {
                        // Vehicle exited the network
                        metricsCollector.getMetricsMutable().vehiclesExited += 1.0;
                    }
                }
            }

            // PHASE 3: Collect metrics periodically
            if (currentStep % metricsInterval == 0) {
                std::lock_guard<std::mutex> lock(sim_mutex_);
                if (simulator_) {
                    metricsCollector.collectMetrics(simulator_->cityMap, dt);
                }
            }

            // PHASE 3.5: Capture snapshot for streaming (every 5 steps for ~2 updates/sec)
            if (currentStep % 5 == 0) {
                captureSimulationSnapshot();
            }

            // PHASE 4: Write metrics to database periodically
            if (currentStep % dbWriteInterval == 0 && currentStep > 0) {
                std::lock_guard<std::mutex> lock(sim_mutex_);
                if (database_ && database_->isConnected() && current_simulation_id_ > 0) {
                    // Get current metrics
                    simulator::SimulationMetrics metrics = metricsCollector.getMetrics();
                    double avgQueueLength = metrics.sampleCount > 0
                        ? metrics.averageQueueLength / metrics.sampleCount
                        : 0.0;
                    double avgSpeed = metrics.sampleCount > 0
                        ? metrics.averageSpeed / metrics.sampleCount
                        : 0.0;

                    // Save metrics to database
                    long timestamp = std::time(nullptr);
                    database_->insertMetric(current_simulation_id_, timestamp, "avg_queue_length", 0, avgQueueLength, "vehicles");
                    database_->insertMetric(current_simulation_id_, timestamp, "avg_speed", 0, avgSpeed, "m/s");
                    database_->insertMetric(current_simulation_id_, timestamp, "vehicles_exited", 0, metrics.vehiclesExited, "count");
                    database_->insertMetric(current_simulation_id_, timestamp, "max_queue_length", 0, metrics.maxQueueLength, "vehicles");

                    LOG_DEBUG(LogComponent::Database, "Saved metrics at step {}: avg_queue={:.2f}, avg_speed={:.2f}, exited={:.0f}",
                             currentStep, avgQueueLength, avgSpeed, metrics.vehiclesExited);
                }
            }

            // Update counters
            simulation_steps_++;
            simulation_time_ = simulation_time_.load() + dt;

            // Small sleep to avoid CPU spinning (optional, can be removed for max speed)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Final metrics write
        {
            std::lock_guard<std::mutex> lock(sim_mutex_);
            if (database_ && database_->isConnected() && current_simulation_id_ > 0) {
                simulator::SimulationMetrics metrics = metricsCollector.getMetrics();
                double avgQueueLength = metrics.sampleCount > 0
                    ? metrics.averageQueueLength / metrics.sampleCount
                    : 0.0;
                double avgSpeed = metrics.sampleCount > 0
                    ? metrics.averageSpeed / metrics.sampleCount
                    : 0.0;

                long timestamp = std::time(nullptr);
                database_->insertMetric(current_simulation_id_, timestamp, "final_avg_queue_length", 0, avgQueueLength, "vehicles");
                database_->insertMetric(current_simulation_id_, timestamp, "final_avg_speed", 0, avgSpeed, "m/s");
                database_->insertMetric(current_simulation_id_, timestamp, "final_vehicles_exited", 0, metrics.vehiclesExited, "count");

                LOG_INFO(LogComponent::Database, "Final metrics saved: avg_queue={:.2f}, avg_speed={:.2f}, exited={:.0f}",
                         avgQueueLength, avgSpeed, metrics.vehiclesExited);
            }
        }

        if (simulation_should_stop_) {
            LOG_INFO(LogComponent::Simulation, "Simulation stopped by user request at step {}", simulation_steps_.load());
        } else {
            LOG_INFO(LogComponent::Simulation, "Simulation completed naturally at step {}", simulation_steps_.load());
        }

    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::Simulation, "Exception in simulation loop: {}", e.what());
    }

    LOG_INFO(LogComponent::Simulation, "Simulation loop ended: steps={}, time={:.2f}", simulation_steps_.load(), simulation_time_.load());
}

void Server::captureSimulationSnapshot() {
    SimulationSnapshot snapshot;
    snapshot.step = simulation_steps_;
    snapshot.time = simulation_time_;

    // Capture vehicle positions
    {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        if (!simulator_) return;

        for (const auto& roadPair : simulator_->cityMap) {
            const auto& road = roadPair.second;
            int roadId = road.getId();

            // Capture vehicles
            unsigned laneIdx = 0;
            for (const auto& lane : road.getVehicles()) {
                for (const auto& vehicle : lane) {
                    VehicleSnapshot vSnap;
                    vSnap.id = vehicle.getId();
                    vSnap.roadId = roadId;
                    vSnap.lane = laneIdx;
                    vSnap.position = vehicle.getPos();
                    vSnap.velocity = vehicle.getVelocity();
                    vSnap.acceleration = vehicle.getAcceleration();
                    snapshot.vehicles.push_back(vSnap);
                }
                ++laneIdx;
            }

            // Capture traffic light states
            auto trafficLights = road.getCurrentLightConfig();
            auto endPosGeo = road.getEndPosGeo();  // Traffic lights at end of road

            for (unsigned i = 0; i < trafficLights.size(); ++i) {
                TrafficLightSnapshot tlSnap;
                tlSnap.roadId = roadId;
                tlSnap.lane = i;
                tlSnap.state = trafficLights[i];

                // Position traffic light at end of road with lane offset
                tlSnap.lon = endPosGeo.first;
                tlSnap.lat = endPosGeo.second;

                // Apply lane offset perpendicular to road direction
                // Use same offset as vehicles (0.00001° per lane)
                if (i > 0) {
                    tlSnap.lat += 0.00001 * i;
                }

                snapshot.trafficLights.push_back(tlSnap);
            }
        }
    }

    // Store snapshot
    {
        std::lock_guard<std::mutex> lock(snapshot_mutex_);
        latest_snapshot_ = std::move(snapshot);
        has_new_snapshot_ = true;
    }
}

// Analytics handlers
void Server::handleGetStatistics(const httplib::Request& req, httplib::Response& res) {
    try {
        int sim_id = std::stoi(req.matches[1]);

        if (!database_) {
            sendError(res, 500, "Database not initialized");
            return;
        }

        // Get simulation info
        auto sim_record = database_->getSimulation(sim_id);
        if (sim_record.id != sim_id) {
            sendError(res, 404, "Simulation not found");
            return;
        }

        // Get all statistics
        auto all_stats = database_->getAllMetricStatistics(sim_id);

        // Format response
        json stats_json = json::object();
        for (const auto& [metric_type, stats] : all_stats) {
            stats_json[metric_type] = {
                {"metric_type", stats.metric_type},
                {"min_value", stats.min_value},
                {"max_value", stats.max_value},
                {"mean_value", stats.mean_value},
                {"median_value", stats.median_value},
                {"stddev_value", stats.stddev_value},
                {"p25_value", stats.p25_value},
                {"p75_value", stats.p75_value},
                {"p95_value", stats.p95_value},
                {"sample_count", stats.sample_count}
            };
        }

        json response = {
            {"simulation_id", sim_id},
            {"simulation_name", sim_record.name},
            {"statistics", stats_json}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, e.what());
    }
}

void Server::handleGetStatisticsByType(const httplib::Request& req, httplib::Response& res) {
    try {
        int sim_id = std::stoi(req.matches[1]);
        std::string metric_type = req.matches[2];

        if (!database_) {
            sendError(res, 500, "Database not initialized");
            return;
        }

        // Get simulation info
        auto sim_record = database_->getSimulation(sim_id);
        if (sim_record.id != sim_id) {
            sendError(res, 404, "Simulation not found");
            return;
        }

        // Get statistics for specific metric type
        auto stats = database_->getMetricStatistics(sim_id, metric_type);

        json response = {
            {"simulation_id", sim_id},
            {"simulation_name", sim_record.name},
            {"metric_type", stats.metric_type},
            {"min_value", stats.min_value},
            {"max_value", stats.max_value},
            {"mean_value", stats.mean_value},
            {"median_value", stats.median_value},
            {"stddev_value", stats.stddev_value},
            {"p25_value", stats.p25_value},
            {"p75_value", stats.p75_value},
            {"p95_value", stats.p95_value},
            {"sample_count", stats.sample_count}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, e.what());
    }
}

void Server::handleCompareSimulations(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!database_) {
            sendError(res, 500, "Database not initialized");
            return;
        }

        // Parse request body
        json request_body = json::parse(req.body);
        std::vector<int> simulation_ids = request_body["simulation_ids"];
        std::string metric_type = request_body["metric_type"];

        if (simulation_ids.empty()) {
            sendError(res, 400, "No simulation IDs provided");
            return;
        }

        // Get comparative metrics
        auto comparative_data = database_->getComparativeMetrics(simulation_ids, metric_type);

        // Format response
        json simulations_json = json::array();
        for (const auto& comp : comparative_data) {
            json metrics_json = json::array();
            for (const auto& metric : comp.metrics) {
                metrics_json.push_back({
                    {"timestamp", metric.timestamp},
                    {"value", metric.value}
                });
            }

            simulations_json.push_back({
                {"simulation_id", comp.simulation_id},
                {"simulation_name", comp.simulation_name},
                {"metrics", metrics_json}
            });
        }

        json response = {
            {"metric_type", metric_type},
            {"simulations", simulations_json}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, e.what());
    }
}

void Server::handleExportMetrics(const httplib::Request& req, httplib::Response& res) {
    try {
        int sim_id = std::stoi(req.matches[1]);

        if (!database_) {
            sendError(res, 500, "Database not initialized");
            return;
        }

        // Check if metric_type filter is provided
        std::string metric_type = "";
        if (req.has_param("metric_type")) {
            metric_type = req.get_param_value("metric_type");
        }

        // Get metrics
        std::vector<data::DatabaseManager::MetricRecord> metrics;
        if (!metric_type.empty()) {
            metrics = database_->getMetricsByType(sim_id, metric_type);
        } else {
            metrics = database_->getMetrics(sim_id);
        }

        // Build CSV content
        std::ostringstream csv;
        csv << "timestamp,metric_type,value,road_id,unit\n";

        for (const auto& metric : metrics) {
            csv << metric.timestamp << ","
                << metric.metric_type << ","
                << metric.value << ","
                << metric.road_id << ","
                << metric.unit << "\n";
        }

        // Set response headers for CSV download
        res.set_header("Content-Type", "text/csv");
        res.set_header("Content-Disposition",
                      "attachment; filename=\"simulation_" + std::to_string(sim_id) + "_metrics.csv\"");
        res.set_content(csv.str(), "text/csv");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, e.what());
    }
}

void Server::handleGetMetricTypes(const httplib::Request& req, httplib::Response& res) {
    // Return list of common metric types
    json response = {
        {"metric_types", {
            "avg_queue_length",
            "avg_speed",
            "vehicles_exited",
            "max_queue_length"
        }}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

} // namespace api
} // namespace ratms
