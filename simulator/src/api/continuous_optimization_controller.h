#ifndef CONTINUOUS_OPTIMIZATION_CONTROLLER_H
#define CONTINUOUS_OPTIMIZATION_CONTROLLER_H

#include "../external/httplib.h"
#include "../optimization/genetic_algorithm.h"
#include "../optimization/metrics.h"
#include "../core/simulator.h"
#include "../data/storage/database_manager.h"
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <chrono>
#include <vector>

namespace ratms {
namespace api {

/**
 * @brief TimingTransition - Represents a gradual timing change for one traffic light
 *
 * Transitions linearly interpolate between start and end timings over the
 * transition duration (default 5 minutes) to avoid sudden changes.
 */
struct TimingTransition {
    int roadId;
    unsigned lane;

    // Starting timings (current values when transition began)
    double startGreen;
    double startRed;

    // Target timings (from GA optimization)
    double endGreen;
    double endRed;

    // Transition timing
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    // Get current interpolated green time
    double getCurrentGreenTime() const;

    // Get current interpolated red time
    double getCurrentRedTime() const;

    // Check if transition is complete
    bool isComplete() const;

    // Get progress as fraction [0.0, 1.0]
    double getProgress() const;
};

/**
 * @brief ContinuousOptimizationController - Background GA optimization with gradual application
 *
 * This controller runs GA optimization periodically in the background and
 * gradually applies the results to the live simulation to avoid sudden
 * traffic light timing changes that could confuse drivers.
 *
 * Key features:
 * - Configurable optimization interval (default: 15 minutes)
 * - Shorter GA runs (30 generations) for faster response
 * - Gradual timing application (5-minute linear blend)
 * - Thread-safe integration with live simulation
 */
class ContinuousOptimizationController {
public:
    struct Config {
        // How often to run optimization (seconds)
        int optimizationIntervalSeconds = 900;  // 15 minutes

        // Duration to blend new timings (seconds)
        int transitionDurationSeconds = 300;    // 5 minutes

        // GA parameters for quick optimization
        int populationSize = 30;
        int generations = 30;
        int simulationSteps = 500;
        double dt = 0.1;

        // Timing bounds
        double minGreenTime = 10.0;
        double maxGreenTime = 60.0;
        double minRedTime = 10.0;
        double maxRedTime = 60.0;
    };

    ContinuousOptimizationController(
        std::shared_ptr<data::DatabaseManager> dbManager,
        std::shared_ptr<simulator::Simulator> simulator,
        std::mutex& simMutex
    );
    ~ContinuousOptimizationController();

    // Register API routes
    void registerRoutes(httplib::Server& server);

    // Start/stop continuous optimization
    void start();
    void stop();
    bool isRunning() const { return running_; }

    // Get/set configuration
    Config getConfig() const { return config_; }
    void setConfig(const Config& config);

    // Manually apply a completed optimization run
    bool applyOptimizationRun(int runId);

    // Get active transitions for monitoring
    std::vector<TimingTransition> getActiveTransitions() const;

    // Update transitions (called from simulation loop)
    void updateTransitions();

private:
    // Route handlers
    void handleStart(const httplib::Request& req, httplib::Response& res);
    void handleStop(const httplib::Request& req, httplib::Response& res);
    void handleStatus(const httplib::Request& req, httplib::Response& res);
    void handleApply(const httplib::Request& req, httplib::Response& res);
    void handleConfig(const httplib::Request& req, httplib::Response& res);
    void handleSetConfig(const httplib::Request& req, httplib::Response& res);

    // Background optimization loop
    void optimizationLoop();

    // Run a single optimization cycle
    void runOptimizationCycle();

    // Apply chromosome to simulation with gradual transitions
    void applyChromosomeGradually(const simulator::Chromosome& chromosome);

    // Apply current transition values to traffic lights
    void applyCurrentTransitionValues();

    std::shared_ptr<data::DatabaseManager> dbManager_;
    std::shared_ptr<simulator::Simulator> simulator_;
    std::mutex& simMutex_;

    Config config_;
    mutable std::mutex configMutex_;

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> optimizationThread_;

    // Active transitions being applied
    std::vector<TimingTransition> activeTransitions_;
    mutable std::mutex transitionsMutex_;

    // Stats
    std::atomic<int> totalOptimizationRuns_{0};
    std::atomic<int> successfulOptimizations_{0};
    std::atomic<double> lastImprovementPercent_{0.0};
    std::chrono::steady_clock::time_point lastOptimizationTime_;
    mutable std::mutex statsMutex_;
};

} // namespace api
} // namespace ratms

#endif // CONTINUOUS_OPTIMIZATION_CONTROLLER_H
