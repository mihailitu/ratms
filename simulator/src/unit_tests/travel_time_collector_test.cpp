/**
 * Unit tests for TravelTimeCollector class
 * Tests O-D pair management, vehicle tracking, and statistics
 */

#include <gtest/gtest.h>
#include "../metrics/travel_time_collector.h"
#include "../core/simulator.h"
#include "../core/road.h"
#include "../core/vehicle.h"
#include "fixtures/test_fixtures.h"

using namespace ratms::metrics;
using namespace simulator;

class TravelTimeCollectorTest : public ratms::test::DatabaseTestFixture {
protected:
    std::unique_ptr<TravelTimeCollector> collector_;
    Simulator::CityMap cityMap_;

    void SetUp() override {
        DatabaseTestFixture::SetUp();
        collector_ = std::make_unique<TravelTimeCollector>(
            std::shared_ptr<ratms::data::DatabaseManager>(db_.release())
        );

        // Create simple road network for testing
        Road origin(1, 500.0, 2, 20);
        Road destination(2, 500.0, 2, 20);
        cityMap_[1] = origin;
        cityMap_[2] = destination;
    }
};

// O-D Pair management tests
TEST_F(TravelTimeCollectorTest, AddODPair_ReturnsValidId) {
    int id = collector_->addODPair(1, 2, "Test Route", "Origin to Destination");
    EXPECT_GT(id, 0);
}

TEST_F(TravelTimeCollectorTest, AddODPair_IncrementingIds) {
    int id1 = collector_->addODPair(1, 2);
    int id2 = collector_->addODPair(1, 3);
    int id3 = collector_->addODPair(2, 3);

    EXPECT_EQ(id2, id1 + 1);
    EXPECT_EQ(id3, id2 + 1);
}

TEST_F(TravelTimeCollectorTest, GetAllODPairs_EmptyInitially) {
    auto pairs = collector_->getAllODPairs();
    EXPECT_TRUE(pairs.empty());
}

TEST_F(TravelTimeCollectorTest, GetAllODPairs_ReturnsAddedPairs) {
    collector_->addODPair(1, 2, "Route A");
    collector_->addODPair(2, 3, "Route B");

    auto pairs = collector_->getAllODPairs();
    EXPECT_EQ(pairs.size(), 2);
}

TEST_F(TravelTimeCollectorTest, GetODPair_ReturnsCorrectData) {
    int id = collector_->addODPair(1, 2, "Test Route", "Description");
    ODPair pair = collector_->getODPair(id);

    EXPECT_EQ(pair.id, id);
    EXPECT_EQ(pair.originRoadId, 1);
    EXPECT_EQ(pair.destinationRoadId, 2);
    EXPECT_EQ(pair.name, "Test Route");
    EXPECT_EQ(pair.description, "Description");
}

TEST_F(TravelTimeCollectorTest, RemoveODPair_RemovesPair) {
    int id = collector_->addODPair(1, 2);
    EXPECT_EQ(collector_->getAllODPairs().size(), 1);

    collector_->removeODPair(id);
    EXPECT_TRUE(collector_->getAllODPairs().empty());
}

TEST_F(TravelTimeCollectorTest, RemoveODPair_NonExistent_NoError) {
    // Removing non-existent pair should not crash
    collector_->removeODPair(999);
    SUCCEED();
}

// Tracking tests
TEST_F(TravelTimeCollectorTest, GetTrackedVehicles_EmptyInitially) {
    auto tracked = collector_->getTrackedVehicles();
    EXPECT_TRUE(tracked.empty());
}

TEST_F(TravelTimeCollectorTest, Update_EmptyMap_NoError) {
    Simulator::CityMap emptyMap;
    collector_->update(emptyMap, 0.1);
    SUCCEED();
}

TEST_F(TravelTimeCollectorTest, Update_NoODPairs_NoTracking) {
    // Add a vehicle to the origin road
    Vehicle v(100.0, 5.0, 15.0);
    cityMap_[1].addVehicle(v, 0);

    collector_->update(cityMap_, 0.1);

    // No O-D pairs defined, so no tracking
    EXPECT_TRUE(collector_->getTrackedVehicles().empty());
}

// Statistics tests
TEST_F(TravelTimeCollectorTest, GetStats_NoSamples) {
    int id = collector_->addODPair(1, 2);
    TravelTimeStats stats = collector_->getStats(id);

    EXPECT_EQ(stats.odPairId, id);
    EXPECT_EQ(stats.sampleCount, 0);
}

TEST_F(TravelTimeCollectorTest, GetAllStats_ReturnsStatsForAllPairs) {
    collector_->addODPair(1, 2);
    collector_->addODPair(2, 3);

    auto allStats = collector_->getAllStats();
    EXPECT_EQ(allStats.size(), 2);
}

TEST_F(TravelTimeCollectorTest, GetRecentSamples_EmptyInitially) {
    int id = collector_->addODPair(1, 2);
    auto samples = collector_->getRecentSamples(id);
    EXPECT_TRUE(samples.empty());
}

TEST_F(TravelTimeCollectorTest, GetRecentSamples_RespectLimit) {
    int id = collector_->addODPair(1, 2);
    auto samples = collector_->getRecentSamples(id, 10);
    // Should return empty or up to 10 samples
    EXPECT_LE(samples.size(), 10);
}

// Reset and flush tests
TEST_F(TravelTimeCollectorTest, Reset_ClearsTracking) {
    collector_->addODPair(1, 2);

    // Add some state
    collector_->update(cityMap_, 0.1);

    collector_->reset();

    EXPECT_TRUE(collector_->getTrackedVehicles().empty());
    EXPECT_TRUE(collector_->getAllODPairs().empty());
}

TEST_F(TravelTimeCollectorTest, Flush_NoError) {
    collector_->addODPair(1, 2);
    collector_->update(cityMap_, 0.1);

    // Flush should not throw
    collector_->flush();
    SUCCEED();
}
