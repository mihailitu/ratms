#include "simulated_traffic_feed.h"
#include "../utils/logger.h"

#include <chrono>
#include <random>
#include <unordered_map>

using namespace ratms;

namespace simulator {

SimulatedTrafficFeed::SimulatedTrafficFeed(
    ratms::data::TrafficPatternStorage &patternStorage,
    const std::map<roadID, Road> &cityMap)
    : patternStorage_(patternStorage), cityMap_(cityMap) {
  LOG_INFO(LogComponent::Simulation,
           "SimulatedTrafficFeed created for {} roads", cityMap_.size());
}

SimulatedTrafficFeed::~SimulatedTrafficFeed() { stop(); }

void SimulatedTrafficFeed::start() {
  if (running_.load()) {
    LOG_WARN(LogComponent::Simulation, "SimulatedTrafficFeed already running");
    return;
  }

  running_.store(true);
  feedThread_ = std::thread(&SimulatedTrafficFeed::feedLoop, this);

  LOG_INFO(LogComponent::Simulation,
           "SimulatedTrafficFeed started with {}ms interval",
           updateIntervalMs_.load());
}

void SimulatedTrafficFeed::stop() {
  if (!running_.load()) {
    return;
  }

  // Signal shutdown and wake up sleeping thread
  {
    std::lock_guard<std::mutex> lock(shutdownMutex_);
    running_.store(false);
  }
  shutdownCv_.notify_all();

  // Wait for thread to finish
  if (feedThread_.joinable()) {
    feedThread_.join();
  }

  LOG_INFO(LogComponent::Simulation, "SimulatedTrafficFeed stopped");
}

void SimulatedTrafficFeed::subscribe(FeedCallback callback) {
  std::lock_guard<std::mutex> lock(subscribersMutex_);
  subscribers_.push_back(std::move(callback));
  LOG_DEBUG(LogComponent::Simulation, "New subscriber added, total: {}",
            subscribers_.size());
}

TrafficFeedSnapshot SimulatedTrafficFeed::getLatestSnapshot() const {
  std::lock_guard<std::mutex> lock(snapshotMutex_);
  return latestSnapshot_;
}

void SimulatedTrafficFeed::setUpdateIntervalMs(int intervalMs) {
  if (intervalMs < 100) {
    LOG_WARN(LogComponent::Simulation,
             "Update interval too low ({}ms), setting to 100ms", intervalMs);
    intervalMs = 100;
  }
  updateIntervalMs_.store(intervalMs);
  LOG_INFO(LogComponent::Simulation, "Feed update interval set to {}ms",
           intervalMs);
}

void SimulatedTrafficFeed::feedLoop() {
  LOG_DEBUG(LogComponent::Simulation, "Feed loop started");

  while (running_.load()) {
    auto snapshot = generateSnapshot();

    // Check if we should stop (generateSnapshot may take time for large maps)
    if (!running_.load()) {
      break;
    }

    // Update latest snapshot
    {
      std::lock_guard<std::mutex> lock(snapshotMutex_);
      latestSnapshot_ = snapshot;
    }

    // Notify subscribers
    notifySubscribers(snapshot);

    // Wait for configured interval or shutdown signal
    std::unique_lock<std::mutex> lock(shutdownMutex_);
    shutdownCv_.wait_for(lock,
                         std::chrono::milliseconds(updateIntervalMs_.load()),
                         [this] { return !running_.load(); });
  }

  LOG_DEBUG(LogComponent::Simulation, "Feed loop ended");
}

TrafficFeedSnapshot SimulatedTrafficFeed::generateSnapshot() {
  auto now = std::chrono::system_clock::now();
  auto timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();

  TrafficFeedSnapshot snapshot;
  snapshot.timestamp = timestamp;
  snapshot.source = "simulated";

  // Get current time slot for pattern lookup
  auto [dayOfWeek, timeSlot] =
      ratms::data::TrafficPatternStorage::getCurrentDayAndSlot();

  // Random generator for adding variation
  static std::random_device rd;
  static std::mt19937 gen(rd());

  // Batch load all patterns for current time slot (much faster than per-road
  // queries)
  auto allPatterns = patternStorage_.getPatterns(dayOfWeek, timeSlot);
  std::unordered_map<int, ratms::data::TrafficPattern> patternMap;
  patternMap.reserve(allPatterns.size());
  for (auto &p : allPatterns) {
    patternMap[p.roadId] = std::move(p);
  }

  for (const auto &[roadId, road] : cityMap_) {
    // Early exit if stopping (snapshot will be discarded anyway)
    if (!running_.load(std::memory_order_relaxed)) {
      break;
    }

    // Only process roads with traffic lights (the ones being optimized)
    // This dramatically reduces iterations for large maps (150K -> few
    // thousand)
    if (road.getTrafficLights().empty()) {
      continue;
    }

    TrafficFeedEntry entry;
    entry.timestamp = timestamp;
    entry.roadId = roadId;
    entry.confidence = 1.0;

    // Look up pattern from pre-loaded map
    auto it = patternMap.find(static_cast<int>(roadId));
    ratms::data::TrafficPattern pattern = {};
    if (it != patternMap.end()) {
      pattern = it->second;
    }

    if (pattern.sampleCount > 0) {
      // Use historical pattern with some variation
      double avgCount = pattern.avgVehicleCount;
      double stddev = pattern.stddevVehicleCount > 0
                          ? pattern.stddevVehicleCount
                          : avgCount * 0.1;

      std::normal_distribution<double> dist(avgCount,
                                            stddev * 0.3); // Reduced variation
      entry.expectedVehicleCount =
          std::max(0, static_cast<int>(std::round(dist(gen))));
      entry.expectedAvgSpeed = pattern.avgSpeed;
      entry.confidence = std::min(
          1.0, pattern.sampleCount / 10.0); // More samples = more confidence
    } else {
      // Estimate based on road capacity
      entry.expectedVehicleCount = estimateDefaultCount(road);
      entry.expectedAvgSpeed =
          road.getMaxSpeed() * 0.7; // Assume 70% of max speed
      entry.confidence = 0.5;       // Low confidence for estimates
    }

    snapshot.entries.push_back(entry);
  }

  LOG_TRACE(LogComponent::Simulation,
            "Generated snapshot with {} entries at slot {}/{}",
            snapshot.entries.size(), dayOfWeek, timeSlot);

  return snapshot;
}

int SimulatedTrafficFeed::estimateDefaultCount(const Road &road) const {
  // Estimate based on road capacity
  // Assume average vehicle spacing of 20m (vehicle length + safe gap)
  const double avgVehicleSpacing = 20.0;
  double roadLength = road.getLength();
  unsigned lanes = road.getLanesNo();

  // Capacity per lane
  int capacityPerLane = static_cast<int>(roadLength / avgVehicleSpacing);

  // Assume 30-50% utilization for normal traffic
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<double> utilDist(0.3, 0.5);

  double utilization = utilDist(gen);
  int estimatedCount = static_cast<int>(capacityPerLane * lanes * utilization);

  return std::max(1, estimatedCount); // At least 1 vehicle
}

void SimulatedTrafficFeed::notifySubscribers(
    const TrafficFeedSnapshot &snapshot) {
  std::lock_guard<std::mutex> lock(subscribersMutex_);

  for (const auto &callback : subscribers_) {
    try {
      callback(snapshot);
    } catch (const std::exception &e) {
      LOG_ERROR(LogComponent::Simulation,
                "Subscriber callback threw exception: {}", e.what());
    }
  }
}

} // namespace simulator
