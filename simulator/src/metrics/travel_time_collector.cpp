#include "travel_time_collector.h"
#include "../utils/logger.h"
#include <algorithm>
#include <numeric>

namespace ratms {
namespace metrics {

TravelTimeCollector::TravelTimeCollector(std::shared_ptr<data::DatabaseManager> dbManager)
    : dbManager_(dbManager) {
    LOG_INFO(LogComponent::Core, "TravelTimeCollector initialized");
}

int TravelTimeCollector::addODPair(int originRoadId, int destinationRoadId,
                                    const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if this pair already exists
    for (const auto& [id, pair] : odPairs_) {
        if (pair.originRoadId == originRoadId && pair.destinationRoadId == destinationRoadId) {
            return id;  // Return existing ID
        }
    }

    // Create new O-D pair
    ODPair pair;
    pair.id = nextODPairId_++;
    pair.originRoadId = originRoadId;
    pair.destinationRoadId = destinationRoadId;
    pair.name = name.empty() ? "Road " + std::to_string(originRoadId) + " -> " +
                               std::to_string(destinationRoadId) : name;
    pair.description = description;

    odPairs_[pair.id] = pair;
    originToODPairs_.insert({originRoadId, pair.id});

    // Initialize stats
    statsCache_[pair.id] = TravelTimeStats{pair.id};

    LOG_INFO(LogComponent::Core, "Added O-D pair {}: {} -> {} ({})",
             pair.id, originRoadId, destinationRoadId, pair.name);

    return pair.id;
}

void TravelTimeCollector::removeODPair(int odPairId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = odPairs_.find(odPairId);
    if (it == odPairs_.end()) return;

    int originRoadId = it->second.originRoadId;
    odPairs_.erase(it);

    // Remove from origin mapping
    auto range = originToODPairs_.equal_range(originRoadId);
    for (auto mapIt = range.first; mapIt != range.second; ) {
        if (mapIt->second == odPairId) {
            mapIt = originToODPairs_.erase(mapIt);
        } else {
            ++mapIt;
        }
    }

    // Remove associated tracked vehicles
    for (auto trackIt = trackedVehicles_.begin(); trackIt != trackedVehicles_.end(); ) {
        if (trackIt->second.odPairId == odPairId) {
            trackIt = trackedVehicles_.erase(trackIt);
        } else {
            ++trackIt;
        }
    }

    // Remove stats
    statsCache_.erase(odPairId);

    LOG_INFO(LogComponent::Core, "Removed O-D pair {}", odPairId);
}

std::vector<ODPair> TravelTimeCollector::getAllODPairs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ODPair> result;
    for (const auto& [id, pair] : odPairs_) {
        result.push_back(pair);
    }
    return result;
}

ODPair TravelTimeCollector::getODPair(int odPairId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = odPairs_.find(odPairId);
    if (it != odPairs_.end()) {
        return it->second;
    }
    return ODPair{};
}

void TravelTimeCollector::update(const simulator::Simulator::CityMap& cityMap, double dt) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (odPairs_.empty()) return;

    vehiclesSeenThisStep_.clear();

    // Scan all vehicles in all roads
    for (const auto& [roadId, road] : cityMap) {
        const auto& vehicles = road.getVehicles();

        for (unsigned lane = 0; lane < vehicles.size(); ++lane) {
            for (const auto& vehicle : vehicles[lane]) {
                int vehicleId = vehicle.getId();
                vehiclesSeenThisStep_.insert(vehicleId);

                // Check if this vehicle is already being tracked
                auto trackedIt = trackedVehicles_.find(vehicleId);
                if (trackedIt != trackedVehicles_.end()) {
                    // Check if vehicle has reached destination
                    if (roadId == trackedIt->second.destinationRoadId) {
                        trackedIt->second.hasReachedDestination = true;
                        recordCompletion(trackedIt->second);
                        trackedVehicles_.erase(trackedIt);
                    }
                } else {
                    // Check if vehicle is on an origin road and should be tracked
                    auto odPairIds = findODPairsWithOrigin(roadId);
                    for (int odPairId : odPairIds) {
                        const auto& pair = odPairs_[odPairId];
                        // Start tracking this vehicle
                        startTracking(vehicleId, odPairId, pair.originRoadId, pair.destinationRoadId);
                    }
                }
            }
        }
    }

    // Clean up vehicles that have left the network without reaching destination
    for (auto it = trackedVehicles_.begin(); it != trackedVehicles_.end(); ) {
        if (vehiclesSeenThisStep_.find(it->first) == vehiclesSeenThisStep_.end()) {
            LOG_DEBUG(LogComponent::Core, "Vehicle {} left network before reaching destination", it->first);
            it = trackedVehicles_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<TrackedVehicle> TravelTimeCollector::getTrackedVehicles() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackedVehicle> result;
    for (const auto& [id, vehicle] : trackedVehicles_) {
        result.push_back(vehicle);
    }
    return result;
}

TravelTimeStats TravelTimeCollector::getStats(int odPairId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = statsCache_.find(odPairId);
    if (it != statsCache_.end()) {
        return it->second;
    }
    return TravelTimeStats{odPairId};
}

std::vector<TravelTimeStats> TravelTimeCollector::getAllStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TravelTimeStats> result;
    for (const auto& [id, stats] : statsCache_) {
        result.push_back(stats);
    }
    return result;
}

std::vector<TravelTimeSample> TravelTimeCollector::getRecentSamples(int odPairId, int limit) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<TravelTimeSample> result;

    // Return from pending samples first
    for (auto it = pendingSamples_.rbegin(); it != pendingSamples_.rend() && result.size() < static_cast<size_t>(limit); ++it) {
        if (it->odPairId == odPairId) {
            result.push_back(*it);
        }
    }

    return result;
}

void TravelTimeCollector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    trackedVehicles_.clear();
    pendingSamples_.clear();
    vehiclesSeenThisStep_.clear();

    // Reset stats
    for (auto& [id, stats] : statsCache_) {
        stats = TravelTimeStats{id};
    }

    LOG_INFO(LogComponent::Core, "TravelTimeCollector reset");
}

void TravelTimeCollector::flush() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (pendingSamples_.empty()) return;

    // TODO: Batch insert to database when DatabaseManager supports travel time tables
    // For now, just update in-memory stats

    for (const auto& sample : pendingSamples_) {
        LOG_DEBUG(LogComponent::Database, "Would save travel time sample: O-D {} vehicle {} time {:.2f}s",
                  sample.odPairId, sample.vehicleId, sample.travelTimeSeconds);
    }

    pendingSamples_.clear();
}

void TravelTimeCollector::startTracking(int vehicleId, int odPairId,
                                         int originRoadId, int destinationRoadId) {
    // Don't re-track a vehicle we're already tracking for this O-D pair
    auto it = trackedVehicles_.find(vehicleId);
    if (it != trackedVehicles_.end() && it->second.odPairId == odPairId) {
        return;
    }

    TrackedVehicle tracked;
    tracked.vehicleId = vehicleId;
    tracked.odPairId = odPairId;
    tracked.originRoadId = originRoadId;
    tracked.destinationRoadId = destinationRoadId;
    tracked.startTime = std::chrono::steady_clock::now();
    tracked.hasReachedDestination = false;

    trackedVehicles_[vehicleId] = tracked;

    LOG_TRACE(LogComponent::Core, "Started tracking vehicle {} for O-D pair {}", vehicleId, odPairId);
}

void TravelTimeCollector::recordCompletion(const TrackedVehicle& vehicle) {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - vehicle.startTime);
    double travelTimeSeconds = duration.count() / 1000.0;

    TravelTimeSample sample;
    sample.odPairId = vehicle.odPairId;
    sample.vehicleId = vehicle.vehicleId;
    sample.travelTimeSeconds = travelTimeSeconds;
    sample.startTime = std::chrono::duration_cast<std::chrono::seconds>(
        vehicle.startTime.time_since_epoch()).count();
    sample.endTime = std::chrono::duration_cast<std::chrono::seconds>(
        endTime.time_since_epoch()).count();

    pendingSamples_.push_back(sample);

    LOG_DEBUG(LogComponent::Core, "Vehicle {} completed O-D {} in {:.2f}s",
              vehicle.vehicleId, vehicle.odPairId, travelTimeSeconds);

    // Update statistics
    updateStats(vehicle.odPairId);
}

void TravelTimeCollector::updateStats(int odPairId) {
    // Collect all samples for this O-D pair
    std::vector<double> times;
    for (const auto& sample : pendingSamples_) {
        if (sample.odPairId == odPairId) {
            times.push_back(sample.travelTimeSeconds);
        }
    }

    if (times.empty()) return;

    // Sort for percentile calculations
    std::sort(times.begin(), times.end());

    TravelTimeStats& stats = statsCache_[odPairId];

    // Update stats with new samples
    stats.sampleCount += times.size();
    stats.minTime = times.front();
    stats.maxTime = times.back();

    // Calculate average
    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    stats.avgTime = sum / times.size();

    // Calculate median (p50)
    size_t n = times.size();
    if (n % 2 == 0) {
        stats.p50Time = (times[n/2 - 1] + times[n/2]) / 2.0;
    } else {
        stats.p50Time = times[n/2];
    }

    // Calculate p95
    size_t p95Index = static_cast<size_t>(n * 0.95);
    if (p95Index >= n) p95Index = n - 1;
    stats.p95Time = times[p95Index];

    LOG_DEBUG(LogComponent::Core, "Updated stats for O-D {}: avg={:.2f}s min={:.2f}s max={:.2f}s p95={:.2f}s samples={}",
              odPairId, stats.avgTime, stats.minTime, stats.maxTime, stats.p95Time, stats.sampleCount);
}

std::vector<int> TravelTimeCollector::findODPairsWithOrigin(int roadId) const {
    std::vector<int> result;
    auto range = originToODPairs_.equal_range(roadId);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

bool TravelTimeCollector::isVehicleOnDestination(int vehicleId, int destinationRoadId,
                                                  const simulator::Simulator::CityMap& cityMap) const {
    auto roadIt = cityMap.find(destinationRoadId);
    if (roadIt == cityMap.end()) return false;

    const auto& vehicles = roadIt->second.getVehicles();
    for (const auto& lane : vehicles) {
        for (const auto& vehicle : lane) {
            if (vehicle.getId() == vehicleId) {
                return true;
            }
        }
    }
    return false;
}

} // namespace metrics
} // namespace ratms
