#ifndef I_TRAFFIC_DATA_FEED_H
#define I_TRAFFIC_DATA_FEED_H

#include "traffic_feed_data.h"

#include <string>

namespace simulator {

/**
 * @brief Interface for pluggable traffic data feeds
 *
 * Implementations can provide traffic density expectations from various sources:
 * - SimulatedTrafficFeed: generates data from historical patterns or estimates
 * - ExternalTrafficFeed: receives real data from external sensors/APIs (future)
 * - MLPredictedFeed: uses ML models to predict traffic (future)
 *
 * The feed runs continuously in a background thread, pushing updates to
 * subscribers at configurable intervals. Designed for 24/7 operation.
 */
class ITrafficDataFeed {
public:
    virtual ~ITrafficDataFeed() = default;

    // ========== Lifecycle ==========

    /**
     * @brief Start the continuous feed
     * Begins the background thread that generates/receives traffic data
     */
    virtual void start() = 0;

    /**
     * @brief Stop the continuous feed
     * Stops the background thread and cleans up resources
     */
    virtual void stop() = 0;

    /**
     * @brief Check if the feed is currently running
     */
    virtual bool isRunning() const = 0;

    // ========== Subscription ==========

    /**
     * @brief Subscribe to continuous feed updates
     * @param callback Function to call when new data arrives
     *
     * Multiple subscribers can be registered. Callbacks are invoked
     * synchronously from the feed thread.
     */
    virtual void subscribe(FeedCallback callback) = 0;

    // ========== Query ==========

    /**
     * @brief Get the most recent snapshot
     * @return Latest traffic feed snapshot (may be empty if feed not started)
     *
     * Useful for status queries without subscribing to continuous updates.
     */
    virtual TrafficFeedSnapshot getLatestSnapshot() const = 0;

    // ========== Identification ==========

    /**
     * @brief Get the name/type of this feed source
     * @return Source identifier (e.g., "simulated", "external", "ml_predicted")
     */
    virtual std::string getSourceName() const = 0;

    /**
     * @brief Check if the feed is healthy and producing valid data
     * @return true if feed is operational, false if there are issues
     *
     * For external feeds, this can indicate connection status.
     * Default implementation returns true.
     */
    virtual bool isHealthy() const { return true; }

    // ========== Configuration ==========

    /**
     * @brief Set the interval between feed updates
     * @param intervalMs Interval in milliseconds
     */
    virtual void setUpdateIntervalMs(int intervalMs) = 0;

    /**
     * @brief Get the current update interval
     * @return Interval in milliseconds
     */
    virtual int getUpdateIntervalMs() const = 0;
};

} // namespace simulator

#endif // I_TRAFFIC_DATA_FEED_H
