#include "server.h"
#include "optimization_controller.h"
#include "../utils/logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ratms {
namespace api {

Server::Server(int port) : port_(port) {
    log_info("API Server initialized on port %d", port_);
}

Server::~Server() {
    stop();
}

void Server::start() {
    if (running_) {
        log_warning("Server already running");
        return;
    }

    setupMiddleware();
    setupRoutes();

    running_ = true;

    // Start server in separate thread
    server_thread_ = std::make_unique<std::thread>([this]() {
        log_info("Starting HTTP server on http://localhost:%d", port_);
        http_server_.listen("0.0.0.0", port_);
    });

    log_info("API Server started successfully");
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

    log_info("API Server stopped");
}

void Server::setSimulator(std::shared_ptr<simulator::Simulator> sim) {
    std::lock_guard<std::mutex> lock(sim_mutex_);
    simulator_ = sim;
    log_info("Simulator instance attached to API server");
}

void Server::setDatabase(std::shared_ptr<data::DatabaseManager> db) {
    database_ = db;
    log_info("Database manager attached to API server");

    // Initialize optimization controller
    if (database_) {
        optimization_controller_ = std::make_unique<OptimizationController>(database_);
        log_info("Optimization controller initialized");
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

    // Logging middleware
    http_server_.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        log_info("HTTP %s %s - %d", req.method.c_str(), req.path.c_str(), res.status);
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

    // Register optimization routes if controller is initialized
    if (optimization_controller_) {
        optimization_controller_->registerRoutes(http_server_);
        log_info("Optimization routes registered");
    }

    log_info("API routes configured");
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
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (simulation_running_) {
        json response = {
            {"error", "Simulation already running"},
            {"status", "running"}
        };
        res.set_content(response.dump(2), "application/json");
        res.status = 400;
        return;
    }

    if (!simulator_) {
        json response = {
            {"error", "Simulator not initialized"},
            {"status", "error"}
        };
        res.set_content(response.dump(2), "application/json");
        res.status = 500;
        return;
    }

    simulation_running_ = true;

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
            log_info("Simulation record created with ID: %d", sim_id);
        }
    }

    log_info("Simulation started via API");

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
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (!simulation_running_) {
        json response = {
            {"error", "Simulation not running"},
            {"status", "stopped"}
        };
        res.set_content(response.dump(2), "application/json");
        res.status = 400;
        return;
    }

    simulation_running_ = false;

    // Complete database record if database is available
    if (database_ && database_->isConnected() && current_simulation_id_ > 0) {
        long end_time = std::time(nullptr);
        database_->completeSimulation(current_simulation_id_, end_time, 0.0);
        log_info("Simulation record %d completed", current_simulation_id_.load());
        current_simulation_id_ = -1;
    }

    log_info("Simulation stopped via API");

    json response = {
        {"message", "Simulation stopped successfully"},
        {"status", "stopped"},
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

    res.set_content(response.dump(2), "application/json");
    res.status = 200;
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
        json response = {
            {"error", "Database not available"}
        };
        res.set_content(response.dump(2), "application/json");
        res.status = 503;
        return;
    }

    int sim_id = std::stoi(req.matches[1]);
    auto sim = database_->getSimulation(sim_id);

    if (sim.id == 0) {
        json response = {
            {"error", "Simulation not found"},
            {"simulation_id", sim_id}
        };
        res.set_content(response.dump(2), "application/json");
        res.status = 404;
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
        json response = {
            {"error", "Database not available"}
        };
        res.set_content(response.dump(2), "application/json");
        res.status = 503;
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

} // namespace api
} // namespace ratms
