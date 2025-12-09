#include "traffic_data_controller.h"
#include "../utils/logger.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <random>

using json = nlohmann::json;

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

TrafficDataController::TrafficDataController(
    std::shared_ptr<data::DatabaseManager> db,
    std::shared_ptr<simulator::Simulator> sim,
    std::mutex& sim_mutex
) : database_(db), simulator_(sim), sim_mutex_(sim_mutex) {
    LOG_INFO(LogComponent::API, "TrafficDataController initialized");
}

TrafficDataController::~TrafficDataController() {
    stopSpawning();
}

void TrafficDataController::registerRoutes(httplib::Server& server) {
    // Traffic profiles
    server.Get("/api/traffic/profiles", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetProfiles(req, res);
    });

    server.Post("/api/traffic/profiles/active", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetActiveProfile(req, res);
    });

    // Flow rates
    server.Get("/api/traffic/flow-rates", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetFlowRates(req, res);
    });

    server.Post("/api/traffic/flow-rates", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetFlowRates(req, res);
    });

    // Spawning control
    server.Post("/api/traffic/spawning/start", [this](const httplib::Request& req, httplib::Response& res) {
        handleStartSpawning(req, res);
    });

    server.Post("/api/traffic/spawning/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleStopSpawning(req, res);
    });

    server.Get("/api/traffic/spawning/status", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetSpawningStatus(req, res);
    });

    LOG_INFO(LogComponent::API, "Traffic data routes registered");
}

void TrafficDataController::handleGetProfiles(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
        auto profiles = getProfiles();
        json profilesJson = json::array();

        for (const auto& profile : profiles) {
            profilesJson.push_back({
                {"id", profile.id},
                {"name", profile.name},
                {"description", profile.description},
                {"isActive", profile.isActive}
            });
        }

        sendSuccess(res, {{"profiles", profilesJson}});
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to get profiles: {}", e.what());
        sendError(res, 500, e.what());
    }
}

void TrafficDataController::handleSetActiveProfile(const httplib::Request& req, httplib::Response& res) {
    try {
        auto body = json::parse(req.body);

        if (!body.contains("profile")) {
            sendError(res, 400, "Missing 'profile' field");
            return;
        }

        std::string profileName = body["profile"].get<std::string>();

        if (setActiveProfile(profileName)) {
            sendSuccess(res, {
                {"message", "Active profile set"},
                {"activeProfile", profileName}
            });
        } else {
            sendError(res, 404, "Profile not found: " + profileName);
        }
    } catch (const json::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to set active profile: {}", e.what());
        sendError(res, 500, e.what());
    }
}

void TrafficDataController::handleGetFlowRates(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
        std::lock_guard<std::mutex> lock(flow_mutex_);

        json ratesJson = json::array();
        for (const auto& rate : flow_rates_) {
            ratesJson.push_back({
                {"roadId", rate.roadId},
                {"lane", rate.lane},
                {"vehiclesPerMinute", rate.vehiclesPerMinute}
            });
        }

        sendSuccess(res, {
            {"activeProfile", active_profile_},
            {"flowRates", ratesJson}
        });
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to get flow rates: {}", e.what());
        sendError(res, 500, e.what());
    }
}

void TrafficDataController::handleSetFlowRates(const httplib::Request& req, httplib::Response& res) {
    try {
        auto body = json::parse(req.body);

        if (!body.contains("flowRates") || !body["flowRates"].is_array()) {
            sendError(res, 400, "Missing or invalid 'flowRates' array");
            return;
        }

        std::vector<FlowRate> newRates;
        for (const auto& rateJson : body["flowRates"]) {
            FlowRate rate;
            rate.roadId = rateJson["roadId"].get<int>();
            rate.lane = rateJson["lane"].get<unsigned>();
            rate.vehiclesPerMinute = rateJson["vehiclesPerMinute"].get<double>();
            rate.accumulator = 0.0;
            newRates.push_back(rate);
        }

        setFlowRates(newRates);

        sendSuccess(res, {
            {"message", "Flow rates updated"},
            {"count", static_cast<int>(newRates.size())}
        });
    } catch (const json::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to set flow rates: {}", e.what());
        sendError(res, 500, e.what());
    }
}

void TrafficDataController::handleStartSpawning(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
        if (spawning_active_) {
            sendSuccess(res, {
                {"message", "Spawning already active"},
                {"activeProfile", active_profile_}
            });
            return;
        }

        startSpawning();

        sendSuccess(res, {
            {"message", "Vehicle spawning started"},
            {"activeProfile", active_profile_}
        });
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to start spawning: {}", e.what());
        sendError(res, 500, e.what());
    }
}

void TrafficDataController::handleStopSpawning(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
        if (!spawning_active_) {
            sendSuccess(res, {{"message", "Spawning was not active"}});
            return;
        }

        stopSpawning();

        sendSuccess(res, {{"message", "Vehicle spawning stopped"}});
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to stop spawning: {}", e.what());
        sendError(res, 500, e.what());
    }
}

void TrafficDataController::handleGetSpawningStatus(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
        std::lock_guard<std::mutex> lock(flow_mutex_);

        sendSuccess(res, {
            {"active", spawning_active_.load()},
            {"activeProfile", active_profile_},
            {"flowRateCount", static_cast<int>(flow_rates_.size())}
        });
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to get spawning status: {}", e.what());
        sendError(res, 500, e.what());
    }
}

std::vector<TrafficProfile> TrafficDataController::getProfiles() {
    std::vector<TrafficProfile> profiles;

    // Return hardcoded profiles for now (can be extended to use database)
    profiles.push_back({1, "morning_rush", "Morning rush hour (7am-9am)", active_profile_ == "morning_rush"});
    profiles.push_back({2, "evening_rush", "Evening rush hour (5pm-7pm)", active_profile_ == "evening_rush"});
    profiles.push_back({3, "off_peak", "Off-peak hours", active_profile_ == "off_peak"});

    return profiles;
}

TrafficProfile TrafficDataController::getActiveProfile() {
    auto profiles = getProfiles();
    for (const auto& profile : profiles) {
        if (profile.isActive) {
            return profile;
        }
    }
    return {3, "off_peak", "Off-peak hours", true};
}

bool TrafficDataController::setActiveProfile(const std::string& name) {
    // Validate profile name
    if (name != "morning_rush" && name != "evening_rush" && name != "off_peak") {
        return false;
    }

    active_profile_ = name;
    LOG_INFO(LogComponent::API, "Active traffic profile set to: {}", name);
    return true;
}

std::vector<FlowRate> TrafficDataController::getFlowRates() {
    std::lock_guard<std::mutex> lock(flow_mutex_);
    return flow_rates_;
}

void TrafficDataController::setFlowRates(const std::vector<FlowRate>& rates) {
    std::lock_guard<std::mutex> lock(flow_mutex_);
    flow_rates_ = rates;
    LOG_INFO(LogComponent::API, "Flow rates updated: {} entries", rates.size());
}

void TrafficDataController::startSpawning() {
    if (spawning_active_) {
        return;
    }

    spawning_active_ = true;
    spawning_thread_ = std::make_unique<std::thread>(&TrafficDataController::spawningLoop, this);
    LOG_INFO(LogComponent::API, "Vehicle spawning started");
}

void TrafficDataController::stopSpawning() {
    if (!spawning_active_) {
        return;
    }

    spawning_active_ = false;
    if (spawning_thread_ && spawning_thread_->joinable()) {
        spawning_thread_->join();
    }
    spawning_thread_.reset();
    LOG_INFO(LogComponent::API, "Vehicle spawning stopped");
}

void TrafficDataController::spawningLoop() {
    auto lastTime = std::chrono::steady_clock::now();

    while (spawning_active_) {
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - lastTime).count();
        lastTime = now;

        spawnVehicles(dt);

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(SPAWN_INTERVAL_MS)));
    }
}

void TrafficDataController::spawnVehicles(double dt) {
    std::lock_guard<std::mutex> flowLock(flow_mutex_);

    if (flow_rates_.empty()) {
        return;
    }

    std::lock_guard<std::mutex> simLock(sim_mutex_);

    if (!simulator_) {
        return;
    }

    // Random number generator for aggressivity
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> aggressivityDist(0.3, 0.7);

    for (auto& rate : flow_rates_) {
        // Accumulate fractional vehicles based on rate and elapsed time
        double vehiclesToSpawn = rate.vehiclesPerMinute * (dt / 60.0);
        rate.accumulator += vehiclesToSpawn;

        // Spawn whole vehicles when accumulator >= 1
        while (rate.accumulator >= 1.0) {
            rate.accumulator -= 1.0;

            // Find the road and spawn vehicle
            auto roadIt = simulator_->cityMap.find(rate.roadId);
            if (roadIt != simulator_->cityMap.end()) {
                double aggressivity = aggressivityDist(gen);
                double initialVelocity = roadIt->second.getMaxSpeed() * 0.5;

                bool spawned = roadIt->second.spawnVehicle(rate.lane, initialVelocity, aggressivity);
                if (spawned) {
                    LOG_DEBUG(LogComponent::Simulation, "Spawned vehicle on road {} lane {}",
                             rate.roadId, rate.lane);
                }
            }
        }
    }
}

} // namespace api
} // namespace ratms
