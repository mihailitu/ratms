#ifndef API_SERVER_H
#define API_SERVER_H

#include "../external/httplib.h"
#include "../core/simulator.h"
#include "../data/storage/database_manager.h"
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace ratms {
namespace api {

// Forward declaration
class OptimizationController;

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
    void handleSimulationStop(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStatus(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStream(const httplib::Request& req, httplib::Response& res);
    void handleGetRoads(const httplib::Request& req, httplib::Response& res);

    // Database query handlers
    void handleGetSimulations(const httplib::Request& req, httplib::Response& res);
    void handleGetSimulation(const httplib::Request& req, httplib::Response& res);
    void handleGetMetrics(const httplib::Request& req, httplib::Response& res);
    void handleGetNetworks(const httplib::Request& req, httplib::Response& res);

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

    // Simulation thread management
    std::unique_ptr<std::thread> simulation_thread_;
    std::atomic<bool> simulation_should_stop_{false};
    std::atomic<int> simulation_steps_{0};
    std::atomic<double> simulation_time_{0.0};

    // Real-time streaming
    SimulationSnapshot latest_snapshot_;
    std::mutex snapshot_mutex_;
    std::atomic<bool> has_new_snapshot_{false};

    // Simulation execution
    void runSimulationLoop();
    void captureSimulationSnapshot();
};

} // namespace api
} // namespace ratms

#endif // API_SERVER_H
