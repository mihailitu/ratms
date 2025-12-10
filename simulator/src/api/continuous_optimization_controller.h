#ifndef CONTINUOUS_OPTIMIZATION_CONTROLLER_H
#define CONTINUOUS_OPTIMIZATION_CONTROLLER_H

#include "../external/httplib.h"
#include "../optimization/genetic_algorithm.h"
#include "../optimization/metrics.h"
#include "../core/simulator.h"
#include "../data/storage/database_manager.h"
#include "../prediction/traffic_predictor.h"
#include "../data/storage/traffic_pattern_storage.h"
#include "../validation/timing_validator.h"
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <chrono>
#include <vector>

namespace ratms {
namespace api {

// Forward declarations
class PredictiveOptimizer;

/**
 * @brief RolloutState - Tracks the state of a timing rollout
 *
 * When optimized timings are applied, we monitor the rollout to detect
 * regressions and automatically rollback if performance degrades.
 */
struct RolloutState {
    int64_t startTime = 0;              // When rollout started (0 = no active rollout)
    int64_t endTime = 0;                // When rollout completed (0 = ongoing)
    std::string status = "idle";        // "idle", "in_progress", "complete", "rolled_back"

    // Pre-rollout baseline metrics
    double preRolloutAvgSpeed = 0.0;
    double preRolloutAvgQueue = 0.0;
    double preRolloutFitness = 0.0;

    // Post-rollout metrics (updated periodically)
    double postRolloutAvgSpeed = 0.0;
    double postRolloutAvgQueue = 0.0;
    double postRolloutFitness = 0.0;
    double regressionPercent = 0.0;     // Negative = improvement

    // The chromosomes for rollback capability
    simulator::Chromosome currentChromosome;
    simulator::Chromosome previousChromosome;

    // Monitoring counters
    int updateCount = 0;                // Number of metric updates since rollout

    bool isActive() const { return status == "in_progress"; }
};

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

        // Prediction mode settings
        bool usePrediction = false;           // Use predictive optimization
        int predictionHorizonMinutes = 30;    // How far ahead to predict (10-120)

        // Validation settings
        bool enableValidation = true;         // Validate before applying
        double validationImprovementThreshold = 5.0;  // Min improvement % to pass
        double validationRegressionThreshold = 10.0;  // Max regression % before rejection

        // Rollout monitoring settings
        bool enableRolloutMonitoring = true;  // Monitor post-rollout metrics
        double rolloutRegressionThreshold = 15.0;  // Auto-rollback threshold %
        int rolloutMonitoringDurationSeconds = 300;  // 5 minutes
    };

    ContinuousOptimizationController(
        std::shared_ptr<data::DatabaseManager> dbManager,
        std::shared_ptr<simulator::Simulator> simulator,
        std::mutex& simMutex
    );
    ~ContinuousOptimizationController();

    // Set up prediction support (called after pattern storage is initialized)
    void setPredictor(std::shared_ptr<prediction::TrafficPredictor> predictor);

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

    // Rollout monitoring
    RolloutState getRolloutState() const;
    bool rollback();
    void updateRolloutMetrics(double avgSpeed, double avgQueue);

    // Validation configuration
    validation::ValidationConfig getValidationConfig() const;
    void setValidationConfig(const validation::ValidationConfig& config);

private:
    // Route handlers
    void handleStart(const httplib::Request& req, httplib::Response& res);
    void handleStop(const httplib::Request& req, httplib::Response& res);
    void handleStatus(const httplib::Request& req, httplib::Response& res);
    void handleApply(const httplib::Request& req, httplib::Response& res);
    void handleConfig(const httplib::Request& req, httplib::Response& res);
    void handleSetConfig(const httplib::Request& req, httplib::Response& res);

    // Rollout and validation route handlers
    void handleRollback(const httplib::Request& req, httplib::Response& res);
    void handleRolloutStatus(const httplib::Request& req, httplib::Response& res);
    void handleValidationConfig(const httplib::Request& req, httplib::Response& res);
    void handleSetValidationConfig(const httplib::Request& req, httplib::Response& res);

    // Background optimization loop
    void optimizationLoop();

    // Run a single optimization cycle
    void runOptimizationCycle();

    // Apply chromosome to simulation with gradual transitions
    void applyChromosomeGradually(const simulator::Chromosome& chromosome);

    // Apply current transition values to traffic lights
    void applyCurrentTransitionValues();

    // Create a copy of the current network for optimization
    std::vector<simulator::Road> copyCurrentNetwork();

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

    // Prediction support
    std::shared_ptr<prediction::TrafficPredictor> predictor_;
    std::unique_ptr<PredictiveOptimizer> predictiveOptimizer_;

    // Rollout monitoring
    RolloutState rolloutState_;
    mutable std::mutex rolloutMutex_;

    // Validation
    std::unique_ptr<validation::TimingValidator> validator_;
    validation::ValidationConfig validationConfig_;

    // Internal rollout helpers
    void startRollout(const simulator::Chromosome& newChromosome,
                      const simulator::Chromosome& previousChromosome,
                      double preRolloutSpeed, double preRolloutQueue);
    bool checkForRegression();
    void completeRollout();
};

} // namespace api
} // namespace ratms

#endif // CONTINUOUS_OPTIMIZATION_CONTROLLER_H
