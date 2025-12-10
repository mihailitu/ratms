#ifndef TRAFFIC_PATTERN_STORAGE_H
#define TRAFFIC_PATTERN_STORAGE_H

#include "database_manager.h"
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace ratms {
namespace data {

/**
 * @brief Traffic snapshot from a single point in time
 */
struct TrafficSnapshot {
    int64_t timestamp;      // Unix timestamp
    int roadId;
    int vehicleCount;
    double queueLength;
    double avgSpeed;
    double flowRate;        // vehicles per minute
};

/**
 * @brief Aggregated traffic pattern for a specific time slot
 */
struct TrafficPattern {
    int id;
    int roadId;
    int dayOfWeek;          // 0=Sunday, 6=Saturday
    int timeSlot;           // 0-47 (30-min slots, 0 = 00:00-00:30)
    double avgVehicleCount;
    double avgQueueLength;
    double avgSpeed;
    double avgFlowRate;
    double minVehicleCount;
    double maxVehicleCount;
    double stddevVehicleCount;
    int sampleCount;
    int64_t lastUpdated;
};

/**
 * @brief Road metrics snapshot for recording
 */
struct RoadMetrics {
    int roadId;
    int vehicleCount;
    double queueLength;
    double avgSpeed;
    double flowRate;
};

/**
 * @brief Configuration for traffic pattern storage
 */
struct TrafficPatternConfig {
    int snapshotIntervalSeconds = 60;       // How often to record snapshots
    int snapshotRetentionDays = 7;          // How long to keep raw snapshots
    int minSamplesForPattern = 3;           // Minimum samples before pattern is valid
    double outlierThreshold = 3.0;          // Standard deviations for outlier detection
};

/**
 * @brief Storage and aggregation for traffic patterns
 *
 * Records traffic snapshots periodically and aggregates them into
 * time-of-day patterns for predictive optimization.
 */
class TrafficPatternStorage {
public:
    explicit TrafficPatternStorage(std::shared_ptr<DatabaseManager> db);
    ~TrafficPatternStorage() = default;

    // Configuration
    void setConfig(const TrafficPatternConfig& config);
    TrafficPatternConfig getConfig() const;

    // Recording snapshots
    bool recordSnapshot(const RoadMetrics& metrics);
    bool recordSnapshotBatch(const std::vector<RoadMetrics>& metrics);
    bool recordSnapshot(const std::map<int, RoadMetrics>& roadMetrics);

    // Querying snapshots
    std::vector<TrafficSnapshot> getSnapshots(int hours = 24);
    std::vector<TrafficSnapshot> getSnapshots(int roadId, int hours = 24);
    std::vector<TrafficSnapshot> getSnapshotsRange(int64_t startTime, int64_t endTime);

    // Querying patterns
    std::vector<TrafficPattern> getAllPatterns();
    std::vector<TrafficPattern> getPatterns(int dayOfWeek, int timeSlot);
    TrafficPattern getPattern(int roadId, int dayOfWeek, int timeSlot);
    std::vector<TrafficPattern> getPatternsForTime(std::chrono::system_clock::time_point time);
    std::vector<TrafficPattern> getPatternsForRoad(int roadId);

    // Pattern aggregation
    bool aggregateSnapshots();
    bool aggregateSnapshotsForTimeSlot(int dayOfWeek, int timeSlot);

    // Maintenance
    int pruneOldSnapshots();
    int pruneOldSnapshots(int days);

    // Utility
    static int getTimeSlot(std::chrono::system_clock::time_point time);
    static int getDayOfWeek(std::chrono::system_clock::time_point time);
    static std::pair<int, int> getCurrentDayAndSlot();
    static std::string timeSlotToString(int timeSlot);  // e.g., "08:00-08:30"

private:
    std::shared_ptr<DatabaseManager> db_;
    TrafficPatternConfig config_;
    mutable std::mutex mutex_;

    // Internal helpers
    bool insertSnapshot(const TrafficSnapshot& snapshot);
    bool updatePattern(const TrafficPattern& pattern);
    int64_t getCurrentTimestamp() const;
};

} // namespace data
} // namespace ratms

#endif // TRAFFIC_PATTERN_STORAGE_H
