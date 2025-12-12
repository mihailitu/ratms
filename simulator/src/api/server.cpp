#include "server.h"
#include "optimization_controller.h"
#include "traffic_data_controller.h"
#include "continuous_optimization_controller.h"
#include "prediction_controller.h"
#include "traffic_profile_service.h"
#include "../prediction/traffic_predictor.h"
#include "../utils/logger.h"
#include "../optimization/metrics.h"
#include "../core/defs.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <random>

// OpenMP support with fallback for non-OpenMP builds
#ifdef _OPENMP
    #include <omp.h>
#else
    inline int omp_get_max_threads() { return 1; }
    inline int omp_get_thread_num() { return 0; }
    inline void omp_set_num_threads(int) {}
#endif

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

Server::Server(int port) : port_(port), server_start_time_(std::chrono::steady_clock::now()) {
    LOG_INFO(LogComponent::API, "API Server initialized on port {}", port_);
}

Server::~Server() {
    // Stop traffic feed first (if running)
    if (traffic_feed_ && traffic_feed_->isRunning()) {
        traffic_feed_->stop();
    }

    // Stop simulation thread
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

        // Initialize traffic pattern storage
        pattern_storage_ = std::make_shared<data::TrafficPatternStorage>(database_);
        last_snapshot_time_ = std::chrono::steady_clock::now();
        LOG_INFO(LogComponent::Database, "Traffic pattern storage initialized");
    }

    // Initialize traffic data controller (requires both database and simulator)
    if (database_ && simulator_) {
        traffic_data_controller_ = std::make_unique<TrafficDataController>(database_, simulator_, sim_mutex_);
        LOG_INFO(LogComponent::API, "Traffic data controller initialized");

        // Initialize continuous optimization controller
        continuous_optimization_controller_ = std::make_unique<ContinuousOptimizationController>(
            database_, simulator_, sim_mutex_);
        LOG_INFO(LogComponent::Optimization, "Continuous optimization controller initialized");

        // Initialize prediction controller (requires pattern storage and simulator)
        if (pattern_storage_) {
            auto predictor = std::make_shared<prediction::TrafficPredictor>(
                pattern_storage_, simulator_, sim_mutex_);
            prediction_controller_ = std::make_unique<PredictionController>(predictor);
            LOG_INFO(LogComponent::API, "Prediction controller initialized");

            // Connect predictor to continuous optimization controller for predictive mode
            if (continuous_optimization_controller_) {
                continuous_optimization_controller_->setPredictor(predictor);
                LOG_INFO(LogComponent::Optimization,
                         "Connected predictor to continuous optimization controller");
            }
        }

        // Initialize traffic profile service
        profile_service_ = std::make_unique<TrafficProfileService>(database_, simulator_, sim_mutex_);
        LOG_INFO(LogComponent::API, "Traffic profile service initialized");

        // Initialize travel time collector
        travel_time_collector_ = std::make_shared<metrics::TravelTimeCollector>(database_);
        LOG_INFO(LogComponent::API, "Travel time collector initialized");

        // Initialize density management (requires database, simulator, and pattern_storage)
        initializeDensityManagement();
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

    // Traffic light control endpoints
    http_server_.Get("/api/traffic-lights", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetTrafficLights(req, res);
    });

    http_server_.Post("/api/traffic-lights", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetTrafficLights(req, res);
    });

    // Spawn rate control endpoints
    http_server_.Get("/api/spawn-rates", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetSpawnRates(req, res);
    });

    http_server_.Post("/api/spawn-rates", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetSpawnRates(req, res);
    });

    // Continuous simulation mode endpoints
    http_server_.Post("/api/simulation/pause", [this](const httplib::Request& req, httplib::Response& res) {
        handleSimulationPause(req, res);
    });

    http_server_.Post("/api/simulation/resume", [this](const httplib::Request& req, httplib::Response& res) {
        handleSimulationResume(req, res);
    });

    http_server_.Get("/api/simulation/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetSimulationConfig(req, res);
    });

    http_server_.Post("/api/simulation/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetSimulationConfig(req, res);
    });

    http_server_.Post("/api/simulation/continuous", [this](const httplib::Request& req, httplib::Response& res) {
        handleSimulationStartContinuous(req, res);
    });

    // Traffic pattern endpoints
    http_server_.Get("/api/patterns", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetPatterns(req, res);
    });

    http_server_.Get("/api/snapshots", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetSnapshots(req, res);
    });

    http_server_.Post("/api/patterns/aggregate", [this](const httplib::Request& req, httplib::Response& res) {
        handleAggregatePatterns(req, res);
    });

    http_server_.Post("/api/patterns/prune", [this](const httplib::Request& req, httplib::Response& res) {
        handlePruneSnapshots(req, res);
    });

    // Traffic profile endpoints
    http_server_.Get("/api/profiles", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetProfiles(req, res);
    });

    http_server_.Get(R"(/api/profiles/([^/]+)/export)", [this](const httplib::Request& req, httplib::Response& res) {
        handleExportProfile(req, res);
    });

    http_server_.Post(R"(/api/profiles/([^/]+)/apply)", [this](const httplib::Request& req, httplib::Response& res) {
        handleApplyProfile(req, res);
    });

    http_server_.Get(R"(/api/profiles/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetProfile(req, res);
    });

    http_server_.Post("/api/profiles", [this](const httplib::Request& req, httplib::Response& res) {
        handleCreateProfile(req, res);
    });

    http_server_.Put(R"(/api/profiles/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleUpdateProfile(req, res);
    });

    http_server_.Delete(R"(/api/profiles/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleDeleteProfile(req, res);
    });

    http_server_.Post("/api/profiles/capture", [this](const httplib::Request& req, httplib::Response& res) {
        handleCaptureProfile(req, res);
    });

    http_server_.Post("/api/profiles/import", [this](const httplib::Request& req, httplib::Response& res) {
        handleImportProfile(req, res);
    });

    // Travel time endpoints
    http_server_.Get("/api/travel-time/od-pairs", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetODPairs(req, res);
    });

    http_server_.Post("/api/travel-time/od-pairs", [this](const httplib::Request& req, httplib::Response& res) {
        handleCreateODPair(req, res);
    });

    http_server_.Delete(R"(/api/travel-time/od-pairs/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleDeleteODPair(req, res);
    });

    http_server_.Get("/api/travel-time/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetTravelTimeStats(req, res);
    });

    http_server_.Get(R"(/api/travel-time/stats/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetODPairStats(req, res);
    });

    http_server_.Get(R"(/api/travel-time/samples/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetTravelTimeSamples(req, res);
    });

    http_server_.Get("/api/travel-time/tracked", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetTrackedVehicles(req, res);
    });

    // Density management routes
    http_server_.Get("/api/density-management/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetDensityConfig(req, res);
    });

    http_server_.Post("/api/density-management/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetDensityConfig(req, res);
    });

    http_server_.Get("/api/density-management/status", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetDensityStatus(req, res);
    });

    http_server_.Get("/api/density-management/feed-info", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetFeedInfo(req, res);
    });

    http_server_.Get("/api/feed-data/export", [this](const httplib::Request& req, httplib::Response& res) {
        handleExportFeedData(req, res);
    });

    // Register optimization routes if controller is initialized
    if (optimization_controller_) {
        optimization_controller_->registerRoutes(http_server_);
        LOG_INFO(LogComponent::API, "Optimization routes registered");
    }

    // Register traffic data routes if controller is initialized
    if (traffic_data_controller_) {
        traffic_data_controller_->registerRoutes(http_server_);
        LOG_INFO(LogComponent::API, "Traffic data routes registered");
    }

    // Register continuous optimization routes if controller is initialized
    if (continuous_optimization_controller_) {
        continuous_optimization_controller_->registerRoutes(http_server_);
        LOG_INFO(LogComponent::API, "Continuous optimization routes registered");
    }

    // Register prediction routes if controller is initialized
    if (prediction_controller_) {
        prediction_controller_->registerRoutes(http_server_);
        LOG_INFO(LogComponent::API, "Prediction routes registered");
    }

    LOG_INFO(LogComponent::API, "API routes configured");
}

void Server::handleHealth(const httplib::Request& req, httplib::Response& res) {
    // Calculate uptime
    auto now = std::chrono::steady_clock::now();
    auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        now - server_start_time_).count();

    json response = {
        {"status", "healthy"},
        {"service", "RATMS API Server"},
        {"version", "0.2.0"},
        {"timestamp", std::time(nullptr)},
        {"simulation", {
            {"running", simulation_running_.load()},
            {"paused", simulation_paused_.load()},
            {"continuousMode", continuous_mode_.load()},
            {"currentStep", simulation_steps_.load()},
            {"stepLimit", step_limit_},
            {"simulationTime", simulation_time_.load()}
        }},
        {"restartCount", restart_count_.load()},
        {"uptime", uptime_seconds}
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

void Server::handleSimulationStartContinuous(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (simulation_running_) {
        LOG_WARN(LogComponent::API, "Continuous start request rejected: simulation already running");
        sendError(res, 400, "Simulation already running");
        return;
    }

    if (!simulator_) {
        LOG_ERROR(LogComponent::API, "Continuous start request failed: simulator not initialized");
        sendError(res, 500, "Simulator not initialized");
        return;
    }

    // Enable continuous mode
    continuous_mode_ = true;
    LOG_INFO(LogComponent::Simulation, "Continuous mode enabled");

    // Create database record if database is available
    if (database_ && database_->isConnected()) {
        int sim_id = database_->createSimulation(
            "Continuous Simulation",
            "Continuous simulation started via REST API",
            1, // Default network ID
            "{\"continuous\": true}"
        );
        if (sim_id > 0) {
            current_simulation_id_ = sim_id;
            database_->updateSimulationStatus(sim_id, "running");
            LOG_INFO(LogComponent::Database, "Continuous simulation record created with ID: {}", sim_id);
        }
    }

    // Reset simulation state
    simulation_should_stop_ = false;
    simulation_paused_ = false;
    simulation_steps_ = 0;
    simulation_time_ = 0.0;
    simulation_running_ = true;

    // Start simulation in background thread
    simulation_thread_ = std::make_unique<std::thread>(&Server::runSimulationLoop, this);

    LOG_INFO(LogComponent::Simulation, "Continuous simulation started via API");

    json response = {
        {"message", "Continuous simulation started successfully"},
        {"status", "running"},
        {"continuousMode", true},
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

    // Parse viewport parameters from query string
    Viewport viewport;
    if (req.has_param("minLat")) viewport.minLat = std::stod(req.get_param_value("minLat"));
    if (req.has_param("maxLat")) viewport.maxLat = std::stod(req.get_param_value("maxLat"));
    if (req.has_param("minLon")) viewport.minLon = std::stod(req.get_param_value("minLon"));
    if (req.has_param("maxLon")) viewport.maxLon = std::stod(req.get_param_value("maxLon"));
    if (req.has_param("maxVehicles")) viewport.maxVehicles = std::stoi(req.get_param_value("maxVehicles"));
    if (req.has_param("maxTrafficLights")) viewport.maxTrafficLights = std::stoi(req.get_param_value("maxTrafficLights"));

    LOG_INFO(LogComponent::SSE, "Client connected to simulation stream (viewport: [{:.4f},{:.4f}]-[{:.4f},{:.4f}], maxVehicles={})",
             viewport.minLat, viewport.minLon, viewport.maxLat, viewport.maxLon, viewport.maxVehicles);

    res.set_chunked_content_provider(
        "text/event-stream",
        [this, viewport](size_t offset, httplib::DataSink &sink) {
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

                // Convert snapshot to JSON with viewport filtering
                json data = {
                    {"step", snapshot.step},
                    {"time", snapshot.time},
                    {"vehicles", json::array()},
                    {"trafficLights", json::array()},
                    {"totalVehicles", snapshot.vehicles.size()},
                    {"totalTrafficLights", snapshot.trafficLights.size()}
                };

                // Filter and limit vehicles
                int vehicleCount = 0;
                int skipInterval = 1;

                // If viewport is default and we have many vehicles, sample them
                if (viewport.isDefault() && static_cast<int>(snapshot.vehicles.size()) > viewport.maxVehicles) {
                    skipInterval = static_cast<int>(snapshot.vehicles.size()) / viewport.maxVehicles + 1;
                }

                int idx = 0;
                for (const auto& v : snapshot.vehicles) {
                    // Apply viewport filter
                    if (!viewport.isDefault() && !viewport.contains(v.lat, v.lon)) {
                        ++idx;
                        continue;
                    }

                    // Apply sampling for large datasets
                    if (idx % skipInterval != 0) {
                        ++idx;
                        continue;
                    }

                    // Check vehicle limit
                    if (vehicleCount >= viewport.maxVehicles) {
                        break;
                    }

                    data["vehicles"].push_back({
                        {"id", v.id},
                        {"roadId", v.roadId},
                        {"lane", v.lane},
                        {"position", v.position},
                        {"velocity", v.velocity},
                        {"acceleration", v.acceleration},
                        {"lat", v.lat},
                        {"lon", v.lon}
                    });
                    ++vehicleCount;
                    ++idx;
                }

                // Filter and limit traffic lights
                int tlCount = 0;
                int tlSkipInterval = 1;

                // If viewport is default and we have many traffic lights, sample them
                if (viewport.isDefault() && static_cast<int>(snapshot.trafficLights.size()) > viewport.maxTrafficLights) {
                    tlSkipInterval = static_cast<int>(snapshot.trafficLights.size()) / viewport.maxTrafficLights + 1;
                }

                int tlIdx = 0;
                for (const auto& tl : snapshot.trafficLights) {
                    // Apply viewport filter
                    if (!viewport.isDefault() && !viewport.contains(tl.lat, tl.lon)) {
                        ++tlIdx;
                        continue;
                    }

                    // Apply sampling for large datasets
                    if (tlIdx % tlSkipInterval != 0) {
                        ++tlIdx;
                        continue;
                    }

                    // Check traffic light limit
                    if (tlCount >= viewport.maxTrafficLights) {
                        break;
                    }

                    data["trafficLights"].push_back({
                        {"roadId", tl.roadId},
                        {"lane", tl.lane},
                        {"state", std::string(1, tl.state)},
                        {"lat", tl.lat},
                        {"lon", tl.lon}
                    });
                    ++tlCount;
                    ++tlIdx;
                }

                // Add filtered counts
                data["filteredVehicles"] = vehicleCount;
                data["filteredTrafficLights"] = data["trafficLights"].size();

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

void Server::rebuildRoadCache() {
    // Must be called with sim_mutex_ held
    cached_road_ptrs_.clear();
    if (simulator_) {
        cached_road_ptrs_.reserve(simulator_->cityMap.size());
        for (auto& [id, road] : simulator_->cityMap) {
            cached_road_ptrs_.push_back(&road);
        }
    }
    road_cache_valid_ = true;
    LOG_DEBUG(LogComponent::Simulation, "Road cache rebuilt: {} roads", cached_road_ptrs_.size());
}

void Server::runSimulationLoop() {
    LOG_INFO(LogComponent::Simulation, "Simulation loop started (continuous_mode={}, step_limit={})",
             continuous_mode_.load(), step_limit_);

    const double dt = 0.1;  // Time step
    const int metricsInterval = 10;  // Collect metrics every 10 steps
    const int dbWriteInterval = 100;  // Write to DB every 100 steps

    simulator::MetricsCollector metricsCollector;

    // Build road pointer cache and pre-allocate transition vectors once at start
    {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        rebuildRoadCache();

        // Pre-allocate thread-local transition vectors
        const int numThreads = omp_get_max_threads();
        thread_transitions_.resize(numThreads);
        const size_t roadsPerThread = cached_road_ptrs_.size() / numThreads + 1;
        for (auto& tv : thread_transitions_) {
            tv.reserve(roadsPerThread);
        }
        pending_transitions_.reserve(cached_road_ptrs_.size() / 10);  // Rough estimate

        LOG_INFO(LogComponent::Simulation, "Simulation initialized: {} roads, {} threads",
                 cached_road_ptrs_.size(), numThreads);
    }

    try {
        // Loop condition: stop if requested OR (not continuous mode AND reached step limit)
        while (!simulation_should_stop_ && (continuous_mode_ || simulation_steps_ < step_limit_)) {
            // Pause checkpoint - wait while paused
            while (simulation_paused_ && !simulation_should_stop_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if (simulation_should_stop_) break;
            int currentStep = simulation_steps_;

            // PHASE 1: Parallel road updates with per-thread transition vectors
            {
                std::lock_guard<std::mutex> lock(sim_mutex_);
                if (!simulator_) {
                    LOG_ERROR(LogComponent::Simulation, "Simulator became null during simulation");
                    break;
                }

                // Use cached road pointers (rebuilt once at start, not every step)
                if (!road_cache_valid_) {
                    rebuildRoadCache();
                }

                // Clear pre-allocated thread transition vectors (keeps capacity)
                for (auto& tv : thread_transitions_) {
                    tv.clear();
                }

                // Parallel road updates - each thread writes to its own transition vector
                #pragma omp parallel for schedule(dynamic, 50)
                for (size_t i = 0; i < cached_road_ptrs_.size(); ++i) {
                    int tid = omp_get_thread_num();
                    cached_road_ptrs_[i]->update(dt, simulator_->cityMap, thread_transitions_[tid]);
                }

                // Merge thread-local transitions into pre-allocated pending vector
                pending_transitions_.clear();
                for (const auto& tv : thread_transitions_) {
                    pending_transitions_.insert(pending_transitions_.end(), tv.begin(), tv.end());
                }
            }

            // PHASE 2: Execute road transitions
            {
                std::lock_guard<std::mutex> lock(sim_mutex_);
                if (!simulator_) break;

                for (const auto& transition : pending_transitions_) {
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

            // PHASE 2.5: Process vehicle spawning
            processVehicleSpawning(dt);

            // PHASE 3: Collect metrics periodically
            if (currentStep % metricsInterval == 0) {
                std::lock_guard<std::mutex> lock(sim_mutex_);
                if (simulator_) {
                    metricsCollector.collectMetrics(simulator_->cityMap, dt);

                    // Update travel time tracking
                    if (travel_time_collector_) {
                        travel_time_collector_->update(simulator_->cityMap, dt);
                    }
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

                // Log periodic simulation status (every 10 seconds of sim time)
                int vehicleCount = 0;
                for (const auto& [_, road] : simulator_->cityMap) {
                    vehicleCount += road.getVehicleCount();
                }
                LOG_INFO(LogComponent::Simulation, "Step {}: {:.1f}s sim time, {} vehicles active, {:.0f} exited",
                         currentStep, simulation_time_.load(), vehicleCount, metricsCollector.getMetrics().vehiclesExited);
            }

            // PHASE 4.5: Record traffic pattern snapshots periodically (real-time interval)
            if (pattern_storage_) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_snapshot_time_).count();

                if (elapsed >= pattern_snapshot_interval_seconds_) {
                    std::lock_guard<std::mutex> lock(sim_mutex_);
                    if (simulator_) {
                        std::vector<data::RoadMetrics> roadMetrics;
                        roadMetrics.reserve(simulator_->cityMap.size());

                        for (auto& [roadId, road] : simulator_->cityMap) {
                            data::RoadMetrics rm;
                            rm.roadId = roadId;
                            rm.vehicleCount = road.getVehicleCount();

                            // Calculate queue length (vehicles within 50m of traffic light)
                            double queueLength = 0;
                            double totalSpeed = 0;
                            int vehicleCountForSpeed = 0;

                            for (unsigned lane = 0; lane < road.getLanesNo(); ++lane) {
                                for (const auto& v : road.getVehicles()[lane]) {
                                    double distToEnd = road.getLength() - v.getPos();
                                    if (distToEnd < 50.0) {
                                        queueLength += 1.0;
                                    }
                                    totalSpeed += v.getVelocity();
                                    vehicleCountForSpeed++;
                                }
                            }

                            rm.queueLength = queueLength;
                            rm.avgSpeed = vehicleCountForSpeed > 0 ? totalSpeed / vehicleCountForSpeed : 0.0;
                            rm.flowRate = 0.0;  // TODO: Calculate from vehicle exits

                            roadMetrics.push_back(rm);
                        }

                        if (!roadMetrics.empty()) {
                            pattern_storage_->recordSnapshotBatch(roadMetrics);
                            LOG_DEBUG(LogComponent::Database, "Recorded traffic pattern snapshot for {} roads", roadMetrics.size());
                        }
                    }

                    last_snapshot_time_ = now;
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
        } else if (!continuous_mode_ && simulation_steps_ >= step_limit_) {
            LOG_INFO(LogComponent::Simulation, "Simulation completed: reached step limit {} at step {}",
                     step_limit_, simulation_steps_.load());
        } else {
            LOG_INFO(LogComponent::Simulation, "Simulation completed naturally at step {}", simulation_steps_.load());
        }

    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::Simulation, "Exception in simulation loop: {}", e.what());
        restart_count_++;
        LOG_WARN(LogComponent::Simulation, "Restart count incremented to {}", restart_count_.load());
    }

    LOG_INFO(LogComponent::Simulation, "Simulation loop ended: steps={}, time={:.2f}, continuous_mode={}",
             simulation_steps_.load(), simulation_time_.load(), continuous_mode_.load());
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

            // Get road geometry for lat/lon interpolation
            auto startGeo = road.getStartPosGeo();
            auto endGeo = road.getEndPosGeo();
            double roadLength = road.getLength();

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

                    // Interpolate lat/lon along road
                    double t = (roadLength > 0) ? (vehicle.getPos() / roadLength) : 0.0;
                    t = std::min(1.0, std::max(0.0, t));  // Clamp to [0,1]
                    vSnap.lon = startGeo.first + t * (endGeo.first - startGeo.first);
                    vSnap.lat = startGeo.second + t * (endGeo.second - startGeo.second);

                    // Apply lane offset perpendicular to road (approx)
                    if (laneIdx > 0) {
                        vSnap.lat += 0.00001 * laneIdx;
                    }

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

void Server::handleGetTrafficLights(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (!simulator_) {
        sendError(res, 500, "Simulator not initialized");
        return;
    }

    json trafficLights = json::array();

    for (const auto& [roadId, road] : simulator_->cityMap) {
        const auto& roadTrafficLights = road.getTrafficLights();
        for (unsigned lane = 0; lane < road.getLanesNo(); lane++) {
            const auto& tl = roadTrafficLights[lane];
            trafficLights.push_back({
                {"roadId", roadId},
                {"lane", lane},
                {"greenTime", tl.getGreenTime()},
                {"yellowTime", tl.getYellowTime()},
                {"redTime", tl.getRedTime()},
                {"currentState", std::string(1, tl.getState())}
            });
        }
    }

    json response = {
        {"trafficLights", trafficLights},
        {"count", trafficLights.size()}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSetTrafficLights(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    try {
        json requestBody = json::parse(req.body);

        if (!requestBody.contains("updates") || !requestBody["updates"].is_array()) {
            sendError(res, 400, "Request must contain 'updates' array");
            return;
        }

        std::lock_guard<std::mutex> lock(sim_mutex_);

        if (!simulator_) {
            sendError(res, 500, "Simulator not initialized");
            return;
        }

        int updated = 0;
        json errors = json::array();

        for (const auto& update : requestBody["updates"]) {
            if (!update.contains("roadId") || !update.contains("lane")) {
                errors.push_back({{"error", "Missing roadId or lane"}});
                continue;
            }

            int roadId = update["roadId"].get<int>();
            unsigned lane = update["lane"].get<unsigned>();

            auto roadIt = simulator_->cityMap.find(roadId);
            if (roadIt == simulator_->cityMap.end()) {
                errors.push_back({{"roadId", roadId}, {"error", "Road not found"}});
                continue;
            }

            auto& road = roadIt->second;
            if (lane >= road.getLanesNo()) {
                errors.push_back({{"roadId", roadId}, {"lane", lane}, {"error", "Lane out of range"}});
                continue;
            }

            auto& roadTrafficLights = road.getTrafficLightsMutable();
            double greenTime = update.value("greenTime", roadTrafficLights[lane].getGreenTime());
            double yellowTime = update.value("yellowTime", roadTrafficLights[lane].getYellowTime());
            double redTime = update.value("redTime", roadTrafficLights[lane].getRedTime());

            if (greenTime <= 0 || yellowTime < 0 || redTime <= 0) {
                errors.push_back({{"roadId", roadId}, {"lane", lane}, {"error", "Invalid timing values"}});
                continue;
            }

            // Reconstruct traffic light with new timings
            roadTrafficLights[lane] = simulator::TrafficLight(greenTime, yellowTime, redTime);
            updated++;

            LOG_INFO(LogComponent::API, "Updated traffic light: road={}, lane={}, g={}, y={}, r={}",
                     roadId, lane, greenTime, yellowTime, redTime);
        }

        json response = {
            {"success", true},
            {"updated", updated}
        };

        if (!errors.empty()) {
            response["errors"] = errors;
        }

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    }
}

// ============================================================================
// Spawn Rate Handlers
// ============================================================================

void Server::handleGetSpawnRates(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    std::lock_guard<std::mutex> lock(spawn_mutex_);

    json spawnRatesArray = json::array();

    for (const auto& [roadId, rate] : spawn_rates_) {
        spawnRatesArray.push_back({
            {"roadId", rate.roadId},
            {"vehiclesPerMinute", rate.vehiclesPerMinute},
            {"accumulator", rate.accumulator}
        });
    }

    json response = {
        {"spawnRates", spawnRatesArray},
        {"count", spawn_rates_.size()}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSetSpawnRates(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    try {
        json body = json::parse(req.body);

        if (!body.contains("rates") || !body["rates"].is_array()) {
            sendError(res, 400, "Request must contain 'rates' array");
            return;
        }

        std::lock_guard<std::mutex> lock(spawn_mutex_);

        int updated = 0;
        json errors = json::array();

        for (const auto& rateObj : body["rates"]) {
            if (!rateObj.contains("roadId") || !rateObj.contains("vehiclesPerMinute")) {
                errors.push_back({{"error", "Each rate must have roadId and vehiclesPerMinute"}});
                continue;
            }

            simulator::roadID roadId = rateObj["roadId"].get<simulator::roadID>();
            double vpm = rateObj["vehiclesPerMinute"].get<double>();

            // Validate road exists
            {
                std::lock_guard<std::mutex> simLock(sim_mutex_);
                if (simulator_->cityMap.find(roadId) == simulator_->cityMap.end()) {
                    errors.push_back({{"roadId", roadId}, {"error", "Road not found"}});
                    continue;
                }
            }

            if (vpm < 0) {
                errors.push_back({{"roadId", roadId}, {"error", "vehiclesPerMinute must be >= 0"}});
                continue;
            }

            if (vpm == 0) {
                // Remove spawn rate for this road
                spawn_rates_.erase(roadId);
                LOG_INFO(LogComponent::API, "Removed spawn rate for road {}", roadId);
            } else {
                // Set or update spawn rate
                spawn_rates_[roadId] = SpawnRate{roadId, vpm, 0.0};
                LOG_INFO(LogComponent::API, "Set spawn rate: road={}, vpm={:.2f}", roadId, vpm);
            }
            updated++;
        }

        json response = {
            {"success", true},
            {"updated", updated}
        };

        if (!errors.empty()) {
            response["errors"] = errors;
        }

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    }
}

/**
 * @brief Process vehicle spawning based on configured spawn rates
 * @param dt - Time step in seconds
 *
 * Uses accumulator pattern: adds (rate * dt / 60) to accumulator each step.
 * When accumulator >= 1.0, spawns a vehicle and decrements.
 */
void Server::processVehicleSpawning(double dt) {
    std::lock_guard<std::mutex> lock(spawn_mutex_);
    std::lock_guard<std::mutex> sim_lock(sim_mutex_);  // Protect simulator_ access

    for (auto& [roadId, rate] : spawn_rates_) {
        // Add partial vehicles based on rate and time step
        // rate is vehicles per minute, dt is in seconds
        rate.accumulator += (rate.vehiclesPerMinute * dt) / 60.0;

        // Spawn vehicles when accumulator reaches 1.0
        while (rate.accumulator >= 1.0) {
            auto roadIt = simulator_->cityMap.find(roadId);
            if (roadIt != simulator_->cityMap.end()) {
                // Spawn with road's max speed as initial velocity
                double initialVelocity = roadIt->second.getMaxSpeed() * 0.8;  // 80% of max speed
                if (roadIt->second.spawnVehicle(initialVelocity)) {
                    rate.accumulator -= 1.0;
                    LOG_DEBUG(LogComponent::Simulation, "Spawned vehicle on road {}", roadId);
                } else {
                    // Road is full, stop trying to spawn more this step
                    break;
                }
            } else {
                LOG_WARN(LogComponent::Simulation, "Road {} not found for spawning", roadId);
                rate.accumulator = 0.0;
                break;
            }
        }
    }
}

// ============================================================================
// Continuous Simulation Mode Handlers
// ============================================================================

void Server::handleSimulationPause(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!simulation_running_) {
        sendError(res, 400, "Simulation not running");
        return;
    }

    if (simulation_paused_) {
        sendError(res, 400, "Simulation already paused");
        return;
    }

    simulation_paused_ = true;
    LOG_INFO(LogComponent::Simulation, "Simulation paused at step {}", simulation_steps_.load());

    json response = {
        {"success", true},
        {"message", "Simulation paused"},
        {"step", simulation_steps_.load()},
        {"time", simulation_time_.load()}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSimulationResume(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!simulation_running_) {
        sendError(res, 400, "Simulation not running");
        return;
    }

    if (!simulation_paused_) {
        sendError(res, 400, "Simulation not paused");
        return;
    }

    simulation_paused_ = false;
    LOG_INFO(LogComponent::Simulation, "Simulation resumed at step {}", simulation_steps_.load());

    json response = {
        {"success", true},
        {"message", "Simulation resumed"},
        {"step", simulation_steps_.load()},
        {"time", simulation_time_.load()}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleGetSimulationConfig(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    json response = {
        {"stepLimit", step_limit_},
        {"continuousMode", continuous_mode_.load()}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSetSimulationConfig(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    try {
        json body = json::parse(req.body);

        if (body.contains("stepLimit")) {
            int newLimit = body["stepLimit"].get<int>();
            if (newLimit < 100) {
                sendError(res, 400, "stepLimit must be at least 100");
                return;
            }
            step_limit_ = newLimit;
            LOG_INFO(LogComponent::Simulation, "Step limit updated to {}", step_limit_);
        }

        if (body.contains("continuousMode")) {
            bool newMode = body["continuousMode"].get<bool>();
            continuous_mode_ = newMode;
            LOG_INFO(LogComponent::Simulation, "Continuous mode set to {}", newMode);
        }

        json response = {
            {"success", true},
            {"stepLimit", step_limit_},
            {"continuousMode", continuous_mode_.load()}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    }
}

/**
 * @brief Detect entry roads (roads with no incoming connections)
 * @return Vector of road IDs that are network entry points
 *
 * An entry road is one that no other road connects to.
 * These are the natural spawn points for vehicles entering the network.
 */
std::vector<simulator::roadID> Server::detectEntryRoads() {
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (!simulator_) {
        return {};
    }

    // Build set of all roads that are connection targets
    std::set<simulator::roadID> hasIncoming;
    for (const auto& [roadId, road] : simulator_->cityMap) {
        // Check all lanes for outgoing connections
        const auto& connections = road.getConnections();
        for (size_t lane = 0; lane < connections.size(); lane++) {
            for (const auto& [targetId, prob] : connections[lane]) {
                hasIncoming.insert(targetId);
            }
        }
    }

    // Entry roads = roads NOT in hasIncoming set
    std::vector<simulator::roadID> entryRoads;
    for (const auto& [roadId, road] : simulator_->cityMap) {
        if (hasIncoming.find(roadId) == hasIncoming.end()) {
            entryRoads.push_back(roadId);
        }
    }

    return entryRoads;
}

/**
 * @brief Initialize default spawn rates for all entry roads
 * @param vehiclesPerMinute - Spawn rate to apply to each entry road
 *
 * Automatically detects entry roads (roads with no incoming connections)
 * and sets a spawn rate for each. This enables automatic vehicle spawning
 * when loading map files.
 */
void Server::initializeDefaultSpawnRates(double vehiclesPerMinute) {
    auto entryRoads = detectEntryRoads();

    {
        std::lock_guard<std::mutex> lock(spawn_mutex_);

        for (simulator::roadID roadId : entryRoads) {
            spawn_rates_[roadId] = SpawnRate{roadId, vehiclesPerMinute, 0.0};
        }
    }

    LOG_INFO(LogComponent::Simulation, "Initialized spawn rates for {} entry roads at {} vehicles/minute each",
             entryRoads.size(), vehiclesPerMinute);
}

/**
 * @brief Pre-populate roads with vehicles at startup
 * @param density - Fraction of road capacity to fill (0.0-1.0)
 *
 * Distributes vehicles along all roads based on safe following distance.
 * This simulates how production data feeders would populate the network,
 * providing immediate visual feedback instead of waiting for spawning.
 */
void Server::populateRoadsWithVehicles(double density) {
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (!simulator_) {
        LOG_WARN(LogComponent::Simulation, "Cannot populate roads - simulator not initialized");
        return;
    }

    int totalSpawned = 0;
    for (auto& [roadId, road] : simulator_->cityMap) {
        double roadLength = road.getLength();
        double maxSpeed = road.getMaxSpeed();

        // Skip very short roads
        if (roadLength < 15.0) continue;

        // Calculate safe following distance: ~1s headway at max speed + vehicle length
        // Tighter spacing = more vehicles on longer roads
        double safeDistance = maxSpeed * 1.0 + 8.0;  // 8m = vehicle (5m) + min gap (3m)

        // Calculate vehicles per lane based on density
        int vehiclesPerLane = static_cast<int>((roadLength / safeDistance) * density);
        if (vehiclesPerLane < 1) continue;

        for (unsigned lane = 0; lane < road.getLanesNo(); ++lane) {
            for (int i = 0; i < vehiclesPerLane; ++i) {
                double position = i * safeDistance + 5.0;  // Start 5m in from road start
                if (position >= roadLength - 10.0) break;  // Leave room at road end

                // Random velocity between 50-100% of max speed
                double velocity = maxSpeed * (0.5 + (rand() % 50) / 100.0);

                // Random aggressivity (0.3-0.7 for normal distribution)
                double aggressivity = 0.3 + (rand() % 40) / 100.0;

                simulator::Vehicle v(position, 5.0, velocity);
                v.setAggressivity(aggressivity);
                road.addVehicle(v, lane);
                totalSpawned++;
            }
        }
    }

    LOG_INFO(LogComponent::Simulation, "Pre-populated roads with {} vehicles (density={:.0f}%)",
             totalSpawned, density * 100);
}

void Server::setNumThreads(int n) {
    num_threads_ = n;
    if (n > 0) {
        omp_set_num_threads(n);
        LOG_INFO(LogComponent::Simulation, "Set thread count to {}", n);
    } else {
        // Auto: use hardware concurrency
        int hw = std::thread::hardware_concurrency();
        omp_set_num_threads(hw);
        num_threads_ = 0;  // Keep as 0 to indicate auto mode
        LOG_INFO(LogComponent::Simulation, "Auto thread count: {} logical cores", hw);
    }
}

int Server::getNumThreads() const {
    return num_threads_ > 0 ? num_threads_ : omp_get_max_threads();
}

// Traffic Pattern Handlers

void Server::handleGetPatterns(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!pattern_storage_) {
        sendError(res, 503, "Traffic pattern storage not initialized");
        return;
    }

    try {
        json response;

        // Check for query parameters
        if (req.has_param("day") && req.has_param("slot")) {
            int dayOfWeek = std::stoi(req.get_param_value("day"));
            int timeSlot = std::stoi(req.get_param_value("slot"));

            auto patterns = pattern_storage_->getPatterns(dayOfWeek, timeSlot);

            json patternsJson = json::array();
            for (const auto& p : patterns) {
                patternsJson.push_back({
                    {"id", p.id},
                    {"roadId", p.roadId},
                    {"dayOfWeek", p.dayOfWeek},
                    {"timeSlot", p.timeSlot},
                    {"timeSlotString", data::TrafficPatternStorage::timeSlotToString(p.timeSlot)},
                    {"avgVehicleCount", p.avgVehicleCount},
                    {"avgQueueLength", p.avgQueueLength},
                    {"avgSpeed", p.avgSpeed},
                    {"avgFlowRate", p.avgFlowRate},
                    {"minVehicleCount", p.minVehicleCount},
                    {"maxVehicleCount", p.maxVehicleCount},
                    {"stddevVehicleCount", p.stddevVehicleCount},
                    {"sampleCount", p.sampleCount},
                    {"lastUpdated", p.lastUpdated}
                });
            }

            response = {
                {"status", "ok"},
                {"dayOfWeek", dayOfWeek},
                {"timeSlot", timeSlot},
                {"count", patterns.size()},
                {"patterns", patternsJson}
            };
        } else if (req.has_param("road")) {
            int roadId = std::stoi(req.get_param_value("road"));
            auto patterns = pattern_storage_->getPatternsForRoad(roadId);

            json patternsJson = json::array();
            for (const auto& p : patterns) {
                patternsJson.push_back({
                    {"id", p.id},
                    {"roadId", p.roadId},
                    {"dayOfWeek", p.dayOfWeek},
                    {"timeSlot", p.timeSlot},
                    {"timeSlotString", data::TrafficPatternStorage::timeSlotToString(p.timeSlot)},
                    {"avgVehicleCount", p.avgVehicleCount},
                    {"avgQueueLength", p.avgQueueLength},
                    {"avgSpeed", p.avgSpeed},
                    {"avgFlowRate", p.avgFlowRate},
                    {"sampleCount", p.sampleCount},
                    {"lastUpdated", p.lastUpdated}
                });
            }

            response = {
                {"status", "ok"},
                {"roadId", roadId},
                {"count", patterns.size()},
                {"patterns", patternsJson}
            };
        } else {
            // Return all patterns
            auto patterns = pattern_storage_->getAllPatterns();

            json patternsJson = json::array();
            for (const auto& p : patterns) {
                patternsJson.push_back({
                    {"id", p.id},
                    {"roadId", p.roadId},
                    {"dayOfWeek", p.dayOfWeek},
                    {"timeSlot", p.timeSlot},
                    {"timeSlotString", data::TrafficPatternStorage::timeSlotToString(p.timeSlot)},
                    {"avgVehicleCount", p.avgVehicleCount},
                    {"avgQueueLength", p.avgQueueLength},
                    {"avgSpeed", p.avgSpeed},
                    {"avgFlowRate", p.avgFlowRate},
                    {"sampleCount", p.sampleCount},
                    {"lastUpdated", p.lastUpdated}
                });
            }

            auto [currentDay, currentSlot] = data::TrafficPatternStorage::getCurrentDayAndSlot();

            response = {
                {"status", "ok"},
                {"count", patterns.size()},
                {"currentDayOfWeek", currentDay},
                {"currentTimeSlot", currentSlot},
                {"currentTimeSlotString", data::TrafficPatternStorage::timeSlotToString(currentSlot)},
                {"patterns", patternsJson}
            };
        }

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error retrieving patterns: ") + e.what());
    }
}

void Server::handleGetSnapshots(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!pattern_storage_) {
        sendError(res, 503, "Traffic pattern storage not initialized");
        return;
    }

    try {
        int hours = 24;  // Default: last 24 hours
        if (req.has_param("hours")) {
            hours = std::stoi(req.get_param_value("hours"));
        }

        // Get snapshots for the specified time range
        int64_t now = std::time(nullptr);
        int64_t cutoff = now - (hours * 3600);
        auto snapshots = pattern_storage_->getSnapshotsRange(cutoff, now);

        json snapshotsJson = json::array();
        for (const auto& s : snapshots) {
            json snapshotObj;
            snapshotObj["timestamp"] = s.timestamp;
            snapshotObj["roadId"] = s.roadId;
            snapshotObj["vehicleCount"] = s.vehicleCount;
            snapshotObj["queueLength"] = s.queueLength;
            snapshotObj["avgSpeed"] = s.avgSpeed;
            snapshotObj["flowRate"] = s.flowRate;
            snapshotsJson.push_back(snapshotObj);
        }

        json response;
        response["status"] = "ok";
        response["hours"] = hours;
        response["count"] = snapshots.size();
        response["snapshots"] = snapshotsJson;

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error retrieving snapshots: ") + e.what());
    }
}

void Server::handleAggregatePatterns(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!pattern_storage_) {
        sendError(res, 503, "Traffic pattern storage not initialized");
        return;
    }

    try {
        bool success = pattern_storage_->aggregateSnapshots();

        json response = {
            {"status", success ? "ok" : "error"},
            {"message", success ? "Patterns aggregated successfully" : "Aggregation failed"}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = success ? 200 : 500;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error aggregating patterns: ") + e.what());
    }
}

void Server::handlePruneSnapshots(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!pattern_storage_) {
        sendError(res, 503, "Traffic pattern storage not initialized");
        return;
    }

    try {
        int days = 7;  // Default: keep last 7 days
        if (!req.body.empty()) {
            json body = json::parse(req.body);
            if (body.contains("days")) {
                days = body["days"].get<int>();
            }
        }

        int deleted = pattern_storage_->pruneOldSnapshots(days);

        json response = {
            {"status", "ok"},
            {"daysRetained", days},
            {"snapshotsDeleted", deleted}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error pruning snapshots: ") + e.what());
    }
}

// ============================================================================
// Traffic Profile Handlers
// ============================================================================

void Server::handleGetProfiles(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        auto profiles = profile_service_->getAllProfiles();

        json profilesJson = json::array();
        for (const auto& p : profiles) {
            profilesJson.push_back({
                {"id", p.id},
                {"name", p.name},
                {"description", p.description},
                {"isActive", p.isActive},
                {"createdAt", p.createdAt},
                {"spawnRateCount", p.spawnRates.size()},
                {"trafficLightCount", p.trafficLights.size()}
            });
        }

        json response = {
            {"profiles", profilesJson},
            {"count", profiles.size()}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error getting profiles: ") + e.what());
    }
}

void Server::handleGetProfile(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        std::string nameOrId = req.matches[1];

        TrafficProfileService::TrafficProfile profile;

        // Check if it's a numeric ID or a name
        try {
            int profileId = std::stoi(nameOrId);
            profile = profile_service_->getProfile(profileId);
        } catch (const std::invalid_argument&) {
            profile = profile_service_->getProfileByName(nameOrId);
        }

        if (profile.id <= 0) {
            sendError(res, 404, "Profile not found");
            return;
        }

        // Build spawn rates JSON
        json spawnRatesJson = json::array();
        for (const auto& rate : profile.spawnRates) {
            spawnRatesJson.push_back({
                {"roadId", rate.road_id},
                {"lane", rate.lane},
                {"vehiclesPerMinute", rate.vehicles_per_minute}
            });
        }

        // Build traffic lights JSON
        json trafficLightsJson = json::array();
        for (const auto& light : profile.trafficLights) {
            trafficLightsJson.push_back({
                {"roadId", light.road_id},
                {"lane", light.lane},
                {"greenTime", light.green_time},
                {"yellowTime", light.yellow_time},
                {"redTime", light.red_time}
            });
        }

        json response = {
            {"id", profile.id},
            {"name", profile.name},
            {"description", profile.description},
            {"isActive", profile.isActive},
            {"createdAt", profile.createdAt},
            {"spawnRates", spawnRatesJson},
            {"trafficLights", trafficLightsJson}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error getting profile: ") + e.what());
    }
}

void Server::handleCreateProfile(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        json body = json::parse(req.body);

        std::string name = body.value("name", "");
        std::string description = body.value("description", "");

        if (name.empty()) {
            sendError(res, 400, "Profile name is required");
            return;
        }

        int profileId = profile_service_->createProfile(name, description);

        if (profileId <= 0) {
            sendError(res, 500, "Failed to create profile");
            return;
        }

        // Save spawn rates if provided
        if (body.contains("spawnRates") && body["spawnRates"].is_array()) {
            std::vector<data::DatabaseManager::ProfileSpawnRateRecord> spawnRates;
            for (const auto& rate : body["spawnRates"]) {
                data::DatabaseManager::ProfileSpawnRateRecord record;
                record.profile_id = profileId;
                record.road_id = rate.value("roadId", 0);
                record.lane = rate.value("lane", 0);
                record.vehicles_per_minute = rate.value("vehiclesPerMinute", 10.0);
                spawnRates.push_back(record);
            }

            std::vector<data::DatabaseManager::ProfileTrafficLightRecord> trafficLights;
            if (body.contains("trafficLights") && body["trafficLights"].is_array()) {
                for (const auto& light : body["trafficLights"]) {
                    data::DatabaseManager::ProfileTrafficLightRecord record;
                    record.profile_id = profileId;
                    record.road_id = light.value("roadId", 0);
                    record.lane = light.value("lane", 0);
                    record.green_time = light.value("greenTime", 30.0);
                    record.yellow_time = light.value("yellowTime", 3.0);
                    record.red_time = light.value("redTime", 30.0);
                    trafficLights.push_back(record);
                }
            }

            profile_service_->saveProfileData(profileId, spawnRates, trafficLights);
        }

        LOG_INFO(LogComponent::API, "Created profile '{}' with ID {}", name, profileId);

        json response = {
            {"success", true},
            {"id", profileId},
            {"name", name}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 201;

    } catch (const json::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error creating profile: ") + e.what());
    }
}

void Server::handleUpdateProfile(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        std::string nameOrId = req.matches[1];
        json body = json::parse(req.body);

        // Find profile by name or ID
        TrafficProfileService::TrafficProfile profile;
        try {
            int profileId = std::stoi(nameOrId);
            profile = profile_service_->getProfile(profileId);
        } catch (const std::invalid_argument&) {
            profile = profile_service_->getProfileByName(nameOrId);
        }

        if (profile.id <= 0) {
            sendError(res, 404, "Profile not found");
            return;
        }

        std::string newName = body.value("name", profile.name);
        std::string newDescription = body.value("description", profile.description);

        bool success = profile_service_->updateProfile(profile.id, newName, newDescription);

        // Update spawn rates if provided
        if (body.contains("spawnRates") && body["spawnRates"].is_array()) {
            std::vector<data::DatabaseManager::ProfileSpawnRateRecord> spawnRates;
            for (const auto& rate : body["spawnRates"]) {
                data::DatabaseManager::ProfileSpawnRateRecord record;
                record.profile_id = profile.id;
                record.road_id = rate.value("roadId", 0);
                record.lane = rate.value("lane", 0);
                record.vehicles_per_minute = rate.value("vehiclesPerMinute", 10.0);
                spawnRates.push_back(record);
            }

            std::vector<data::DatabaseManager::ProfileTrafficLightRecord> trafficLights;
            if (body.contains("trafficLights") && body["trafficLights"].is_array()) {
                for (const auto& light : body["trafficLights"]) {
                    data::DatabaseManager::ProfileTrafficLightRecord record;
                    record.profile_id = profile.id;
                    record.road_id = light.value("roadId", 0);
                    record.lane = light.value("lane", 0);
                    record.green_time = light.value("greenTime", 30.0);
                    record.yellow_time = light.value("yellowTime", 3.0);
                    record.red_time = light.value("redTime", 30.0);
                    trafficLights.push_back(record);
                }
            }

            profile_service_->saveProfileData(profile.id, spawnRates, trafficLights);
        }

        if (success) {
            LOG_INFO(LogComponent::API, "Updated profile '{}' (ID {})", newName, profile.id);
        }

        json response = {
            {"success", success},
            {"id", profile.id},
            {"name", newName}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = success ? 200 : 500;

    } catch (const json::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error updating profile: ") + e.what());
    }
}

void Server::handleDeleteProfile(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        std::string nameOrId = req.matches[1];

        // Find profile by name or ID
        TrafficProfileService::TrafficProfile profile;
        try {
            int profileId = std::stoi(nameOrId);
            profile = profile_service_->getProfile(profileId);
        } catch (const std::invalid_argument&) {
            profile = profile_service_->getProfileByName(nameOrId);
        }

        if (profile.id <= 0) {
            sendError(res, 404, "Profile not found");
            return;
        }

        bool success = profile_service_->deleteProfile(profile.id);

        if (success) {
            LOG_INFO(LogComponent::API, "Deleted profile '{}' (ID {})", profile.name, profile.id);
        }

        json response = {
            {"success", success},
            {"message", success ? "Profile deleted" : "Failed to delete profile"}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = success ? 200 : 500;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error deleting profile: ") + e.what());
    }
}

void Server::handleApplyProfile(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        std::string nameOrId = req.matches[1];

        bool success = false;

        // Try to apply by ID first, then by name
        try {
            int profileId = std::stoi(nameOrId);
            success = profile_service_->applyProfile(profileId);
        } catch (const std::invalid_argument&) {
            success = profile_service_->applyProfileByName(nameOrId);
        }

        if (!success) {
            sendError(res, 404, "Profile not found or failed to apply");
            return;
        }

        LOG_INFO(LogComponent::API, "Applied profile '{}'", nameOrId);

        json response = {
            {"success", true},
            {"message", "Profile applied successfully"}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error applying profile: ") + e.what());
    }
}

void Server::handleCaptureProfile(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        json body = json::parse(req.body);

        std::string name = body.value("name", "Captured Profile");
        std::string description = body.value("description", "Captured from current simulation state");

        int profileId = profile_service_->captureCurrentState(name, description);

        if (profileId <= 0) {
            sendError(res, 500, "Failed to capture current state");
            return;
        }

        LOG_INFO(LogComponent::API, "Captured current state as profile '{}' (ID {})", name, profileId);

        json response = {
            {"success", true},
            {"id", profileId},
            {"name", name},
            {"message", "Current state captured as new profile"}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 201;

    } catch (const json::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error capturing profile: ") + e.what());
    }
}

void Server::handleExportProfile(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        std::string nameOrId = req.matches[1];

        std::string jsonStr;

        // Try to export by ID first, then by name
        try {
            int profileId = std::stoi(nameOrId);
            jsonStr = profile_service_->exportProfileToJson(profileId);
        } catch (const std::invalid_argument&) {
            jsonStr = profile_service_->exportProfileToJson(nameOrId);
        }

        if (jsonStr == "{}") {
            sendError(res, 404, "Profile not found");
            return;
        }

        res.set_header("Content-Type", "application/json");
        res.set_header("Content-Disposition",
                      "attachment; filename=\"profile_" + nameOrId + ".json\"");
        res.set_content(jsonStr, "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error exporting profile: ") + e.what());
    }
}

void Server::handleImportProfile(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!profile_service_) {
        sendError(res, 503, "Profile service not initialized");
        return;
    }

    try {
        int profileId = profile_service_->importProfileFromJson(req.body);

        if (profileId <= 0) {
            sendError(res, 400, "Failed to import profile - invalid JSON format");
            return;
        }

        auto profile = profile_service_->getProfile(profileId);

        LOG_INFO(LogComponent::API, "Imported profile '{}' (ID {})", profile.name, profileId);

        json response = {
            {"success", true},
            {"id", profileId},
            {"name", profile.name},
            {"message", "Profile imported successfully"}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 201;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error importing profile: ") + e.what());
    }
}

// ============================================================================
// Travel Time Handlers
// ============================================================================

void Server::handleGetODPairs(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!travel_time_collector_) {
        sendError(res, 503, "Travel time collector not initialized");
        return;
    }

    try {
        auto odPairs = travel_time_collector_->getAllODPairs();

        json pairsJson = json::array();
        for (const auto& pair : odPairs) {
            pairsJson.push_back({
                {"id", pair.id},
                {"originRoadId", pair.originRoadId},
                {"destinationRoadId", pair.destinationRoadId},
                {"name", pair.name},
                {"description", pair.description}
            });
        }

        json response = {
            {"odPairs", pairsJson},
            {"count", odPairs.size()}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error getting O-D pairs: ") + e.what());
    }
}

void Server::handleCreateODPair(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!travel_time_collector_) {
        sendError(res, 503, "Travel time collector not initialized");
        return;
    }

    try {
        json body = json::parse(req.body);

        int originRoadId = body.value("originRoadId", -1);
        int destinationRoadId = body.value("destinationRoadId", -1);
        std::string name = body.value("name", "");
        std::string description = body.value("description", "");

        if (originRoadId < 0 || destinationRoadId < 0) {
            sendError(res, 400, "originRoadId and destinationRoadId are required");
            return;
        }

        int odPairId = travel_time_collector_->addODPair(originRoadId, destinationRoadId, name, description);

        LOG_INFO(LogComponent::API, "Created O-D pair {}: {} -> {}", odPairId, originRoadId, destinationRoadId);

        json response = {
            {"success", true},
            {"id", odPairId},
            {"originRoadId", originRoadId},
            {"destinationRoadId", destinationRoadId},
            {"name", name.empty() ? "Road " + std::to_string(originRoadId) + " -> " + std::to_string(destinationRoadId) : name}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 201;

    } catch (const json::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error creating O-D pair: ") + e.what());
    }
}

void Server::handleDeleteODPair(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!travel_time_collector_) {
        sendError(res, 503, "Travel time collector not initialized");
        return;
    }

    try {
        int odPairId = std::stoi(req.matches[1]);

        travel_time_collector_->removeODPair(odPairId);

        LOG_INFO(LogComponent::API, "Deleted O-D pair {}", odPairId);

        json response = {
            {"success", true},
            {"message", "O-D pair deleted"}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error deleting O-D pair: ") + e.what());
    }
}

void Server::handleGetTravelTimeStats(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!travel_time_collector_) {
        sendError(res, 503, "Travel time collector not initialized");
        return;
    }

    try {
        auto allStats = travel_time_collector_->getAllStats();

        json statsJson = json::array();
        for (const auto& stats : allStats) {
            statsJson.push_back({
                {"odPairId", stats.odPairId},
                {"avgTime", stats.avgTime},
                {"minTime", stats.minTime},
                {"maxTime", stats.maxTime},
                {"p50Time", stats.p50Time},
                {"p95Time", stats.p95Time},
                {"sampleCount", stats.sampleCount}
            });
        }

        json response = {
            {"stats", statsJson},
            {"count", allStats.size()}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error getting travel time stats: ") + e.what());
    }
}

void Server::handleGetODPairStats(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!travel_time_collector_) {
        sendError(res, 503, "Travel time collector not initialized");
        return;
    }

    try {
        int odPairId = std::stoi(req.matches[1]);

        auto stats = travel_time_collector_->getStats(odPairId);
        auto pair = travel_time_collector_->getODPair(odPairId);

        if (pair.id <= 0) {
            sendError(res, 404, "O-D pair not found");
            return;
        }

        json response = {
            {"odPair", {
                {"id", pair.id},
                {"originRoadId", pair.originRoadId},
                {"destinationRoadId", pair.destinationRoadId},
                {"name", pair.name},
                {"description", pair.description}
            }},
            {"stats", {
                {"avgTime", stats.avgTime},
                {"minTime", stats.minTime},
                {"maxTime", stats.maxTime},
                {"p50Time", stats.p50Time},
                {"p95Time", stats.p95Time},
                {"sampleCount", stats.sampleCount}
            }}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error getting O-D pair stats: ") + e.what());
    }
}

void Server::handleGetTravelTimeSamples(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!travel_time_collector_) {
        sendError(res, 503, "Travel time collector not initialized");
        return;
    }

    try {
        int odPairId = std::stoi(req.matches[1]);
        int limit = 100;

        if (req.has_param("limit")) {
            limit = std::stoi(req.get_param_value("limit"));
            if (limit < 1) limit = 1;
            if (limit > 1000) limit = 1000;
        }

        auto samples = travel_time_collector_->getRecentSamples(odPairId, limit);

        json samplesJson = json::array();
        for (const auto& sample : samples) {
            samplesJson.push_back({
                {"odPairId", sample.odPairId},
                {"vehicleId", sample.vehicleId},
                {"travelTimeSeconds", sample.travelTimeSeconds},
                {"startTime", sample.startTime},
                {"endTime", sample.endTime}
            });
        }

        json response = {
            {"samples", samplesJson},
            {"count", samples.size()},
            {"odPairId", odPairId}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error getting travel time samples: ") + e.what());
    }
}

void Server::handleGetTrackedVehicles(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!travel_time_collector_) {
        sendError(res, 503, "Travel time collector not initialized");
        return;
    }

    try {
        auto trackedVehicles = travel_time_collector_->getTrackedVehicles();

        json vehiclesJson = json::array();
        for (const auto& vehicle : trackedVehicles) {
            vehiclesJson.push_back({
                {"vehicleId", vehicle.vehicleId},
                {"odPairId", vehicle.odPairId},
                {"originRoadId", vehicle.originRoadId},
                {"destinationRoadId", vehicle.destinationRoadId}
            });
        }

        json response = {
            {"trackedVehicles", vehiclesJson},
            {"count", trackedVehicles.size()}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error getting tracked vehicles: ") + e.what());
    }
}

// ============================================================================
// Density Management Implementation
// ============================================================================

void Server::initializeDensityManagement() {
    if (!pattern_storage_) {
        LOG_WARN(LogComponent::Simulation, "Cannot initialize density management: pattern storage not available");
        return;
    }

    if (!simulator_) {
        LOG_WARN(LogComponent::Simulation, "Cannot initialize density management: simulator not available");
        return;
    }

    // Create feed storage
    if (database_) {
        feed_storage_ = std::make_shared<data::TrafficFeedStorage>(database_);
        LOG_INFO(LogComponent::Simulation, "Traffic feed storage initialized");
    }

    // Create simulated traffic feed
    traffic_feed_ = std::make_unique<simulator::SimulatedTrafficFeed>(
        *pattern_storage_, simulator_->cityMap);

    // Configure feed update interval
    traffic_feed_->setUpdateIntervalMs(density_config_.feedUpdateIntervalMs);

    // Subscribe to feed updates
    traffic_feed_->subscribe([this](const simulator::TrafficFeedSnapshot& snapshot) {
        onFeedUpdate(snapshot);
    });

    LOG_INFO(LogComponent::Simulation, "Density management initialized");
}

void Server::onFeedUpdate(const simulator::TrafficFeedSnapshot& snapshot) {
    // Save feed data for ML training (always, even if density adjustment is disabled)
    if (density_config_.saveFeedData && feed_storage_) {
        feed_storage_->recordFeedSnapshot(snapshot);
    }

    // Skip density adjustment if disabled
    if (!density_config_.enabled) {
        return;
    }

    if (!simulator_) {
        return;
    }

    // Random number generation for spawn positions
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Adjust vehicle counts per road
    std::lock_guard<std::mutex> lock(sim_mutex_);

    for (const auto& entry : snapshot.entries) {
        auto roadIt = simulator_->cityMap.find(entry.roadId);
        if (roadIt == simulator_->cityMap.end()) continue;

        auto& road = roadIt->second;
        int current = road.getVehicleCount();
        int expected = entry.expectedVehicleCount;
        int diff = expected - current;
        double tolerance = std::max(1.0, expected * density_config_.tolerancePercent);

        if (diff > tolerance) {
            // Under-populated: inject vehicles
            int toAdd = std::min(static_cast<int>(diff - tolerance),
                                 static_cast<int>(density_config_.maxAdjustmentRate));

            for (int i = 0; i < toAdd; i++) {
                // Random position along road (avoiding ends)
                double roadLen = road.getLength();
                std::uniform_real_distribution<double> posDist(10.0, roadLen - 10.0);
                double pos = posDist(gen);

                // Random lane
                std::uniform_int_distribution<unsigned> laneDist(0, road.getLanesNo() - 1);
                unsigned lane = laneDist(gen);

                // Random aggressivity
                std::uniform_real_distribution<double> aggrDist(0.3, 0.7);
                double aggr = aggrDist(gen);

                road.spawnVehicleAtPosition(pos, lane, road.getMaxSpeed() * 0.8, aggr);
            }

            LOG_TRACE(LogComponent::Simulation, "Density: Road {} injected {} vehicles (current={}, expected={})",
                     entry.roadId, toAdd, current, expected);

        } else if (diff < -tolerance) {
            // Over-populated: remove vehicles
            int toRemove = std::min(static_cast<int>(-diff - tolerance),
                                    static_cast<int>(density_config_.maxAdjustmentRate));

            for (int i = 0; i < toRemove; i++) {
                road.removeVehicle();
            }

            LOG_TRACE(LogComponent::Simulation, "Density: Road {} removed {} vehicles (current={}, expected={})",
                     entry.roadId, toRemove, current, expected);
        }
    }
}

void Server::handleGetDensityConfig(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    json response = {
        {"enabled", density_config_.enabled},
        {"maxAdjustmentRate", density_config_.maxAdjustmentRate},
        {"tolerancePercent", density_config_.tolerancePercent},
        {"saveFeedData", density_config_.saveFeedData},
        {"feedUpdateIntervalMs", density_config_.feedUpdateIntervalMs},
        {"feedRunning", traffic_feed_ ? traffic_feed_->isRunning() : false}
    };

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleSetDensityConfig(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    try {
        json body = json::parse(req.body);

        if (body.contains("enabled")) {
            density_config_.enabled = body["enabled"].get<bool>();
        }
        if (body.contains("maxAdjustmentRate")) {
            density_config_.maxAdjustmentRate = body["maxAdjustmentRate"].get<double>();
        }
        if (body.contains("tolerancePercent")) {
            density_config_.tolerancePercent = body["tolerancePercent"].get<double>();
        }
        if (body.contains("saveFeedData")) {
            density_config_.saveFeedData = body["saveFeedData"].get<bool>();
        }
        if (body.contains("feedUpdateIntervalMs")) {
            density_config_.feedUpdateIntervalMs = body["feedUpdateIntervalMs"].get<int>();
            if (traffic_feed_) {
                traffic_feed_->setUpdateIntervalMs(density_config_.feedUpdateIntervalMs);
            }
        }

        // Start or stop feed based on enabled state
        if (traffic_feed_) {
            if (density_config_.enabled && !traffic_feed_->isRunning()) {
                traffic_feed_->start();
                LOG_INFO(LogComponent::Simulation, "Traffic feed started");
            } else if (!density_config_.enabled && traffic_feed_->isRunning()) {
                traffic_feed_->stop();
                LOG_INFO(LogComponent::Simulation, "Traffic feed stopped");
            }
        }

        json response = {
            {"success", true},
            {"config", {
                {"enabled", density_config_.enabled},
                {"maxAdjustmentRate", density_config_.maxAdjustmentRate},
                {"tolerancePercent", density_config_.tolerancePercent},
                {"saveFeedData", density_config_.saveFeedData},
                {"feedUpdateIntervalMs", density_config_.feedUpdateIntervalMs}
            }}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 400, std::string("Invalid request: ") + e.what());
    }
}

void Server::handleGetDensityStatus(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!traffic_feed_) {
        sendError(res, 503, "Traffic feed not initialized");
        return;
    }

    try {
        auto snapshot = traffic_feed_->getLatestSnapshot();

        json roadsJson = json::array();

        std::lock_guard<std::mutex> lock(sim_mutex_);

        for (const auto& entry : snapshot.entries) {
            auto roadIt = simulator_->cityMap.find(entry.roadId);
            if (roadIt == simulator_->cityMap.end()) continue;

            int current = roadIt->second.getVehicleCount();
            int expected = entry.expectedVehicleCount;
            double tolerance = std::max(1.0, expected * density_config_.tolerancePercent);

            std::string status = "ok";
            if (current < expected - tolerance) {
                status = "under";
            } else if (current > expected + tolerance) {
                status = "over";
            }

            roadsJson.push_back({
                {"roadId", entry.roadId},
                {"current", current},
                {"expected", expected},
                {"confidence", entry.confidence},
                {"status", status}
            });
        }

        json response = {
            {"feedSource", traffic_feed_->getSourceName()},
            {"feedHealthy", traffic_feed_->isHealthy()},
            {"feedRunning", traffic_feed_->isRunning()},
            {"densityEnabled", density_config_.enabled},
            {"snapshotTimestamp", snapshot.timestamp},
            {"roadCount", roadsJson.size()},
            {"roads", roadsJson}
        };

        res.set_content(response.dump(2), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error getting density status: ") + e.what());
    }
}

void Server::handleGetFeedInfo(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    json response;

    if (traffic_feed_) {
        response = {
            {"source", traffic_feed_->getSourceName()},
            {"running", traffic_feed_->isRunning()},
            {"healthy", traffic_feed_->isHealthy()},
            {"updateIntervalMs", traffic_feed_->getUpdateIntervalMs()}
        };
    } else {
        response = {
            {"source", "none"},
            {"running", false},
            {"healthy", false},
            {"updateIntervalMs", 0}
        };
    }

    if (feed_storage_) {
        auto stats = feed_storage_->getStats();
        response["storage"] = {
            {"totalEntries", stats.totalEntries},
            {"uniqueRoads", stats.uniqueRoads},
            {"oldestTimestamp", stats.oldestTimestamp},
            {"newestTimestamp", stats.newestTimestamp}
        };
    }

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
}

void Server::handleExportFeedData(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!feed_storage_) {
        sendError(res, 503, "Feed storage not initialized");
        return;
    }

    try {
        // Parse query parameters
        int64_t startTime = 0;
        int64_t endTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        if (req.has_param("startTime")) {
            startTime = std::stoll(req.get_param_value("startTime"));
        }
        if (req.has_param("endTime")) {
            endTime = std::stoll(req.get_param_value("endTime"));
        }

        std::string format = req.has_param("format") ? req.get_param_value("format") : "json";

        if (format == "csv") {
            std::string csv = feed_storage_->exportToCSV(startTime, endTime);
            res.set_content(csv, "text/csv");
            res.set_header("Content-Disposition", "attachment; filename=feed_data.csv");
        } else {
            std::string jsonData = feed_storage_->exportToJSON(startTime, endTime);
            res.set_content(jsonData, "application/json");
        }

        res.status = 200;

    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Error exporting feed data: ") + e.what());
    }
}

} // namespace api
} // namespace ratms
