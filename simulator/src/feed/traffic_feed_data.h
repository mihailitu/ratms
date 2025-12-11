#ifndef TRAFFIC_FEED_DATA_H
#define TRAFFIC_FEED_DATA_H

#include "../core/defs.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace simulator {

/**
 * @brief Single entry in a traffic feed snapshot - expected state for one road
 */
struct TrafficFeedEntry {
    int64_t timestamp;              // Unix timestamp
    roadID roadId;                  // Road identifier
    int expectedVehicleCount;       // Expected number of vehicles on this road
    double expectedAvgSpeed;        // Expected average speed (m/s), -1 if unknown
    double confidence;              // Confidence level 0.0-1.0 (for ML predictions)

    TrafficFeedEntry()
        : timestamp(0), roadId(0), expectedVehicleCount(0),
          expectedAvgSpeed(-1.0), confidence(1.0) {}

    TrafficFeedEntry(int64_t ts, roadID rid, int count, double speed = -1.0, double conf = 1.0)
        : timestamp(ts), roadId(rid), expectedVehicleCount(count),
          expectedAvgSpeed(speed), confidence(conf) {}
};

/**
 * @brief Complete snapshot of expected traffic state across all roads
 */
struct TrafficFeedSnapshot {
    int64_t timestamp;                      // When this snapshot was generated
    std::vector<TrafficFeedEntry> entries;  // Expected state per road
    std::string source;                     // "simulated", "external", "ml_predicted"

    TrafficFeedSnapshot() : timestamp(0), source("unknown") {}
};

/**
 * @brief Callback type for feed subscribers
 */
using FeedCallback = std::function<void(const TrafficFeedSnapshot&)>;

} // namespace simulator

#endif // TRAFFIC_FEED_DATA_H
