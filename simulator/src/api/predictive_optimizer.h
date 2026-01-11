#ifndef PREDICTIVE_OPTIMIZER_H
#define PREDICTIVE_OPTIMIZER_H

#include "../prediction/traffic_predictor.h"
#include "../optimization/genetic_algorithm.h"
#include "../optimization/metrics.h"
#include "../core/simulator.h"
#include "../data/storage/database_manager.h"
#include "../validation/timing_validator.h"
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <optional>

namespace ratms {
namespace api {

/**
 * @brief Pipeline status for predictive optimization
 */
enum class PipelineStatus {
    IDLE,           // Not running
    PREDICTING,     // Getting traffic prediction
    OPTIMIZING,     // Running GA optimization
    VALIDATING,     // Validating optimized timings (Phase 4)
    APPLYING,       // Applying timings to simulation
    COMPLETE,       // Cycle complete
    ERROR           // Error occurred
};

inline std::string pipelineStatusToString(PipelineStatus status) {
    switch (status) {
        case PipelineStatus::IDLE: return "idle";
        case PipelineStatus::PREDICTING: return "predicting";
        case PipelineStatus::OPTIMIZING: return "optimizing";
        case PipelineStatus::VALIDATING: return "validating";
        case PipelineStatus::APPLYING: return "applying";
        case PipelineStatus::COMPLETE: return "complete";
        case PipelineStatus::ERROR: return "error";
        default: return "unknown";
    }
}

/**
 * @brief Configuration for predictive optimization
 */
struct PredictiveOptimizerConfig {
    // Prediction settings
    int predictionHorizonMinutes = 30;    // How far ahead to predict (10-120)

    // GA optimization settings
    int populationSize = 30;
    int generations = 30;
    int simulationSteps = 500;
    double dt = 0.1;

    // Timing bounds
    double minGreenTime = 10.0;
    double maxGreenTime = 60.0;
    double minRedTime = 10.0;
    double maxRedTime = 60.0;

    // Vehicle scaling for predicted network
    double vehicleScaleFactor = 1.0;      // Scale factor for predicted vehicle counts
    bool adjustSpawnRates = true;         // Whether to adjust spawn rates based on prediction
};

/**
 * @brief Result of a predictive optimization run
 */
struct PredictiveOptimizationResult {
    int runId;                            // Database run ID (-1 if not persisted)
    int64_t startTime;                    // When optimization started
    int64_t endTime;                      // When optimization completed
    int horizonMinutes;                   // Prediction horizon used

    // Prediction info
    int predictedDayOfWeek;
    int predictedTimeSlot;
    std::string predictedTimeSlotString;
    double averagePredictionConfidence;

    // Optimization results
    double baselineFitness;
    double optimizedFitness;
    double improvementPercent;

    // Best chromosome (if optimization succeeded)
    std::optional<simulator::Chromosome> bestChromosome;

    // Validation result (if validation was performed)
    std::optional<validation::ValidationResult> validationResult;

    // Status
    PipelineStatus finalStatus;
    std::string errorMessage;
};

/**
 * @brief Accuracy tracking for prediction validation
 */
struct PredictionAccuracy {
    int64_t timestamp;
    int horizonMinutes;

    // Predicted vs actual vehicle counts (averaged across roads)
    double predictedVehicleCount;
    double actualVehicleCount;
    double vehicleCountError;             // Absolute error

    // Predicted vs actual queue lengths
    double predictedQueueLength;
    double actualQueueLength;
    double queueLengthError;

    // Overall accuracy score (0.0-1.0)
    double accuracyScore;
};

/**
 * @brief PredictiveOptimizer - Runs GA optimization on predicted future traffic state
 *
 * This class combines traffic prediction with GA optimization to proactively
 * optimize traffic light timings for anticipated future conditions rather than
 * current conditions.
 *
 * Pipeline:
 * 1. PREDICTING: Get predicted traffic state T+N minutes ahead
 * 2. OPTIMIZING: Run GA optimization on predicted network state
 * 3. VALIDATING: (Phase 4) Validate optimized timings in simulation
 * 4. APPLYING: Apply optimized timings gradually
 * 5. COMPLETE: Cycle finished
 *
 * Key features:
 * - Configurable prediction horizon (10-120 minutes)
 * - Creates predicted network state for GA evaluation
 * - Tracks prediction accuracy over time
 * - Thread-safe operation
 */
class PredictiveOptimizer {
public:
    PredictiveOptimizer(
        std::shared_ptr<prediction::TrafficPredictor> predictor,
        std::shared_ptr<data::DatabaseManager> dbManager,
        std::shared_ptr<simulator::Simulator> simulator,
        std::mutex& simMutex
    );
    ~PredictiveOptimizer() = default;

    // Configuration
    void setConfig(const PredictiveOptimizerConfig& config);
    PredictiveOptimizerConfig getConfig() const;

    // Validation configuration
    void setValidationConfig(const validation::ValidationConfig& config);
    validation::ValidationConfig getValidationConfig() const;
    void setValidationEnabled(bool enabled) { validationEnabled_ = enabled; }

    // Run optimization (blocking)
    PredictiveOptimizationResult runOptimization();
    PredictiveOptimizationResult runOptimization(int horizonMinutes);

    // Pipeline status
    PipelineStatus getStatus() const { return currentStatus_; }
    std::string getStatusMessage() const;
    double getProgress() const;

    // Accuracy tracking
    void recordActualMetrics();           // Call after prediction horizon elapses
    std::vector<PredictionAccuracy> getAccuracyHistory() const;
    double getAverageAccuracy() const;

    // Apply optimized chromosome
    bool applyChromosome(const simulator::Chromosome& chromosome);

private:
    // Pipeline stages
    prediction::PredictionResult performPrediction(int horizonMinutes);
    std::vector<simulator::Road> createPredictedNetwork(
        const prediction::PredictionResult& prediction);
    simulator::Chromosome runGAOptimization(
        const std::vector<simulator::Road>& network,
        double& baselineFitness);
    bool persistResults(const PredictiveOptimizationResult& result);

    // Network modification based on predictions
    void adjustNetworkForPrediction(
        std::vector<simulator::Road>& network,
        const prediction::PredictionResult& prediction);
    void scaleVehicleCounts(
        std::vector<simulator::Road>& network,
        const std::map<int, double>& targetCounts);

    // Accuracy calculation
    double calculateAccuracyScore(
        double predictedValue,
        double actualValue) const;

    std::shared_ptr<prediction::TrafficPredictor> predictor_;
    std::shared_ptr<data::DatabaseManager> dbManager_;
    std::shared_ptr<simulator::Simulator> simulator_;
    std::mutex& simMutex_;

    PredictiveOptimizerConfig config_;
    mutable std::mutex configMutex_;

    // Pipeline state
    std::atomic<PipelineStatus> currentStatus_{PipelineStatus::IDLE};
    std::string statusMessage_;
    mutable std::mutex statusMutex_;

    // Prediction tracking for accuracy validation
    struct PendingPrediction {
        int64_t predictionTime;           // When prediction was made
        int64_t targetTime;               // When prediction is for
        int horizonMinutes;
        std::map<int, double> predictedVehicleCounts;
        std::map<int, double> predictedQueueLengths;
    };
    std::vector<PendingPrediction> pendingPredictions_;
    std::vector<PredictionAccuracy> accuracyHistory_;
    mutable std::mutex accuracyMutex_;
    static constexpr size_t MAX_ACCURACY_HISTORY = 100;

    // Statistics
    std::atomic<int> totalRuns_{0};
    std::atomic<int> successfulRuns_{0};
    std::atomic<double> averageImprovement_{0.0};

    // Validation
    std::unique_ptr<validation::TimingValidator> validator_;
    validation::ValidationConfig validationConfig_;
    std::atomic<bool> validationEnabled_{true};

    // Validation helper
    validation::ValidationResult performValidation(
        const std::vector<simulator::Road>& network,
        const simulator::Chromosome& chromosome);
};

} // namespace api
} // namespace ratms

#endif // PREDICTIVE_OPTIMIZER_H
