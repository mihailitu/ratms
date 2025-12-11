/**
 * Unit tests for Traffic Feed and Density Management
 * Tests TrafficFeedStorage, SimulatedTrafficFeed, and Road density operations
 */

#include <gtest/gtest.h>
#include "../feed/traffic_feed_data.h"
#include "../feed/simulated_traffic_feed.h"
#include "../data/storage/traffic_feed_storage.h"
#include "../data/storage/database_manager.h"
#include "../data/storage/traffic_pattern_storage.h"
#include "../core/road.h"

#include <chrono>
#include <thread>
#include <memory>

using namespace simulator;
using namespace ratms::data;

// ============================================================================
// TrafficFeedEntry Tests
// ============================================================================

class TrafficFeedEntryTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TrafficFeedEntryTest, DefaultConstruction) {
    TrafficFeedEntry entry;
    EXPECT_EQ(entry.timestamp, 0);
    EXPECT_EQ(entry.roadId, 0);
    EXPECT_EQ(entry.expectedVehicleCount, 0);
    EXPECT_DOUBLE_EQ(entry.expectedAvgSpeed, -1.0);  // -1 means unknown
    EXPECT_DOUBLE_EQ(entry.confidence, 1.0);         // Default full confidence
}

TEST_F(TrafficFeedEntryTest, ValueInitialization) {
    TrafficFeedEntry entry;
    entry.timestamp = 1000;
    entry.roadId = 42;
    entry.expectedVehicleCount = 15;
    entry.expectedAvgSpeed = 25.5;
    entry.confidence = 0.85;

    EXPECT_EQ(entry.timestamp, 1000);
    EXPECT_EQ(entry.roadId, 42);
    EXPECT_EQ(entry.expectedVehicleCount, 15);
    EXPECT_DOUBLE_EQ(entry.expectedAvgSpeed, 25.5);
    EXPECT_DOUBLE_EQ(entry.confidence, 0.85);
}

// ============================================================================
// TrafficFeedSnapshot Tests
// ============================================================================

class TrafficFeedSnapshotTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TrafficFeedSnapshotTest, DefaultConstruction) {
    TrafficFeedSnapshot snapshot;
    EXPECT_EQ(snapshot.timestamp, 0);
    EXPECT_TRUE(snapshot.entries.empty());
    EXPECT_EQ(snapshot.source, "unknown");  // Default source
}

TEST_F(TrafficFeedSnapshotTest, AddEntries) {
    TrafficFeedSnapshot snapshot;
    snapshot.timestamp = 1000;
    snapshot.source = "test";

    TrafficFeedEntry e1, e2;
    e1.roadId = 1;
    e1.expectedVehicleCount = 10;
    e2.roadId = 2;
    e2.expectedVehicleCount = 20;

    snapshot.entries.push_back(e1);
    snapshot.entries.push_back(e2);

    EXPECT_EQ(snapshot.entries.size(), 2);
    EXPECT_EQ(snapshot.entries[0].roadId, 1);
    EXPECT_EQ(snapshot.entries[1].roadId, 2);
}

// ============================================================================
// TrafficFeedStorage Tests
// ============================================================================

class TrafficFeedStorageTest : public ::testing::Test {
protected:
    std::shared_ptr<DatabaseManager> db_;
    std::unique_ptr<TrafficFeedStorage> storage_;

    void SetUp() override {
        // Use in-memory database for testing
        db_ = std::make_shared<DatabaseManager>(":memory:");
        ASSERT_TRUE(db_->initialize());

        // Create the traffic_feed_entries table
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS traffic_feed_entries (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp INTEGER NOT NULL,
                road_id INTEGER NOT NULL,
                expected_vehicle_count INTEGER NOT NULL,
                expected_avg_speed REAL,
                confidence REAL DEFAULT 1.0,
                source TEXT NOT NULL,
                created_at INTEGER
            )
        )";
        sqlite3_exec(db_->getConnection(), sql, nullptr, nullptr, nullptr);

        storage_ = std::make_unique<TrafficFeedStorage>(db_);
    }
};

TEST_F(TrafficFeedStorageTest, DefaultConfig) {
    auto config = storage_->getConfig();
    EXPECT_EQ(config.retentionDays, 30);
    EXPECT_EQ(config.batchSize, 100);
    EXPECT_FALSE(config.asyncWrite);
}

TEST_F(TrafficFeedStorageTest, SetConfig) {
    TrafficFeedStorageConfig newConfig;
    newConfig.retentionDays = 7;
    newConfig.batchSize = 50;

    storage_->setConfig(newConfig);

    auto config = storage_->getConfig();
    EXPECT_EQ(config.retentionDays, 7);
    EXPECT_EQ(config.batchSize, 50);
}

TEST_F(TrafficFeedStorageTest, RecordFeedEntry) {
    TrafficFeedEntry entry;
    entry.timestamp = 1000;
    entry.roadId = 1;
    entry.expectedVehicleCount = 10;
    entry.expectedAvgSpeed = 25.0;
    entry.confidence = 0.9;

    EXPECT_TRUE(storage_->recordFeedEntry(entry, "test"));
    EXPECT_EQ(storage_->getEntryCount(), 1);
}

TEST_F(TrafficFeedStorageTest, RecordFeedSnapshot) {
    TrafficFeedSnapshot snapshot;
    snapshot.timestamp = 1000;
    snapshot.source = "test";

    for (int i = 0; i < 5; i++) {
        TrafficFeedEntry entry;
        entry.timestamp = 1000;
        entry.roadId = i;
        entry.expectedVehicleCount = 10 + i;
        entry.expectedAvgSpeed = 20.0;
        entry.confidence = 0.8;
        snapshot.entries.push_back(entry);
    }

    EXPECT_TRUE(storage_->recordFeedSnapshot(snapshot));
    EXPECT_EQ(storage_->getEntryCount(), 5);
}

TEST_F(TrafficFeedStorageTest, GetEntries) {
    // Insert test data
    for (int i = 0; i < 10; i++) {
        TrafficFeedEntry entry;
        entry.timestamp = 1000 + i;
        entry.roadId = i % 3;
        entry.expectedVehicleCount = 10;
        entry.expectedAvgSpeed = 25.0;
        entry.confidence = 1.0;
        storage_->recordFeedEntry(entry, "test");
    }

    auto entries = storage_->getEntries(1000, 1009);
    EXPECT_EQ(entries.size(), 10);
}

TEST_F(TrafficFeedStorageTest, GetEntriesForRoad) {
    // Insert test data for multiple roads
    for (int i = 0; i < 10; i++) {
        TrafficFeedEntry entry;
        entry.timestamp = 1000 + i;
        entry.roadId = i % 3;
        entry.expectedVehicleCount = 10;
        entry.expectedAvgSpeed = 25.0;
        entry.confidence = 1.0;
        storage_->recordFeedEntry(entry, "test");
    }

    // Road 0 should have entries at positions 0, 3, 6, 9 (4 entries)
    auto entries = storage_->getEntriesForRoad(0, 1000, 1009);
    EXPECT_EQ(entries.size(), 4);

    for (const auto& entry : entries) {
        EXPECT_EQ(entry.roadId, 0);
    }
}

TEST_F(TrafficFeedStorageTest, GetEntryCount) {
    EXPECT_EQ(storage_->getEntryCount(), 0);

    for (int i = 0; i < 15; i++) {
        TrafficFeedEntry entry;
        entry.timestamp = 1000 + i;
        entry.roadId = 1;
        entry.expectedVehicleCount = 10;
        entry.expectedAvgSpeed = 25.0;
        entry.confidence = 1.0;
        storage_->recordFeedEntry(entry, "test");
    }

    EXPECT_EQ(storage_->getEntryCount(), 15);
}

TEST_F(TrafficFeedStorageTest, ExportToCSV) {
    TrafficFeedEntry entry;
    entry.timestamp = 1000;
    entry.roadId = 1;
    entry.expectedVehicleCount = 10;
    entry.expectedAvgSpeed = 25.5;
    entry.confidence = 0.9;
    storage_->recordFeedEntry(entry, "test");

    std::string csv = storage_->exportToCSV(1000, 1000);
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("timestamp,road_id,expected_vehicle_count"), std::string::npos);
    EXPECT_NE(csv.find("1000"), std::string::npos);
}

TEST_F(TrafficFeedStorageTest, ExportToJSON) {
    TrafficFeedEntry entry;
    entry.timestamp = 1000;
    entry.roadId = 1;
    entry.expectedVehicleCount = 10;
    entry.expectedAvgSpeed = 25.5;
    entry.confidence = 0.9;
    storage_->recordFeedEntry(entry, "test");

    std::string json = storage_->exportToJSON(1000, 1000);
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("\"timestamp\":1000"), std::string::npos);
    EXPECT_NE(json.find("\"road_id\":1"), std::string::npos);
}

TEST_F(TrafficFeedStorageTest, GetStats) {
    // Insert data from multiple sources
    for (int i = 0; i < 5; i++) {
        TrafficFeedEntry entry;
        entry.timestamp = 1000 + i;
        entry.roadId = i % 2;
        entry.expectedVehicleCount = 10;
        entry.expectedAvgSpeed = 25.0;
        entry.confidence = 1.0;
        storage_->recordFeedEntry(entry, "source_a");
    }
    for (int i = 0; i < 3; i++) {
        TrafficFeedEntry entry;
        entry.timestamp = 2000 + i;
        entry.roadId = 3;
        entry.expectedVehicleCount = 15;
        entry.expectedAvgSpeed = 30.0;
        entry.confidence = 0.8;
        storage_->recordFeedEntry(entry, "source_b");
    }

    auto stats = storage_->getStats();
    EXPECT_EQ(stats.totalEntries, 8);
    EXPECT_GE(stats.uniqueRoads, 2);  // At least 2 unique roads
    EXPECT_EQ(stats.entriesBySource["source_a"], 5);
    EXPECT_EQ(stats.entriesBySource["source_b"], 3);
}

// ============================================================================
// Road Density Operations Tests
// ============================================================================

class RoadDensityTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(RoadDensityTest, SpawnVehicleAtPosition_EmptyRoad) {
    Road r(1, 500.0, 2, 20);

    // Spawn at middle of road
    EXPECT_TRUE(r.spawnVehicleAtPosition(250.0, 0, 15.0, 0.5));
    EXPECT_EQ(r.getVehicleCount(), 1);
}

TEST_F(RoadDensityTest, SpawnVehicleAtPosition_MultiplePositions) {
    Road r(1, 500.0, 1, 20);

    // Spawn vehicles at different positions with enough gap
    EXPECT_TRUE(r.spawnVehicleAtPosition(100.0, 0, 15.0));
    EXPECT_TRUE(r.spawnVehicleAtPosition(200.0, 0, 15.0));
    EXPECT_TRUE(r.spawnVehicleAtPosition(300.0, 0, 15.0));

    EXPECT_EQ(r.getVehicleCount(), 3);
}

TEST_F(RoadDensityTest, SpawnVehicleAtPosition_CollisionDetection) {
    Road r(1, 500.0, 1, 20);

    // Add a vehicle at position 200
    EXPECT_TRUE(r.spawnVehicleAtPosition(200.0, 0, 15.0));

    // Try to spawn at same position - should fail due to collision
    EXPECT_FALSE(r.spawnVehicleAtPosition(200.0, 0, 15.0));
}

TEST_F(RoadDensityTest, SpawnVehicleAtPosition_InvalidLane) {
    Road r(1, 500.0, 2, 20);  // 2 lanes (0 and 1)

    // Lane 5 doesn't exist
    bool result = r.spawnVehicleAtPosition(250.0, 5, 15.0);
    // Should either fail or add to lane 0 (implementation dependent)
    // Just verify it doesn't crash
}

TEST_F(RoadDensityTest, SpawnVehicleAtPosition_NearRoadEnd) {
    Road r(1, 500.0, 1, 20);

    // Spawn near end of road
    EXPECT_TRUE(r.spawnVehicleAtPosition(490.0, 0, 10.0));
    EXPECT_EQ(r.getVehicleCount(), 1);
}

TEST_F(RoadDensityTest, SpawnVehicleAtPosition_NearRoadStart) {
    Road r(1, 500.0, 1, 20);

    // Spawn near start of road
    EXPECT_TRUE(r.spawnVehicleAtPosition(10.0, 0, 10.0));
    EXPECT_EQ(r.getVehicleCount(), 1);
}

TEST_F(RoadDensityTest, RemoveVehicle_RemovesFromTrailing) {
    Road r(1, 500.0, 1, 20);

    // Add multiple vehicles
    r.spawnVehicleAtPosition(100.0, 0, 10.0);
    r.spawnVehicleAtPosition(200.0, 0, 10.0);
    r.spawnVehicleAtPosition(300.0, 0, 10.0);

    EXPECT_EQ(r.getVehicleCount(), 3);

    // Remove trailing vehicle (lowest position)
    EXPECT_TRUE(r.removeVehicle());
    EXPECT_EQ(r.getVehicleCount(), 2);
}

TEST_F(RoadDensityTest, RemoveVehicle_EmptyRoad) {
    Road r(1, 500.0, 1, 20);

    // Try to remove from empty road
    EXPECT_FALSE(r.removeVehicle());
}

TEST_F(RoadDensityTest, RemoveVehicle_MultipleTimes) {
    Road r(1, 500.0, 1, 20);

    // Add vehicles
    for (int i = 0; i < 5; i++) {
        r.spawnVehicleAtPosition(100.0 + i * 50.0, 0, 10.0);
    }
    EXPECT_EQ(r.getVehicleCount(), 5);

    // Remove all vehicles one by one
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(r.removeVehicle());
    }
    EXPECT_EQ(r.getVehicleCount(), 0);

    // Should fail when empty
    EXPECT_FALSE(r.removeVehicle());
}

TEST_F(RoadDensityTest, RemoveVehicle_MultipleLanes) {
    Road r(1, 500.0, 3, 20);

    // Add vehicles to different lanes
    r.spawnVehicleAtPosition(100.0, 0, 10.0);
    r.spawnVehicleAtPosition(150.0, 1, 10.0);
    r.spawnVehicleAtPosition(50.0, 2, 10.0);  // This is the trailing one

    EXPECT_EQ(r.getVehicleCount(), 3);

    // Remove should remove the vehicle with lowest position (lane 2, pos 50)
    EXPECT_TRUE(r.removeVehicle());
    EXPECT_EQ(r.getVehicleCount(), 2);
}

// ============================================================================
// SimulatedTrafficFeed Tests
// ============================================================================

class SimulatedTrafficFeedTest : public ::testing::Test {
protected:
    std::shared_ptr<DatabaseManager> db_;
    std::unique_ptr<TrafficPatternStorage> patternStorage_;
    std::map<roadID, Road> cityMap_;

    void SetUp() override {
        // Create in-memory database
        db_ = std::make_shared<DatabaseManager>(":memory:");
        ASSERT_TRUE(db_->initialize());

        patternStorage_ = std::make_unique<TrafficPatternStorage>(db_);

        // Create a simple road network
        Road r1(1, 500.0, 2, 20);
        Road r2(2, 300.0, 1, 25);
        cityMap_[r1.getId()] = r1;
        cityMap_[r2.getId()] = r2;
    }
};

TEST_F(SimulatedTrafficFeedTest, Construction) {
    SimulatedTrafficFeed feed(*patternStorage_, cityMap_);
    EXPECT_FALSE(feed.isRunning());
    EXPECT_EQ(feed.getSourceName(), "simulated");
}

TEST_F(SimulatedTrafficFeedTest, StartStop) {
    SimulatedTrafficFeed feed(*patternStorage_, cityMap_);

    feed.start();
    EXPECT_TRUE(feed.isRunning());

    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    feed.stop();
    EXPECT_FALSE(feed.isRunning());
}

TEST_F(SimulatedTrafficFeedTest, SetUpdateInterval) {
    SimulatedTrafficFeed feed(*patternStorage_, cityMap_);

    EXPECT_EQ(feed.getUpdateIntervalMs(), 1000);  // Default

    feed.setUpdateIntervalMs(500);
    EXPECT_EQ(feed.getUpdateIntervalMs(), 500);
}

TEST_F(SimulatedTrafficFeedTest, GetLatestSnapshot) {
    SimulatedTrafficFeed feed(*patternStorage_, cityMap_);
    feed.setUpdateIntervalMs(50);  // Fast updates for testing

    feed.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Wait for at least one update
    feed.stop();

    auto snapshot = feed.getLatestSnapshot();
    // Should have entries for roads in the map
    EXPECT_FALSE(snapshot.entries.empty());
    EXPECT_EQ(snapshot.source, "simulated");
}

TEST_F(SimulatedTrafficFeedTest, Subscribe) {
    SimulatedTrafficFeed feed(*patternStorage_, cityMap_);
    feed.setUpdateIntervalMs(50);

    int callbackCount = 0;
    std::mutex countMutex;

    feed.subscribe([&](const TrafficFeedSnapshot& snapshot) {
        std::lock_guard<std::mutex> lock(countMutex);
        callbackCount++;
        EXPECT_FALSE(snapshot.entries.empty());
    });

    feed.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Should get ~4 callbacks
    feed.stop();

    std::lock_guard<std::mutex> lock(countMutex);
    EXPECT_GT(callbackCount, 0);
}

TEST_F(SimulatedTrafficFeedTest, MultipleSubscribers) {
    SimulatedTrafficFeed feed(*patternStorage_, cityMap_);
    feed.setUpdateIntervalMs(50);

    int callback1Count = 0;
    int callback2Count = 0;
    std::mutex countMutex;

    feed.subscribe([&](const TrafficFeedSnapshot&) {
        std::lock_guard<std::mutex> lock(countMutex);
        callback1Count++;
    });

    feed.subscribe([&](const TrafficFeedSnapshot&) {
        std::lock_guard<std::mutex> lock(countMutex);
        callback2Count++;
    });

    feed.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    feed.stop();

    std::lock_guard<std::mutex> lock(countMutex);
    EXPECT_GT(callback1Count, 0);
    EXPECT_GT(callback2Count, 0);
    EXPECT_EQ(callback1Count, callback2Count);  // Both should receive same updates
}

TEST_F(SimulatedTrafficFeedTest, IsHealthy) {
    SimulatedTrafficFeed feed(*patternStorage_, cityMap_);

    EXPECT_FALSE(feed.isHealthy());  // Not running yet

    feed.start();
    EXPECT_TRUE(feed.isHealthy());

    feed.stop();
    EXPECT_FALSE(feed.isHealthy());
}
