#ifndef OPTIMIZATION_CONTROLLER_H
#define OPTIMIZATION_CONTROLLER_H

#include "../external/httplib.h"
#include <nlohmann/json.hpp>
#include "../optimization/genetic_algorithm.h"
#include "../optimization/metrics.h"
#include "../core/simulator.h"
#include "../data/storage/database_manager.h"

#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

namespace ratms {
namespace api {

using json = nlohmann::json;

/**
 * @brief OptimizationRun - Represents a running or completed optimization
 */
struct OptimizationRun {
    int id;
    std::string status;  // "pending", "running", "completed", "failed"

    // GA parameters
    simulator::GeneticAlgorithm::Parameters gaParams;
    int simulationSteps;
    double dt;

    // Results
    double baselineFitness = 0.0;
    double bestFitness = 0.0;
    double improvementPercent = 0.0;

    // Evolution history
    std::vector<double> fitnessHistory;
    simulator::Chromosome bestChromosome;

    // Progress tracking
    std::atomic<int> currentGeneration{0};
    std::atomic<bool> isRunning{false};

    // Timestamps
    int64_t startedAt = 0;
    int64_t completedAt = 0;

    // Thread for background execution
    std::unique_ptr<std::thread> optimizationThread;
};

/**
 * @brief OptimizationController - Handles GA optimization API endpoints
 */
class OptimizationController {
private:
    std::shared_ptr<ratms::data::DatabaseManager> dbManager_;
    std::map<int, std::shared_ptr<OptimizationRun>> activeRuns_;
    std::mutex runsMutex_;
    int nextRunId_ = 1;

public:
    explicit OptimizationController(std::shared_ptr<ratms::data::DatabaseManager> dbManager);
    ~OptimizationController();

    // Register routes with the HTTP server
    void registerRoutes(httplib::Server& server);

private:
    // Route handlers
    void handleStartOptimization(const httplib::Request& req, httplib::Response& res);
    void handleGetStatus(const httplib::Request& req, httplib::Response& res);
    void handleGetResults(const httplib::Request& req, httplib::Response& res);
    void handleGetHistory(const httplib::Request& req, httplib::Response& res);
    void handleStopOptimization(const httplib::Request& req, httplib::Response& res);

    // Helper methods
    int createOptimizationRun(const simulator::GeneticAlgorithm::Parameters& params,
                             int simulationSteps, double dt, int networkId);
    void runOptimizationBackground(std::shared_ptr<OptimizationRun> run, int networkId);
    void saveOptimizationResults(std::shared_ptr<OptimizationRun> run, int dbRunId);
    void loadOptimizationHistory();
    json runToJson(const std::shared_ptr<OptimizationRun>& run, bool includeFullHistory);

    // Test network creation
    std::vector<simulator::Road> createTestNetwork();
};

} // namespace api
} // namespace ratms

#endif // OPTIMIZATION_CONTROLLER_H
