#ifndef TRAVEL_TIME_COLLECTOR_H
#define TRAVEL_TIME_COLLECTOR_H

#include "../data/storage/database_manager.h"
#include "../core/simulator.h"
#include <memory>
#include <map>
#include <set>
#include <chrono>
#include <mutex>
#include <vector>

namespace ratms {
namespace metrics {

/**
 * @brief ODPair - Origin-Destination pair definition
 */
struct ODPair {
    int id;
    int originRoadId;
    int destinationRoadId;
    std::string name;
    std::string description;
};

/**
 * @brief TrackedVehicle - Vehicle being tracked for travel time
 */
struct TrackedVehicle {
    int vehicleId;
    int odPairId;
    int originRoadId;
    int destinationRoadId;
    std::chrono::steady_clock::time_point startTime;
    bool hasReachedDestination = false;
};

/**
 * @brief TravelTimeSample - Individual travel time measurement
 */
struct TravelTimeSample {
    int odPairId;
    int vehicleId;
    double travelTimeSeconds;
    long startTime;
    long endTime;
};

/**
 * @brief TravelTimeStats - Aggregated statistics for an O-D pair
 */
struct TravelTimeStats {
    int odPairId;
    double avgTime = 0.0;
    double minTime = 0.0;
    double maxTime = 0.0;
    double p50Time = 0.0;  // Median
    double p95Time = 0.0;  // 95th percentile
    int sampleCount = 0;
};

/**
 * @brief TravelTimeCollector - Tracks vehicle travel times between O-D pairs
 *
 * This collector monitors designated origin-destination pairs and records
 * travel times as vehicles traverse from origin to destination roads.
 */
class TravelTimeCollector {
public:
    TravelTimeCollector(std::shared_ptr<data::DatabaseManager> dbManager);
    ~TravelTimeCollector() = default;

    // O-D pair management
    int addODPair(int originRoadId, int destinationRoadId,
                  const std::string& name = "", const std::string& description = "");
    void removeODPair(int odPairId);
    std::vector<ODPair> getAllODPairs() const;
    ODPair getODPair(int odPairId) const;

    // Called each simulation step to track vehicles
    void update(const simulator::Simulator::CityMap& cityMap, double dt);

    // Get current tracked vehicles
    std::vector<TrackedVehicle> getTrackedVehicles() const;

    // Get statistics
    TravelTimeStats getStats(int odPairId) const;
    std::vector<TravelTimeStats> getAllStats() const;

    // Get recent samples
    std::vector<TravelTimeSample> getRecentSamples(int odPairId, int limit = 100) const;

    // Clear all tracking data
    void reset();

    // Save pending samples to database
    void flush();

private:
    // Start tracking a vehicle that just entered an origin road
    void startTracking(int vehicleId, int odPairId, int originRoadId, int destinationRoadId);

    // Record a completed journey
    void recordCompletion(const TrackedVehicle& vehicle);

    // Update aggregated statistics for an O-D pair
    void updateStats(int odPairId);

    // Find O-D pairs that include the given road as origin
    std::vector<int> findODPairsWithOrigin(int roadId) const;

    // Check if vehicle is on a destination road
    bool isVehicleOnDestination(int vehicleId, int destinationRoadId,
                                 const simulator::Simulator::CityMap& cityMap) const;

    std::shared_ptr<data::DatabaseManager> dbManager_;

    // O-D pairs indexed by ID
    std::map<int, ODPair> odPairs_;

    // Origin road to O-D pair ID mapping (for quick lookup when vehicle enters origin)
    std::multimap<int, int> originToODPairs_;

    // Currently tracked vehicles indexed by vehicle ID
    std::map<int, TrackedVehicle> trackedVehicles_;

    // Vehicles on each road (for efficient lookup)
    std::set<int> vehiclesSeenThisStep_;

    // Pending samples to be saved
    std::vector<TravelTimeSample> pendingSamples_;

    // In-memory stats cache
    std::map<int, TravelTimeStats> statsCache_;

    // Thread safety
    mutable std::mutex mutex_;

    // Next O-D pair ID (in-memory)
    int nextODPairId_ = 1;
};

} // namespace metrics
} // namespace ratms

#endif // TRAVEL_TIME_COLLECTOR_H
