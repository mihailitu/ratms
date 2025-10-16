#ifndef API_SERVER_H
#define API_SERVER_H

#include "../external/httplib.h"
#include "../core/simulator.h"
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace ratms {
namespace api {

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

private:
    void setupRoutes();
    void setupMiddleware();

    // Route handlers
    void handleHealth(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStart(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStop(const httplib::Request& req, httplib::Response& res);
    void handleSimulationStatus(const httplib::Request& req, httplib::Response& res);

    // Middleware
    void corsMiddleware(const httplib::Request& req, httplib::Response& res);
    void loggingMiddleware(const httplib::Request& req, httplib::Response& res);

    httplib::Server http_server_;
    std::shared_ptr<simulator::Simulator> simulator_;
    std::unique_ptr<std::thread> server_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> simulation_running_{false};
    std::mutex sim_mutex_;
    int port_;
};

} // namespace api
} // namespace ratms

#endif // API_SERVER_H
