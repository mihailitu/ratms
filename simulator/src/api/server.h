#ifndef API_SERVER_H
#define API_SERVER_H

#include "../external/httplib.h"
#include "../core/simulator.h"
#include "../data/storage/database_manager.h"
#include "../data/storage/traffic_pattern_storage.h"
#include "../data/storage/traffic_feed_storage.h"
#include "../metrics/travel_time_collector.h"
#include "../feed/i_traffic_data_feed.h"
#include "../feed/simulated_traffic_feed.h"
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <vector>
#include <chrono>

namespace ratms {
namespace api {

// Forward declarations
class OptimizationController;
class TrafficDataController;
class ContinuousOptimizationController;
class PredictionController;
class TrafficProfileService;

/**
 * @brief VehicleSnapshot - Real-time vehicle position data
 */
struct VehicleSnapshot {
    int id;
    int roadId;
    unsigned lane;
    double position;
    double velocity;
    double acceleration;
    double lat;  // Geographic latitude
    double lon;  // Geographic longitude
};

/**
 * @brief Viewport - Geographic bounding box for filtering
 */
struct Viewport {
    double minLat = -90.0;
    double maxLat = 90.0;
    double minLon = -180.0;
    double maxLon = 180.0;
    int maxVehicles = 10000;       // Max vehicles to send (for performance)
    int maxTrafficLights = 5000;   // Max traffic lights to send

    bool contains(double lat, double lon) const {
        return lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon;
    }

    bool isDefault() const {
        return minLat == -90.0 && maxLat == 90.0 && minLon == -180.0 && maxLon == 180.0;
    }
};

/**
 * @brief TrafficLightSnapshot - Real-time traffic light state
 */
struct TrafficLightSnapshot {
    int roadId;
    unsigned lane;
    char state;  // 'R', 'Y', 'G'
    double lat;
    double lon;
};

/**
 * @brief SimulationSnapshot - Complete simulation state at a point in time
 */
struct SimulationSnapshot {
    int step;
    double time;
    std::vector<VehicleSnapshot> vehicles;
    std::vector<TrafficLightSnapshot> trafficLights;
};

/**
 * @brief SpawnRate - Vehicle spawn rate configuration per road
 */
struct SpawnRate {
    simulator::roadID roadId;
    double vehiclesPerMinute;  // Spawn rate
    double accumulator;        // Partial vehicle accumulator
};

/**
 * @brief DensityManagerConfig - Configuration for traffic density management
 */
struct DensityManagerConfig {
    bool enabled = false;                   // Whether density management is active
    double maxAdjustmentRate = 2.0;         // Max vehicles to add/remove per update
    double tolerancePercent = 0.2;          // Tolerance band (±20% before adjusting)
    bool saveFeedData = true;               // Save feed data to DB for ML training
    int feedUpdateIntervalMs = 1000;        // How often the feed generates updates
};

/**
 * @brief HTTP API Server for RATMS
 *
 * Provides REST API endpoints for:
 * - Simulation control (start/stop/status)
 * - Real-time simulation data
 * - Configuration management
 */
class Server {
public:
    Server(int port = 8080);
    ~Server();

    // Server lifecycle
    void start();
    void stop();
    bool isRunning() const { return running_; }

    // Simulation control
    void setSimulator(std::shared_ptr<simulator::Simulator> sim);
    void setDatabase(std::shared_ptr<data::DatabaseManager> db);

private:
    void setupRoutes();
    void setupMiddleware();

    // Route handlers
    void handleHealth(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStart(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStartContinuous(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStop(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStatus(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStream(const httplib::Request& req, httplib::Response& res);
    void handleGetRoads(const httplib::Request& req, httplib::Response& res);

    // Database query handlers
    void handleGetSimulations(const httplib::Request& req, httplib::Response& res);
    void handleGetSimulation(const httplib::Request& req, httplib::Response& res);
    void handleGetMetrics(const httplib::Request& req, httplib::Response& res);
    void handleGetNetworks(const httplib::Request& req, httplib::Response& res);

    // Analytics handlers
    void handleGetStatistics(const httplib::Request& req, httplib::Response& res);
    void handleGetStatisticsByType(const httplib::Request& req, httplib::Response& res);
    void handleCompareSimulations(const httplib::Request& req, httplib::Response& res);
    void handleExportMetrics(const httplib::Request& req, httplib::Response& res);
    void handleGetMetricTypes(const httplib::Request& req, httplib::Response& res);

    // Traffic light handlers
    void handleGetTrafficLights(const httplib::Request& req, httplib::Response& res);
    void handleSetTrafficLights(const httplib::Request& req, httplib::Response& res);

    // Spawn rate handlers
    void handleGetSpawnRates(const httplib::Request& req, httplib::Response& res);
    void handleSetSpawnRates(const httplib::Request& req, httplib::Response& res);

    // Continuous simulation mode handlers
    void handleSimulationPause(const httplib::Request& req, httplib::Response& res);
    void handleSimulationResume(const httplib::Request& req, httplib::Response& res);
    void handleGetSimulationConfig(const httplib::Request& req, httplib::Response& res);
    void handleSetSimulationConfig(const httplib::Request& req, httplib::Response& res);

    // Traffic pattern handlers
    void handleGetPatterns(const httplib::Request& req, httplib::Response& res);
    void handleGetSnapshots(const httplib::Request& req, httplib::Response& res);
    void handleAggregatePatterns(const httplib::Request& req, httplib::Response& res);
    void handlePruneSnapshots(const httplib::Request& req, httplib::Response& res);

    // Traffic profile handlers
    void handleGetProfiles(const httplib::Request& req, httplib::Response& res);
    void handleGetProfile(const httplib::Request& req, httplib::Response& res);
    void handleCreateProfile(const httplib::Request& req, httplib::Response& res);
    void handleUpdateProfile(const httplib::Request& req, httplib::Response& res);
    void handleDeleteProfile(const httplib::Request& req, httplib::Response& res);
    void handleApplyProfile(const httplib::Request& req, httplib::Response& res);
    void handleCaptureProfile(const httplib::Request& req, httplib::Response& res);
    void handleExportProfile(const httplib::Request& req, httplib::Response& res);
    void handleImportProfile(const httplib::Request& req, httplib::Response& res);

    // Travel time handlers
    void handleGetODPairs(const httplib::Request& req, httplib::Response& res);
    void handleCreateODPair(const httplib::Request& req, httplib::Response& res);
    void handleDeleteODPair(const httplib::Request& req, httplib::Response& res);
    void handleGetTravelTimeStats(const httplib::Request& req, httplib::Response& res);
    void handleGetODPairStats(const httplib::Request& req, httplib::Response& res);
    void handleGetTravelTimeSamples(const httplib::Request& req, httplib::Response& res);
    void handleGetTrackedVehicles(const httplib::Request& req, httplib::Response& res);

    // Density management handlers
    void handleGetDensityConfig(const httplib::Request& req, httplib::Response& res);
    void handleSetDensityConfig(const httplib::Request& req, httplib::Response& res);
    void handleGetDensityStatus(const httplib::Request& req, httplib::Response& res);
    void handleGetFeedInfo(const httplib::Request& req, httplib::Response& res);
    void handleExportFeedData(const httplib::Request& req, httplib::Response& res);

    // Middleware
    void corsMiddleware(const httplib::Request& req, httplib::Response& res);
    void loggingMiddleware(const httplib::Request& req, httplib::Response& res);

    httplib::Server http_server_;
    std::shared_ptr<simulator::Simulator> simulator_;
    std::shared_ptr<data::DatabaseManager> database_;
    std::unique_ptr<std::thread> server_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> simulation_running_{false};
    std::atomic<int> current_simulation_id_{-1};
    std::mutex sim_mutex_;
    int port_;

    // Controllers
    std::unique_ptr<OptimizationController> optimization_controller_;
    std::unique_ptr<TrafficDataController> traffic_data_controller_;
    std::unique_ptr<ContinuousOptimizationController> continuous_optimization_controller_;
    std::unique_ptr<PredictionController> prediction_controller_;
    std::unique_ptr<TrafficProfileService> profile_service_;

    // Traffic pattern storage
    std::shared_ptr<data::TrafficPatternStorage> pattern_storage_;
    std::chrono::steady_clock::time_point last_snapshot_time_;
    int pattern_snapshot_interval_seconds_{60};  // Default: record every 60 seconds

    // Travel time collection
    std::shared_ptr<metrics::TravelTimeCollector> travel_time_collector_;

    // Traffic density management
    DensityManagerConfig density_config_;
    std::unique_ptr<simulator::ITrafficDataFeed> traffic_feed_;
    std::shared_ptr<data::TrafficFeedStorage> feed_storage_;
    void initializeDensityManagement();
    void onFeedUpdate(const simulator::TrafficFeedSnapshot& snapshot);

    // Simulation thread management
    std::unique_ptr<std::thread> simulation_thread_;
    std::atomic<bool> simulation_should_stop_{false};
    std::atomic<int> simulation_steps_{0};
    std::atomic<double> simulation_time_{0.0};

    // Continuous simulation mode
    std::atomic<bool> continuous_mode_{false};
    std::atomic<bool> simulation_paused_{false};
    std::atomic<int> restart_count_{0};
    int step_limit_{10000};  // Configurable step limit (ignored in continuous mode)
    std::chrono::steady_clock::time_point server_start_time_;

    // Parallel simulation configuration
    int num_threads_{0};  // 0 = auto (use hardware concurrency)

    // Cached road pointers for efficient iteration (avoid map-to-vector each step)
    std::vector<simulator::Road*> cached_road_ptrs_;
    bool road_cache_valid_{false};
    void rebuildRoadCache();

    // Pre-allocated transition vectors (reused each step to avoid allocations)
    std::vector<std::vector<simulator::RoadTransition>> thread_transitions_;
    std::vector<simulator::RoadTransition> pending_transitions_;

    // Real-time streaming
    SimulationSnapshot latest_snapshot_;
    std::mutex snapshot_mutex_;
    std::atomic<bool> has_new_snapshot_{false};

    // Simulation execution
    void runSimulationLoop();
    void captureSimulationSnapshot();

    // Vehicle spawning
    std::map<simulator::roadID, SpawnRate> spawn_rates_;  // roadId -> spawn rate config
    std::mutex spawn_mutex_;
    void processVehicleSpawning(double dt);

public:
    // Entry road detection and auto-spawn initialization
    std::vector<simulator::roadID> detectEntryRoads();
    void initializeDefaultSpawnRates(double vehiclesPerMinute = 10.0);
    void populateRoadsWithVehicles(double density = 0.3);

    // Thread configuration for parallel simulation
    void setNumThreads(int n);  // Set thread count (0 = auto)
    int getNumThreads() const;
};

} // namespace api
} // namespace ratms

#endif // API_SERVER_H
