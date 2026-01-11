#include "traffic_predictor.h"
#include "../utils/logger.h"
#include <cmath>
#include <ctime>
#include <set>
#include <algorithm>
#include <numeric>

namespace ratms {
namespace prediction {

TrafficPredictor::TrafficPredictor(
    std::shared_ptr<data::TrafficPatternStorage> patternStorage,
    std::shared_ptr<simulator::Simulator> simulator,
    std::mutex& simMutex
) : patternStorage_(patternStorage),
    simulator_(simulator),
    simMutex_(simMutex)
{
    LOG_INFO(LogComponent::Core, "TrafficPredictor initialized");
}

void TrafficPredictor::setConfig(const PredictionConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_ = config;

    // Ensure weights sum to 1.0
    double total = config_.patternWeight + config_.currentWeight;
    if (total > 0.0 && std::abs(total - 1.0) > 0.001) {
        config_.patternWeight /= total;
        config_.currentWeight /= total;
    }

    // Clear cache when config changes
    clearCache();

    LOG_INFO(LogComponent::Core, "TrafficPredictor config updated: horizon={}min, patternWeight={:.2f}, currentWeight={:.2f}",
             config_.horizonMinutes, config_.patternWeight, config_.currentWeight);
}

PredictionConfig TrafficPredictor::getConfig() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_;
}

PredictionResult TrafficPredictor::predictCurrent() {
    return predictForecast(0);
}

PredictionResult TrafficPredictor::predictForecast(int horizonMinutes) {
    // Validate horizon
    PredictionConfig config;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        config = config_;
    }

    if (horizonMinutes < 0) {
        horizonMinutes = 0;
    }
    if (horizonMinutes > config.maxHorizonMinutes) {
        horizonMinutes = config.maxHorizonMinutes;
    }

    // Check cache
    if (isCacheValid(horizonMinutes)) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = predictionCache_.find(horizonMinutes);
        if (it != predictionCache_.end()) {
            LOG_DEBUG(LogComponent::Core, "Returning cached prediction for horizon={}min", horizonMinutes);
            return it->second.result;
        }
    }

    // Calculate target time slot
    auto [targetDay, targetSlot] = getFutureTimeSlot(horizonMinutes);

    // Gather data
    auto currentStates = getCurrentState();
    auto patterns = getPatterns(targetDay, targetSlot);

    // Generate predictions for each road
    std::vector<PredictedMetrics> predictions;
    predictions.reserve(std::max(currentStates.size(), patterns.size()));

    // Collect all road IDs from both sources
    std::set<int> allRoadIds;
    for (const auto& [roadId, _] : currentStates) {
        allRoadIds.insert(roadId);
    }
    for (const auto& [roadId, _] : patterns) {
        allRoadIds.insert(roadId);
    }

    // Generate prediction for each road
    for (int roadId : allRoadIds) {
        std::optional<data::TrafficPattern> pattern;
        std::optional<CurrentRoadState> current;

        auto patternIt = patterns.find(roadId);
        if (patternIt != patterns.end()) {
            pattern = patternIt->second;
        }

        auto currentIt = currentStates.find(roadId);
        if (currentIt != currentStates.end()) {
            current = currentIt->second;
        }

        auto predicted = predictForRoad(roadId, pattern, current, targetDay, targetSlot);
        predictions.push_back(predicted);
    }

    // Calculate average confidence
    double avgConfidence = 0.0;
    if (!predictions.empty()) {
        double totalConfidence = std::accumulate(
            predictions.begin(), predictions.end(), 0.0,
            [](double sum, const PredictedMetrics& pm) { return sum + pm.confidence; }
        );
        avgConfidence = totalConfidence / predictions.size();
    }

    // Build result
    auto now = std::chrono::system_clock::now();
    auto nowUnix = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    auto targetUnix = nowUnix + (horizonMinutes * 60);

    PredictionResult result;
    result.predictionTimestamp = nowUnix;
    result.targetTimestamp = targetUnix;
    result.horizonMinutes = horizonMinutes;
    result.targetDayOfWeek = targetDay;
    result.targetTimeSlot = targetSlot;
    result.targetTimeSlotString = timeSlotToString(targetSlot);
    result.roadPredictions = std::move(predictions);
    result.averageConfidence = avgConfidence;
    result.configUsed = config;

    // Cache result
    updateCache(horizonMinutes, result);

    LOG_DEBUG(LogComponent::Core, "Generated prediction for horizon={}min: {} roads, avgConfidence={:.2f}",
              horizonMinutes, result.roadPredictions.size(), avgConfidence);

    return result;
}

std::optional<PredictedMetrics> TrafficPredictor::predictRoad(int roadId, int horizonMinutes) {
    auto result = predictForecast(horizonMinutes);

    for (const auto& prediction : result.roadPredictions) {
        if (prediction.roadId == roadId) {
            return prediction;
        }
    }

    return std::nullopt;
}

std::pair<int, int> TrafficPredictor::getFutureTimeSlot(int horizonMinutes) {
    auto now = std::chrono::system_clock::now();
    auto future = now + std::chrono::minutes(horizonMinutes);

    std::time_t futureTime = std::chrono::system_clock::to_time_t(future);
    std::tm* tm = std::localtime(&futureTime);

    int dayOfWeek = tm->tm_wday;  // 0 = Sunday
    int hour = tm->tm_hour;
    int minute = tm->tm_min;
    int timeSlot = hour * 2 + (minute / 30);  // 0-47

    return {dayOfWeek, timeSlot};
}

double TrafficPredictor::calculateConfidence(int sampleCount, double stddev,
                                             double avgValue, int minSamples) {
    // Sample count factor: [0, 1] based on how many samples we have
    double sampleFactor = std::min(1.0, static_cast<double>(sampleCount) / minSamples);

    // Variability factor: lower stddev = higher confidence
    double variabilityFactor = 1.0;
    if (avgValue > 0.01) {
        double coefficientOfVariation = stddev / avgValue;
        variabilityFactor = 1.0 - std::min(coefficientOfVariation, 1.0);
    } else if (sampleCount > 0) {
        // If avgValue is very low, use moderate confidence
        variabilityFactor = 0.5;
    }

    // Combined confidence
    double confidence = sampleFactor * variabilityFactor;

    return std::clamp(confidence, 0.0, 1.0);
}

std::string TrafficPredictor::timeSlotToString(int timeSlot) {
    if (timeSlot < 0 || timeSlot > 47) {
        return "Invalid";
    }

    int startHour = timeSlot / 2;
    int startMinute = (timeSlot % 2) * 30;
    int endHour = (timeSlot + 1) / 2;
    int endMinute = ((timeSlot + 1) % 2) * 30;

    if (endHour == 24) {
        endHour = 0;
    }

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d-%02d:%02d",
                  startHour, startMinute, endHour, endMinute);
    return std::string(buffer);
}

PredictedMetrics TrafficPredictor::predictForRoad(
    int roadId,
    const std::optional<data::TrafficPattern>& pattern,
    const std::optional<CurrentRoadState>& currentState,
    int targetDayOfWeek,
    int targetTimeSlot
) {
    PredictionConfig config;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        config = config_;
    }

    PredictedMetrics metrics;
    metrics.roadId = roadId;
    metrics.predictionDayOfWeek = targetDayOfWeek;
    metrics.predictionTimeSlot = targetTimeSlot;
    metrics.hasHistoricalPattern = pattern.has_value();
    metrics.hasCurrentData = currentState.has_value();

    // Extract values from pattern and current state
    double patternVehicleCount = 0.0;
    double patternQueueLength = 0.0;
    double patternAvgSpeed = 0.0;
    double patternFlowRate = 0.0;
    double patternStddev = 0.0;
    int patternSampleCount = 0;

    if (pattern.has_value()) {
        patternVehicleCount = pattern->avgVehicleCount;
        patternQueueLength = pattern->avgQueueLength;
        patternAvgSpeed = pattern->avgSpeed;
        patternFlowRate = pattern->avgFlowRate;
        patternStddev = pattern->stddevVehicleCount;
        patternSampleCount = pattern->sampleCount;
    }

    double currentVehicleCount = 0.0;
    double currentQueueLength = 0.0;
    double currentAvgSpeed = 0.0;
    double currentFlowRate = 0.0;

    if (currentState.has_value()) {
        currentVehicleCount = currentState->vehicleCount;
        currentQueueLength = currentState->queueLength;
        currentAvgSpeed = currentState->avgSpeed;
        currentFlowRate = currentState->flowRate;
    }

    // Store component values for transparency
    metrics.patternVehicleCount = patternVehicleCount;
    metrics.currentVehicleCount = currentVehicleCount;
    metrics.historicalSampleCount = patternSampleCount;

    // Blend values
    metrics.vehicleCount = blendValue(patternVehicleCount, currentVehicleCount,
                                       metrics.hasHistoricalPattern, metrics.hasCurrentData);
    metrics.queueLength = blendValue(patternQueueLength, currentQueueLength,
                                      metrics.hasHistoricalPattern, metrics.hasCurrentData);
    metrics.avgSpeed = blendValue(patternAvgSpeed, currentAvgSpeed,
                                   metrics.hasHistoricalPattern, metrics.hasCurrentData);
    metrics.flowRate = blendValue(patternFlowRate, currentFlowRate,
                                   metrics.hasHistoricalPattern, metrics.hasCurrentData);

    // Calculate confidence
    if (metrics.hasHistoricalPattern) {
        metrics.confidence = calculateConfidence(
            patternSampleCount, patternStddev, patternVehicleCount,
            config.minSamplesForFullConfidence
        );
    } else if (metrics.hasCurrentData) {
        // No pattern, just current data - low confidence
        metrics.confidence = 0.1;
    } else {
        // No data at all
        metrics.confidence = 0.0;
    }

    return metrics;
}

std::map<int, CurrentRoadState> TrafficPredictor::getCurrentState() {
    std::map<int, CurrentRoadState> states;

    if (!simulator_) {
        return states;
    }

    std::lock_guard<std::mutex> lock(simMutex_);

    for (const auto& [roadId, road] : simulator_->cityMap) {
        CurrentRoadState state;
        state.roadId = roadId;
        state.vehicleCount = road.getVehicleCount();

        // Calculate queue length and average speed
        double totalSpeed = 0.0;
        int vehicleCount = 0;
        double queueLength = 0.0;

        double roadLength = road.getLength();
        double queueThreshold = roadLength - 50.0;  // Within 50m of end

        for (unsigned lane = 0; lane < road.getLanesNo(); ++lane) {
            for (const auto& vehicle : road.getVehicles()[lane]) {
                totalSpeed += vehicle.getVelocity();
                vehicleCount++;

                // Count vehicles in queue (near end of road, low speed)
                if (vehicle.getPos() >= queueThreshold && vehicle.getVelocity() < 2.0) {
                    queueLength += 1.0;
                }
            }
        }

        state.avgSpeed = vehicleCount > 0 ? totalSpeed / vehicleCount : 0.0;
        state.queueLength = queueLength;
        state.flowRate = 0.0;  // TODO: Calculate from recent vehicle exits

        states[roadId] = state;
    }

    return states;
}

std::map<int, data::TrafficPattern> TrafficPredictor::getPatterns(int dayOfWeek, int timeSlot) {
    std::map<int, data::TrafficPattern> patternMap;

    if (!patternStorage_) {
        return patternMap;
    }

    auto patterns = patternStorage_->getPatterns(dayOfWeek, timeSlot);

    for (const auto& pattern : patterns) {
        patternMap[pattern.roadId] = pattern;
    }

    return patternMap;
}

double TrafficPredictor::blendValue(double patternValue, double currentValue,
                                    bool hasPattern, bool hasCurrent) {
    PredictionConfig config;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        config = config_;
    }

    if (hasPattern && hasCurrent) {
        // Normal case: blend both sources
        return config.patternWeight * patternValue + config.currentWeight * currentValue;
    } else if (hasPattern && !hasCurrent) {
        // No current data: use pattern only
        return patternValue;
    } else if (!hasPattern && hasCurrent) {
        // No pattern: use current only
        return currentValue;
    } else {
        // Neither available
        return 0.0;
    }
}

bool TrafficPredictor::isCacheValid(int horizonMinutes) const {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    auto it = predictionCache_.find(horizonMinutes);
    if (it == predictionCache_.end()) {
        return false;
    }

    PredictionConfig config;
    {
        std::lock_guard<std::mutex> lock2(configMutex_);
        config = config_;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - it->second.timestamp).count();

    return elapsed < config.cacheDurationSeconds;
}

void TrafficPredictor::updateCache(int horizonMinutes, const PredictionResult& result) {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    CacheEntry entry;
    entry.result = result;
    entry.timestamp = std::chrono::steady_clock::now();

    predictionCache_[horizonMinutes] = entry;
}

void TrafficPredictor::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    predictionCache_.clear();
}

} // namespace prediction
} // namespace ratms
