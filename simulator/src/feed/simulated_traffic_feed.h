#ifndef SIMULATED_TRAFFIC_FEED_H
#define SIMULATED_TRAFFIC_FEED_H

#include "i_traffic_data_feed.h"
#include "../core/road.h"
#include "../data/storage/traffic_pattern_storage.h"

#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace simulator {

/**
 * @brief Simulated traffic feed that generates expected traffic density
 *
 * Generates traffic expectations based on:
 * 1. Historical patterns from traffic_patterns table (if available)
 * 2. Estimates based on road capacity (fallback)
 *
 * Runs continuously in a background thread, pushing updates to subscribers.
 */
class SimulatedTrafficFeed : public ITrafficDataFeed {
public:
    /**
     * @brief Construct a simulated traffic feed
     * @param patternStorage Reference to traffic pattern storage for historical data
     * @param cityMap Reference to the road network
     */
    SimulatedTrafficFeed(ratms::data::TrafficPatternStorage& patternStorage,
                         const std::map<roadID, Road>& cityMap);

    ~SimulatedTrafficFeed() override;

    // Prevent copying
    SimulatedTrafficFeed(const SimulatedTrafficFeed&) = delete;
    SimulatedTrafficFeed& operator=(const SimulatedTrafficFeed&) = delete;

    // ========== Lifecycle ==========
    void start() override;
    void stop() override;
    bool isRunning() const override { return running_.load(); }

    // ========== Subscription ==========
    void subscribe(FeedCallback callback) override;

    // ========== Query ==========
    TrafficFeedSnapshot getLatestSnapshot() const override;
    std::string getSourceName() const override { return "simulated"; }
    bool isHealthy() const override { return running_.load(); }

    // ========== Configuration ==========
    void setUpdateIntervalMs(int intervalMs) override;
    int getUpdateIntervalMs() const override { return updateIntervalMs_.load(); }

private:
    ratms::data::TrafficPatternStorage& patternStorage_;
    const std::map<roadID, Road>& cityMap_;

    std::atomic<bool> running_{false};
    std::thread feedThread_;
    std::atomic<int> updateIntervalMs_{1000};  // Default 1 second

    mutable std::mutex snapshotMutex_;
    TrafficFeedSnapshot latestSnapshot_;

    mutable std::mutex subscribersMutex_;
    std::vector<FeedCallback> subscribers_;

    /**
     * @brief Main feed loop running in background thread
     */
    void feedLoop();

    /**
     * @brief Generate a new traffic snapshot based on current time
     * @return Snapshot with expected vehicle counts for all roads
     */
    TrafficFeedSnapshot generateSnapshot();

    /**
     * @brief Estimate vehicle count for a road when no pattern exists
     * @param road Reference to the road
     * @return Estimated vehicle count based on road capacity
     */
    int estimateDefaultCount(const Road& road) const;

    /**
     * @brief Notify all subscribers of a new snapshot
     * @param snapshot The new snapshot to broadcast
     */
    void notifySubscribers(const TrafficFeedSnapshot& snapshot);
};

} // namespace simulator

#endif // SIMULATED_TRAFFIC_FEED_H
