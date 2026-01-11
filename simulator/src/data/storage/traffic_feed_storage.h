#ifndef TRAFFIC_FEED_STORAGE_H
#define TRAFFIC_FEED_STORAGE_H

#include "database_manager.h"
#include "../../feed/traffic_feed_data.h"

#include <memory>
#include <mutex>
#include <vector>

namespace ratms {
namespace data {

/**
 * @brief Configuration for traffic feed storage
 */
struct TrafficFeedStorageConfig {
    int retentionDays = 30;         // How long to keep feed data
    int batchSize = 100;            // Batch size for bulk inserts
    bool asyncWrite = false;        // Write asynchronously (future)
};

/**
 * @brief Storage for traffic feed data for ML training
 *
 * Persists all incoming traffic feed snapshots to the database.
 * This data is used to train ML models for traffic prediction.
 */
class TrafficFeedStorage {
public:
    explicit TrafficFeedStorage(std::shared_ptr<DatabaseManager> db);
    ~TrafficFeedStorage() = default;

    // Configuration
    void setConfig(const TrafficFeedStorageConfig& config);
    TrafficFeedStorageConfig getConfig() const;

    // Recording feed data
    bool recordFeedSnapshot(const simulator::TrafficFeedSnapshot& snapshot);
    bool recordFeedEntry(const simulator::TrafficFeedEntry& entry, const std::string& source);

    // Querying feed data
    std::vector<simulator::TrafficFeedEntry> getEntries(int64_t startTime, int64_t endTime);
    std::vector<simulator::TrafficFeedEntry> getEntriesForRoad(int roadId, int64_t startTime, int64_t endTime);
    int getEntryCount(int64_t startTime = 0, int64_t endTime = 0);

    // Export for ML training
    std::string exportToCSV(int64_t startTime, int64_t endTime);
    std::string exportToJSON(int64_t startTime, int64_t endTime);

    // Maintenance
    int pruneOldEntries();
    int pruneOldEntries(int days);

    // Statistics
    struct FeedStats {
        int totalEntries;
        int uniqueRoads;
        int64_t oldestTimestamp;
        int64_t newestTimestamp;
        std::map<std::string, int> entriesBySource;
    };
    FeedStats getStats();

private:
    std::shared_ptr<DatabaseManager> db_;
    TrafficFeedStorageConfig config_;
    mutable std::mutex mutex_;

    // Internal helpers
    bool insertEntry(const simulator::TrafficFeedEntry& entry, const std::string& source);
    int64_t getCurrentTimestamp() const;
};

} // namespace data
} // namespace ratms

#endif // TRAFFIC_FEED_STORAGE_H
