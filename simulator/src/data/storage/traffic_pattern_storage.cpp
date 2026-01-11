#include "traffic_pattern_storage.h"
#include "../../utils/logger.h"
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ratms {
namespace data {

using namespace ratms;

TrafficPatternStorage::TrafficPatternStorage(std::shared_ptr<DatabaseManager> db)
    : db_(std::move(db)) {
    LOG_INFO(LogComponent::Database, "TrafficPatternStorage initialized");
}

void TrafficPatternStorage::setConfig(const TrafficPatternConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    LOG_DEBUG(LogComponent::Database, "TrafficPatternStorage config updated: interval={}s, retention={}d",
              config_.snapshotIntervalSeconds, config_.snapshotRetentionDays);
}

TrafficPatternConfig TrafficPatternStorage::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

int64_t TrafficPatternStorage::getCurrentTimestamp() const {
    return std::time(nullptr);
}

// Recording snapshots
bool TrafficPatternStorage::recordSnapshot(const RoadMetrics& metrics) {
    DatabaseManager::TrafficSnapshotRecord record;
    record.timestamp = getCurrentTimestamp();
    record.road_id = metrics.roadId;
    record.vehicle_count = metrics.vehicleCount;
    record.queue_length = metrics.queueLength;
    record.avg_speed = metrics.avgSpeed;
    record.flow_rate = metrics.flowRate;

    std::lock_guard<std::mutex> lock(mutex_);
    return db_->insertTrafficSnapshot(record);
}

bool TrafficPatternStorage::recordSnapshotBatch(const std::vector<RoadMetrics>& metrics) {
    if (metrics.empty()) return true;

    int64_t timestamp = getCurrentTimestamp();
    std::vector<DatabaseManager::TrafficSnapshotRecord> records;
    records.reserve(metrics.size());

    for (const auto& m : metrics) {
        DatabaseManager::TrafficSnapshotRecord record;
        record.timestamp = timestamp;
        record.road_id = m.roadId;
        record.vehicle_count = m.vehicleCount;
        record.queue_length = m.queueLength;
        record.avg_speed = m.avgSpeed;
        record.flow_rate = m.flowRate;
        records.push_back(record);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    bool result = db_->insertTrafficSnapshotsBatch(records);

    if (result) {
        LOG_DEBUG(LogComponent::Database, "Recorded {} traffic snapshots", metrics.size());
    }

    return result;
}

bool TrafficPatternStorage::recordSnapshot(const std::map<int, RoadMetrics>& roadMetrics) {
    std::vector<RoadMetrics> metrics;
    metrics.reserve(roadMetrics.size());
    for (const auto& [roadId, m] : roadMetrics) {
        metrics.push_back(m);
    }
    return recordSnapshotBatch(metrics);
}

bool TrafficPatternStorage::insertSnapshot(const TrafficSnapshot& snapshot) {
    DatabaseManager::TrafficSnapshotRecord record;
    record.timestamp = snapshot.timestamp;
    record.road_id = snapshot.roadId;
    record.vehicle_count = snapshot.vehicleCount;
    record.queue_length = snapshot.queueLength;
    record.avg_speed = snapshot.avgSpeed;
    record.flow_rate = snapshot.flowRate;

    return db_->insertTrafficSnapshot(record);
}

// Querying snapshots
std::vector<TrafficSnapshot> TrafficPatternStorage::getSnapshots(int hours) {
    int64_t cutoff = getCurrentTimestamp() - (hours * 3600);
    return getSnapshotsRange(cutoff, getCurrentTimestamp());
}

std::vector<TrafficSnapshot> TrafficPatternStorage::getSnapshots(int roadId, int hours) {
    int64_t cutoff = getCurrentTimestamp() - (hours * 3600);

    std::lock_guard<std::mutex> lock(mutex_);
    auto dbRecords = db_->getTrafficSnapshotsForRoad(roadId, cutoff);

    std::vector<TrafficSnapshot> result;
    result.reserve(dbRecords.size());

    for (const auto& r : dbRecords) {
        TrafficSnapshot s;
        s.timestamp = r.timestamp;
        s.roadId = r.road_id;
        s.vehicleCount = r.vehicle_count;
        s.queueLength = r.queue_length;
        s.avgSpeed = r.avg_speed;
        s.flowRate = r.flow_rate;
        result.push_back(s);
    }

    return result;
}

std::vector<TrafficSnapshot> TrafficPatternStorage::getSnapshotsRange(int64_t startTime, int64_t endTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto dbRecords = db_->getTrafficSnapshotsRange(startTime, endTime);

    std::vector<TrafficSnapshot> result;
    result.reserve(dbRecords.size());

    for (const auto& r : dbRecords) {
        TrafficSnapshot s;
        s.timestamp = r.timestamp;
        s.roadId = r.road_id;
        s.vehicleCount = r.vehicle_count;
        s.queueLength = r.queue_length;
        s.avgSpeed = r.avg_speed;
        s.flowRate = r.flow_rate;
        result.push_back(s);
    }

    return result;
}

// Querying patterns
std::vector<TrafficPattern> TrafficPatternStorage::getAllPatterns() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto dbRecords = db_->getAllTrafficPatterns();

    std::vector<TrafficPattern> result;
    result.reserve(dbRecords.size());

    for (const auto& r : dbRecords) {
        TrafficPattern p;
        p.id = r.id;
        p.roadId = r.road_id;
        p.dayOfWeek = r.day_of_week;
        p.timeSlot = r.time_slot;
        p.avgVehicleCount = r.avg_vehicle_count;
        p.avgQueueLength = r.avg_queue_length;
        p.avgSpeed = r.avg_speed;
        p.avgFlowRate = r.avg_flow_rate;
        p.minVehicleCount = r.min_vehicle_count;
        p.maxVehicleCount = r.max_vehicle_count;
        p.stddevVehicleCount = r.stddev_vehicle_count;
        p.sampleCount = r.sample_count;
        p.lastUpdated = r.last_updated;
        result.push_back(p);
    }

    return result;
}

std::vector<TrafficPattern> TrafficPatternStorage::getPatterns(int dayOfWeek, int timeSlot) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto dbRecords = db_->getTrafficPatterns(dayOfWeek, timeSlot);

    std::vector<TrafficPattern> result;
    result.reserve(dbRecords.size());

    for (const auto& r : dbRecords) {
        TrafficPattern p;
        p.id = r.id;
        p.roadId = r.road_id;
        p.dayOfWeek = r.day_of_week;
        p.timeSlot = r.time_slot;
        p.avgVehicleCount = r.avg_vehicle_count;
        p.avgQueueLength = r.avg_queue_length;
        p.avgSpeed = r.avg_speed;
        p.avgFlowRate = r.avg_flow_rate;
        p.minVehicleCount = r.min_vehicle_count;
        p.maxVehicleCount = r.max_vehicle_count;
        p.stddevVehicleCount = r.stddev_vehicle_count;
        p.sampleCount = r.sample_count;
        p.lastUpdated = r.last_updated;
        result.push_back(p);
    }

    return result;
}

TrafficPattern TrafficPatternStorage::getPattern(int roadId, int dayOfWeek, int timeSlot) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto r = db_->getTrafficPattern(roadId, dayOfWeek, timeSlot);

    TrafficPattern p;
    p.id = r.id;
    p.roadId = r.road_id;
    p.dayOfWeek = r.day_of_week;
    p.timeSlot = r.time_slot;
    p.avgVehicleCount = r.avg_vehicle_count;
    p.avgQueueLength = r.avg_queue_length;
    p.avgSpeed = r.avg_speed;
    p.avgFlowRate = r.avg_flow_rate;
    p.minVehicleCount = r.min_vehicle_count;
    p.maxVehicleCount = r.max_vehicle_count;
    p.stddevVehicleCount = r.stddev_vehicle_count;
    p.sampleCount = r.sample_count;
    p.lastUpdated = r.last_updated;

    return p;
}

std::vector<TrafficPattern> TrafficPatternStorage::getPatternsForTime(
    std::chrono::system_clock::time_point time) {
    int dayOfWeek = getDayOfWeek(time);
    int timeSlot = getTimeSlot(time);
    return getPatterns(dayOfWeek, timeSlot);
}

std::vector<TrafficPattern> TrafficPatternStorage::getPatternsForRoad(int roadId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto dbRecords = db_->getTrafficPatternsForRoad(roadId);

    std::vector<TrafficPattern> result;
    result.reserve(dbRecords.size());

    for (const auto& r : dbRecords) {
        TrafficPattern p;
        p.id = r.id;
        p.roadId = r.road_id;
        p.dayOfWeek = r.day_of_week;
        p.timeSlot = r.time_slot;
        p.avgVehicleCount = r.avg_vehicle_count;
        p.avgQueueLength = r.avg_queue_length;
        p.avgSpeed = r.avg_speed;
        p.avgFlowRate = r.avg_flow_rate;
        p.minVehicleCount = r.min_vehicle_count;
        p.maxVehicleCount = r.max_vehicle_count;
        p.stddevVehicleCount = r.stddev_vehicle_count;
        p.sampleCount = r.sample_count;
        p.lastUpdated = r.last_updated;
        result.push_back(p);
    }

    return result;
}

// Pattern aggregation
bool TrafficPatternStorage::aggregateSnapshots() {
    LOG_INFO(LogComponent::Database, "Starting snapshot aggregation");

    // Get all snapshots from the last retention period
    int64_t cutoff = getCurrentTimestamp() - (config_.snapshotRetentionDays * 24 * 3600);

    std::lock_guard<std::mutex> lock(mutex_);
    auto snapshots = db_->getTrafficSnapshots(cutoff);

    if (snapshots.empty()) {
        LOG_INFO(LogComponent::Database, "No snapshots to aggregate");
        return true;
    }

    // Group snapshots by (road_id, day_of_week, time_slot)
    struct AggKey {
        int roadId;
        int dayOfWeek;
        int timeSlot;

        bool operator<(const AggKey& other) const {
            if (roadId != other.roadId) return roadId < other.roadId;
            if (dayOfWeek != other.dayOfWeek) return dayOfWeek < other.dayOfWeek;
            return timeSlot < other.timeSlot;
        }
    };

    std::map<AggKey, std::vector<DatabaseManager::TrafficSnapshotRecord>> grouped;

    for (const auto& snapshot : snapshots) {
        // Convert timestamp to day of week and time slot
        std::time_t t = snapshot.timestamp;
        std::tm* tm = std::localtime(&t);

        AggKey key;
        key.roadId = snapshot.road_id;
        key.dayOfWeek = tm->tm_wday;
        key.timeSlot = tm->tm_hour * 2 + (tm->tm_min / 30);

        grouped[key].push_back(snapshot);
    }

    // Aggregate each group
    int patternsUpdated = 0;
    for (const auto& [key, records] : grouped) {
        if (static_cast<int>(records.size()) < config_.minSamplesForPattern) {
            continue;  // Skip if not enough samples
        }

        // Calculate statistics
        double sumVehicles = 0, sumQueue = 0, sumSpeed = 0, sumFlow = 0;
        double minVehicles = std::numeric_limits<double>::max();
        double maxVehicles = 0;
        std::vector<double> vehicleCounts;
        vehicleCounts.reserve(records.size());

        for (const auto& r : records) {
            sumVehicles += r.vehicle_count;
            sumQueue += r.queue_length;
            sumSpeed += r.avg_speed;
            sumFlow += r.flow_rate;
            minVehicles = std::min(minVehicles, static_cast<double>(r.vehicle_count));
            maxVehicles = std::max(maxVehicles, static_cast<double>(r.vehicle_count));
            vehicleCounts.push_back(r.vehicle_count);
        }

        double n = static_cast<double>(records.size());
        double avgVehicles = sumVehicles / n;
        double avgQueue = sumQueue / n;
        double avgSpeed = sumSpeed / n;
        double avgFlow = sumFlow / n;

        // Calculate standard deviation
        double sumSqDiff = 0;
        for (double v : vehicleCounts) {
            sumSqDiff += (v - avgVehicles) * (v - avgVehicles);
        }
        double stddev = std::sqrt(sumSqDiff / n);

        // Create pattern record
        DatabaseManager::TrafficPatternRecord pattern;
        pattern.road_id = key.roadId;
        pattern.day_of_week = key.dayOfWeek;
        pattern.time_slot = key.timeSlot;
        pattern.avg_vehicle_count = avgVehicles;
        pattern.avg_queue_length = avgQueue;
        pattern.avg_speed = avgSpeed;
        pattern.avg_flow_rate = avgFlow;
        pattern.min_vehicle_count = minVehicles;
        pattern.max_vehicle_count = maxVehicles;
        pattern.stddev_vehicle_count = stddev;
        pattern.sample_count = static_cast<int>(records.size());
        pattern.last_updated = getCurrentTimestamp();

        if (db_->insertOrUpdateTrafficPattern(pattern)) {
            patternsUpdated++;
        }
    }

    LOG_INFO(LogComponent::Database, "Snapshot aggregation completed: {} patterns updated", patternsUpdated);
    return true;
}

bool TrafficPatternStorage::aggregateSnapshotsForTimeSlot(int dayOfWeek, int timeSlot) {
    // This method aggregates snapshots for a specific time slot
    // Useful for incremental updates

    LOG_DEBUG(LogComponent::Database, "Aggregating snapshots for day={}, slot={}", dayOfWeek, timeSlot);

    // For now, delegate to full aggregation
    // A more efficient implementation would filter snapshots by day/slot first
    return aggregateSnapshots();
}

bool TrafficPatternStorage::updatePattern(const TrafficPattern& pattern) {
    DatabaseManager::TrafficPatternRecord record;
    record.road_id = pattern.roadId;
    record.day_of_week = pattern.dayOfWeek;
    record.time_slot = pattern.timeSlot;
    record.avg_vehicle_count = pattern.avgVehicleCount;
    record.avg_queue_length = pattern.avgQueueLength;
    record.avg_speed = pattern.avgSpeed;
    record.avg_flow_rate = pattern.avgFlowRate;
    record.min_vehicle_count = pattern.minVehicleCount;
    record.max_vehicle_count = pattern.maxVehicleCount;
    record.stddev_vehicle_count = pattern.stddevVehicleCount;
    record.sample_count = pattern.sampleCount;
    record.last_updated = pattern.lastUpdated;

    return db_->insertOrUpdateTrafficPattern(record);
}

// Maintenance
int TrafficPatternStorage::pruneOldSnapshots() {
    return pruneOldSnapshots(config_.snapshotRetentionDays);
}

int TrafficPatternStorage::pruneOldSnapshots(int days) {
    int64_t cutoff = getCurrentTimestamp() - (days * 24 * 3600);

    LOG_INFO(LogComponent::Database, "Pruning snapshots older than {} days (before timestamp {})",
             days, cutoff);

    std::lock_guard<std::mutex> lock(mutex_);
    return db_->deleteTrafficSnapshotsBefore(cutoff);
}

// Utility functions
int TrafficPatternStorage::getTimeSlot(std::chrono::system_clock::time_point time) {
    auto time_t_val = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&time_t_val);

    // Each slot is 30 minutes
    // slot = hour * 2 + (minute / 30)
    return tm->tm_hour * 2 + (tm->tm_min / 30);
}

int TrafficPatternStorage::getDayOfWeek(std::chrono::system_clock::time_point time) {
    auto time_t_val = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&time_t_val);
    return tm->tm_wday;  // 0=Sunday, 6=Saturday
}

std::pair<int, int> TrafficPatternStorage::getCurrentDayAndSlot() {
    auto now = std::chrono::system_clock::now();
    return {getDayOfWeek(now), getTimeSlot(now)};
}

std::string TrafficPatternStorage::timeSlotToString(int timeSlot) {
    if (timeSlot < 0 || timeSlot > 47) {
        return "invalid";
    }

    int startHour = timeSlot / 2;
    int startMin = (timeSlot % 2) * 30;
    int endHour = startHour;
    int endMin = startMin + 30;

    if (endMin >= 60) {
        endMin = 0;
        endHour = (endHour + 1) % 24;
    }

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << startHour << ":"
        << std::setfill('0') << std::setw(2) << startMin << "-"
        << std::setfill('0') << std::setw(2) << endHour << ":"
        << std::setfill('0') << std::setw(2) << endMin;

    return oss.str();
}

} // namespace data
} // namespace ratms
