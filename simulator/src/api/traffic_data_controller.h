#ifndef TRAFFIC_DATA_CONTROLLER_H
#define TRAFFIC_DATA_CONTROLLER_H

#include "../external/httplib.h"
#include "../core/simulator.h"
#include "../data/storage/database_manager.h"
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>

namespace ratms {
namespace api {

/**
 * @brief FlowRate - Vehicle spawn rate configuration per road/lane
 */
struct FlowRate {
    int roadId;
    unsigned lane;
    double vehiclesPerMinute;
    double accumulator{0.0};  // Fractional vehicle accumulator for spawning
};

/**
 * @brief TrafficProfile - Named traffic pattern configuration
 */
struct TrafficProfile {
    int id;
    std::string name;
    std::string description;
    bool isActive;
};

/**
 * @brief TrafficDataController - Manages traffic profiles and vehicle spawning
 *
 * Provides REST API endpoints for:
 * - Traffic profile management (CRUD)
 * - Flow rate configuration per road/lane
 * - Background vehicle spawning based on flow rates
 */
class TrafficDataController {
public:
    TrafficDataController(
        std::shared_ptr<data::DatabaseManager> db,
        std::shared_ptr<simulator::Simulator> sim,
        std::mutex& sim_mutex
    );
    ~TrafficDataController();

    // Register HTTP routes
    void registerRoutes(httplib::Server& server);

    // Spawning control
    void startSpawning();
    void stopSpawning();
    bool isSpawning() const { return spawning_active_; }

    // Profile management
    std::vector<TrafficProfile> getProfiles();
    TrafficProfile getActiveProfile();
    bool setActiveProfile(const std::string& name);

    // Flow rate management
    std::vector<FlowRate> getFlowRates();
    void setFlowRates(const std::vector<FlowRate>& rates);

private:
    // Route handlers
    void handleGetProfiles(const httplib::Request& req, httplib::Response& res);
    void handleSetActiveProfile(const httplib::Request& req, httplib::Response& res);
    void handleGetFlowRates(const httplib::Request& req, httplib::Response& res);
    void handleSetFlowRates(const httplib::Request& req, httplib::Response& res);
    void handleStartSpawning(const httplib::Request& req, httplib::Response& res);
    void handleStopSpawning(const httplib::Request& req, httplib::Response& res);
    void handleGetSpawningStatus(const httplib::Request& req, httplib::Response& res);

    // Background spawning
    void spawningLoop();
    void spawnVehicles(double dt);

    // Database
    std::shared_ptr<data::DatabaseManager> database_;
    std::shared_ptr<simulator::Simulator> simulator_;
    std::mutex& sim_mutex_;

    // Spawning state
    std::atomic<bool> spawning_active_{false};
    std::unique_ptr<std::thread> spawning_thread_;
    std::vector<FlowRate> flow_rates_;
    std::mutex flow_mutex_;
    std::string active_profile_{"off_peak"};

    // Configuration
    static constexpr double SPAWN_INTERVAL_MS = 100.0;  // Check every 100ms
};

} // namespace api
} // namespace ratms

#endif // TRAFFIC_DATA_CONTROLLER_H
