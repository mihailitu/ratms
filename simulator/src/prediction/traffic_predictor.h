#ifndef TRAFFIC_PREDICTOR_H
#define TRAFFIC_PREDICTOR_H

#include "../data/storage/traffic_pattern_storage.h"
#include "../core/simulator.h"
#include <memory>
#include <mutex>
#include <optional>
#include <chrono>
#include <map>
#include <vector>

namespace ratms {
namespace prediction {

/**
 * @brief Configuration for traffic prediction
 */
struct PredictionConfig {
    int horizonMinutes = 30;              // Default prediction horizon (10-120)
    int minHorizonMinutes = 10;           // Minimum allowed horizon
    int maxHorizonMinutes = 120;          // Maximum allowed horizon
    double patternWeight = 0.7;           // Historical pattern weight (0.0-1.0)
    double currentWeight = 0.3;           // Current state weight (0.0-1.0)
    int minSamplesForFullConfidence = 10; // Sample count for max confidence
    int cacheDurationSeconds = 30;        // How long to cache predictions
};

/**
 * @brief Current state of a road from the live simulation
 */
struct CurrentRoadState {
    int roadId;
    int vehicleCount;
    double queueLength;
    double avgSpeed;
    double flowRate;
};

/**
 * @brief Predicted metrics for a single road
 */
struct PredictedMetrics {
    int roadId;
    int predictionDayOfWeek;              // Day of week for prediction (0-6)
    int predictionTimeSlot;               // Time slot for prediction (0-47)

    // Predicted values (blended)
    double vehicleCount;
    double queueLength;
    double avgSpeed;
    double flowRate;

    // Confidence and metadata
    double confidence;                    // 0.0-1.0 reliability score
    int historicalSampleCount;            // Samples used from pattern
    bool hasCurrentData;                  // Whether current state was available
    bool hasHistoricalPattern;            // Whether pattern existed for time slot

    // Component breakdown for transparency
    double patternVehicleCount;           // Value from historical pattern
    double currentVehicleCount;           // Value from current state
};

/**
 * @brief Complete prediction result for all roads
 */
struct PredictionResult {
    int64_t predictionTimestamp;          // When prediction was generated (Unix timestamp)
    int64_t targetTimestamp;              // Timestamp being predicted for
    int horizonMinutes;                   // Horizon used for this prediction
    int targetDayOfWeek;                  // Day of week predicted (0-6)
    int targetTimeSlot;                   // Time slot predicted (0-47)
    std::string targetTimeSlotString;     // Human-readable time slot (e.g., "08:00-08:30")

    std::vector<PredictedMetrics> roadPredictions;
    double averageConfidence;             // Mean confidence across all roads
    PredictionConfig configUsed;          // Config snapshot used for prediction
};

/**
 * @brief TrafficPredictor - Predicts future traffic state by blending patterns with current state
 *
 * This class blends historical traffic patterns (stored by day-of-week and time-slot)
 * with the current simulation state to predict traffic metrics T+N minutes ahead.
 *
 * Key features:
 * - Configurable prediction horizon (10-120 minutes)
 * - Weighted blending of historical patterns and current state
 * - Confidence scoring based on sample count and variability
 * - Lightweight caching to prevent redundant computation
 */
class TrafficPredictor {
public:
    TrafficPredictor(
        std::shared_ptr<data::TrafficPatternStorage> patternStorage,
        std::shared_ptr<simulator::Simulator> simulator,
        std::mutex& simMutex
    );
    ~TrafficPredictor() = default;

    // Configuration
    void setConfig(const PredictionConfig& config);
    PredictionConfig getConfig() const;

    // Core prediction methods
    PredictionResult predictCurrent();
    PredictionResult predictForecast(int horizonMinutes);
    std::optional<PredictedMetrics> predictRoad(int roadId, int horizonMinutes);

    // Utility methods
    static std::pair<int, int> getFutureTimeSlot(int horizonMinutes);
    static double calculateConfidence(int sampleCount, double stddev,
                                      double avgValue, int minSamples);
    static std::string timeSlotToString(int timeSlot);

private:
    // Core prediction logic
    PredictedMetrics predictForRoad(
        int roadId,
        const std::optional<data::TrafficPattern>& pattern,
        const std::optional<CurrentRoadState>& currentState,
        int targetDayOfWeek,
        int targetTimeSlot
    );

    // Data gathering
    std::map<int, CurrentRoadState> getCurrentState();
    std::map<int, data::TrafficPattern> getPatterns(int dayOfWeek, int timeSlot);

    // Blending logic
    double blendValue(double patternValue, double currentValue,
                      bool hasPattern, bool hasCurrent);

    // Cache management
    bool isCacheValid(int horizonMinutes) const;
    void updateCache(int horizonMinutes, const PredictionResult& result);
    void clearCache();

    std::shared_ptr<data::TrafficPatternStorage> patternStorage_;
    std::shared_ptr<simulator::Simulator> simulator_;
    std::mutex& simMutex_;

    PredictionConfig config_;
    mutable std::mutex configMutex_;

    // Prediction cache
    struct CacheEntry {
        PredictionResult result;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::map<int, CacheEntry> predictionCache_;  // horizonMinutes -> cached result
    mutable std::mutex cacheMutex_;
};

} // namespace prediction
} // namespace ratms

#endif // TRAFFIC_PREDICTOR_H
