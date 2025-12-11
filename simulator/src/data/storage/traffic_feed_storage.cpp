#include "traffic_feed_storage.h"
#include "../../utils/logger.h"

#include <chrono>
#include <sstream>

using namespace ratms;

namespace ratms {
namespace data {

TrafficFeedStorage::TrafficFeedStorage(std::shared_ptr<DatabaseManager> db)
    : db_(std::move(db))
{
    LOG_INFO(LogComponent::Database, "TrafficFeedStorage initialized");
}

void TrafficFeedStorage::setConfig(const TrafficFeedStorageConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

TrafficFeedStorageConfig TrafficFeedStorage::getConfig() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

bool TrafficFeedStorage::recordFeedSnapshot(const simulator::TrafficFeedSnapshot& snapshot)
{
    std::lock_guard<std::mutex> lock(mutex_);

    int successCount = 0;
    for (const auto& entry : snapshot.entries) {
        if (insertEntry(entry, snapshot.source)) {
            successCount++;
        }
    }

    LOG_TRACE(LogComponent::Database, "Recorded {}/{} feed entries from source '{}'",
              successCount, snapshot.entries.size(), snapshot.source);

    return successCount == static_cast<int>(snapshot.entries.size());
}

bool TrafficFeedStorage::recordFeedEntry(const simulator::TrafficFeedEntry& entry, const std::string& source)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return insertEntry(entry, source);
}

bool TrafficFeedStorage::insertEntry(const simulator::TrafficFeedEntry& entry, const std::string& source)
{
    const char* sql = R"(
        INSERT INTO traffic_feed_entries
        (timestamp, road_id, expected_vehicle_count, expected_avg_speed, confidence, source, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_->getConnection(), sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        LOG_ERROR(LogComponent::Database, "Failed to prepare feed entry insert: {}",
                  sqlite3_errmsg(db_->getConnection()));
        return false;
    }

    int64_t now = getCurrentTimestamp();

    sqlite3_bind_int64(stmt, 1, entry.timestamp);
    sqlite3_bind_int(stmt, 2, static_cast<int>(entry.roadId));
    sqlite3_bind_int(stmt, 3, entry.expectedVehicleCount);
    sqlite3_bind_double(stmt, 4, entry.expectedAvgSpeed);
    sqlite3_bind_double(stmt, 5, entry.confidence);
    sqlite3_bind_text(stmt, 6, source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, now);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR(LogComponent::Database, "Failed to insert feed entry: {}",
                  sqlite3_errmsg(db_->getConnection()));
        return false;
    }

    return true;
}

std::vector<simulator::TrafficFeedEntry> TrafficFeedStorage::getEntries(int64_t startTime, int64_t endTime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<simulator::TrafficFeedEntry> entries;

    const char* sql = R"(
        SELECT timestamp, road_id, expected_vehicle_count, expected_avg_speed, confidence
        FROM traffic_feed_entries
        WHERE timestamp >= ? AND timestamp <= ?
        ORDER BY timestamp ASC
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_->getConnection(), sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        LOG_ERROR(LogComponent::Database, "Failed to prepare feed entries query: {}",
                  sqlite3_errmsg(db_->getConnection()));
        return entries;
    }

    sqlite3_bind_int64(stmt, 1, startTime);
    sqlite3_bind_int64(stmt, 2, endTime);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        simulator::TrafficFeedEntry entry;
        entry.timestamp = sqlite3_column_int64(stmt, 0);
        entry.roadId = static_cast<simulator::roadID>(sqlite3_column_int(stmt, 1));
        entry.expectedVehicleCount = sqlite3_column_int(stmt, 2);
        entry.expectedAvgSpeed = sqlite3_column_double(stmt, 3);
        entry.confidence = sqlite3_column_double(stmt, 4);
        entries.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return entries;
}

std::vector<simulator::TrafficFeedEntry> TrafficFeedStorage::getEntriesForRoad(
    int roadId, int64_t startTime, int64_t endTime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<simulator::TrafficFeedEntry> entries;

    const char* sql = R"(
        SELECT timestamp, road_id, expected_vehicle_count, expected_avg_speed, confidence
        FROM traffic_feed_entries
        WHERE road_id = ? AND timestamp >= ? AND timestamp <= ?
        ORDER BY timestamp ASC
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_->getConnection(), sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        LOG_ERROR(LogComponent::Database, "Failed to prepare feed entries query: {}",
                  sqlite3_errmsg(db_->getConnection()));
        return entries;
    }

    sqlite3_bind_int(stmt, 1, roadId);
    sqlite3_bind_int64(stmt, 2, startTime);
    sqlite3_bind_int64(stmt, 3, endTime);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        simulator::TrafficFeedEntry entry;
        entry.timestamp = sqlite3_column_int64(stmt, 0);
        entry.roadId = static_cast<simulator::roadID>(sqlite3_column_int(stmt, 1));
        entry.expectedVehicleCount = sqlite3_column_int(stmt, 2);
        entry.expectedAvgSpeed = sqlite3_column_double(stmt, 3);
        entry.confidence = sqlite3_column_double(stmt, 4);
        entries.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return entries;
}

int TrafficFeedStorage::getEntryCount(int64_t startTime, int64_t endTime)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string sql = "SELECT COUNT(*) FROM traffic_feed_entries";
    if (startTime > 0 && endTime > 0) {
        sql += " WHERE timestamp >= ? AND timestamp <= ?";
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_->getConnection(), sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        LOG_ERROR(LogComponent::Database, "Failed to prepare count query: {}",
                  sqlite3_errmsg(db_->getConnection()));
        return -1;
    }

    if (startTime > 0 && endTime > 0) {
        sqlite3_bind_int64(stmt, 1, startTime);
        sqlite3_bind_int64(stmt, 2, endTime);
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

std::string TrafficFeedStorage::exportToCSV(int64_t startTime, int64_t endTime)
{
    auto entries = getEntries(startTime, endTime);

    std::ostringstream csv;
    csv << "timestamp,road_id,expected_vehicle_count,expected_avg_speed,confidence\n";

    for (const auto& entry : entries) {
        csv << entry.timestamp << ","
            << entry.roadId << ","
            << entry.expectedVehicleCount << ","
            << entry.expectedAvgSpeed << ","
            << entry.confidence << "\n";
    }

    return csv.str();
}

std::string TrafficFeedStorage::exportToJSON(int64_t startTime, int64_t endTime)
{
    auto entries = getEntries(startTime, endTime);

    std::ostringstream json;
    json << "[\n";

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        json << "  {"
             << "\"timestamp\":" << entry.timestamp << ","
             << "\"road_id\":" << entry.roadId << ","
             << "\"expected_vehicle_count\":" << entry.expectedVehicleCount << ","
             << "\"expected_avg_speed\":" << entry.expectedAvgSpeed << ","
             << "\"confidence\":" << entry.confidence
             << "}";
        if (i < entries.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "]";
    return json.str();
}

int TrafficFeedStorage::pruneOldEntries()
{
    return pruneOldEntries(config_.retentionDays);
}

int TrafficFeedStorage::pruneOldEntries(int days)
{
    std::lock_guard<std::mutex> lock(mutex_);

    int64_t cutoff = getCurrentTimestamp() - (days * 24 * 60 * 60);

    const char* sql = "DELETE FROM traffic_feed_entries WHERE timestamp < ?";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_->getConnection(), sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        LOG_ERROR(LogComponent::Database, "Failed to prepare prune query: {}",
                  sqlite3_errmsg(db_->getConnection()));
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, cutoff);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR(LogComponent::Database, "Failed to prune feed entries: {}",
                  sqlite3_errmsg(db_->getConnection()));
        return -1;
    }

    int deleted = sqlite3_changes(db_->getConnection());
    LOG_INFO(LogComponent::Database, "Pruned {} feed entries older than {} days", deleted, days);

    return deleted;
}

TrafficFeedStorage::FeedStats TrafficFeedStorage::getStats()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FeedStats stats = {};

    // Total entries
    {
        const char* sql = "SELECT COUNT(*) FROM traffic_feed_entries";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_->getConnection(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.totalEntries = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Unique roads
    {
        const char* sql = "SELECT COUNT(DISTINCT road_id) FROM traffic_feed_entries";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_->getConnection(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.uniqueRoads = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Timestamps
    {
        const char* sql = "SELECT MIN(timestamp), MAX(timestamp) FROM traffic_feed_entries";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_->getConnection(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.oldestTimestamp = sqlite3_column_int64(stmt, 0);
                stats.newestTimestamp = sqlite3_column_int64(stmt, 1);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Entries by source
    {
        const char* sql = "SELECT source, COUNT(*) FROM traffic_feed_entries GROUP BY source";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_->getConnection(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                int count = sqlite3_column_int(stmt, 1);
                if (source) {
                    stats.entriesBySource[source] = count;
                }
            }
            sqlite3_finalize(stmt);
        }
    }

    return stats;
}

int64_t TrafficFeedStorage::getCurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

} // namespace data
} // namespace ratms
