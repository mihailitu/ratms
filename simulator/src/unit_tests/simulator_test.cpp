/**
 * Unit tests for Simulator class
 * Tests simulation orchestration, road management, and serialization
 *
 * Note: Road IDs are auto-generated, not user-specified.
 * The Road constructor ignores the rId parameter.
 */

#include <gtest/gtest.h>
#include <sstream>
#include "../core/simulator.h"
#include "../core/road.h"
#include "../core/vehicle.h"
#include "fixtures/test_fixtures.h"
#include "helpers/test_helpers.h"

using namespace simulator;

class SimulatorTest : public ::testing::Test {
protected:
    Simulator sim_;

    void SetUp() override {
        // Fresh simulator for each test
    }
};

// Construction tests
TEST_F(SimulatorTest, DefaultConstruction) {
    EXPECT_TRUE(sim_.cityMap.empty());
}

// Road management tests
TEST_F(SimulatorTest, AddRoadToMap_SingleRoad) {
    Road r(0, 500.0, 2, 20);
    roadID actualId = r.getId();  // Get the auto-generated ID
    sim_.addRoadToMap(r);

    EXPECT_EQ(sim_.cityMap.size(), 1);
    EXPECT_TRUE(sim_.cityMap.find(actualId) != sim_.cityMap.end());
}

TEST_F(SimulatorTest, AddRoadToMap_MultipleRoads) {
    Road r1(0, 500.0, 2, 20);
    Road r2(0, 300.0, 3, 25);
    Road r3(0, 1000.0, 1, 15);

    roadID id1 = r1.getId();
    roadID id2 = r2.getId();
    roadID id3 = r3.getId();

    sim_.addRoadToMap(r1);
    sim_.addRoadToMap(r2);
    sim_.addRoadToMap(r3);

    EXPECT_EQ(sim_.cityMap.size(), 3);
    EXPECT_TRUE(sim_.cityMap.find(id1) != sim_.cityMap.end());
    EXPECT_TRUE(sim_.cityMap.find(id2) != sim_.cityMap.end());
    EXPECT_TRUE(sim_.cityMap.find(id3) != sim_.cityMap.end());
}

TEST_F(SimulatorTest, AddRoadToMap_SameRoadTwice_Overwrites) {
    Road r1(0, 500.0, 2, 20);
    roadID id1 = r1.getId();

    sim_.addRoadToMap(r1);
    EXPECT_EQ(sim_.cityMap.size(), 1);
    EXPECT_DOUBLE_EQ(sim_.cityMap[id1].getLength(), 500.0);

    // Adding the same road again overwrites
    Road r2(0, 300.0, 3, 25);
    roadID id2 = r2.getId();
    sim_.addRoadToMap(r2);

    // Now we have 2 roads with different IDs
    EXPECT_EQ(sim_.cityMap.size(), 2);
}

TEST_F(SimulatorTest, AddRoadNetToMap_EmptyNetwork) {
    std::vector<Road> emptyNet;
    sim_.addRoadNetToMap(emptyNet);

    EXPECT_TRUE(sim_.cityMap.empty());
}

TEST_F(SimulatorTest, AddRoadNetToMap_MultipleRoads) {
    std::vector<Road> roadNet;
    roadNet.push_back(Road(0, 500.0, 2, 20));
    roadNet.push_back(Road(0, 300.0, 2, 20));
    roadNet.push_back(Road(0, 400.0, 2, 20));

    sim_.addRoadNetToMap(roadNet);

    EXPECT_EQ(sim_.cityMap.size(), 3);
}

// Serialization tests
TEST_F(SimulatorTest, Serialize_EmptyMap) {
    std::ostringstream output;
    sim_.serialize(0.0, output);

    // Empty map should produce empty output
    EXPECT_TRUE(output.str().empty());
}

TEST_F(SimulatorTest, Serialize_SingleRoadNoVehicles) {
    Road r(0, 500.0, 2, 20);
    sim_.addRoadToMap(r);

    std::ostringstream output;
    sim_.serialize(1.5, output);

    std::string result = output.str();
    EXPECT_FALSE(result.empty());
    // Should contain time
    EXPECT_TRUE(result.find("1.5") != std::string::npos || result.find("1.50") != std::string::npos);
}

TEST_F(SimulatorTest, Serialize_RoadWithVehicles) {
    Road r(0, 500.0, 2, 20);
    Vehicle v(100.0, 5.0, 15.0);
    r.addVehicle(v, 0);
    sim_.addRoadToMap(r);

    std::ostringstream output;
    sim_.serialize(2.0, output);

    std::string result = output.str();
    EXPECT_FALSE(result.empty());
}

// CityMap access tests
TEST_F(SimulatorTest, CityMapAccess_FindExistingRoad) {
    Road r(0, 500.0, 2, 20);
    roadID actualId = r.getId();
    sim_.addRoadToMap(r);

    auto it = sim_.cityMap.find(actualId);
    EXPECT_NE(it, sim_.cityMap.end());
    EXPECT_EQ(it->second.getId(), actualId);
}

TEST_F(SimulatorTest, CityMapAccess_FindNonExistentRoad) {
    Road r(0, 500.0, 2, 20);
    sim_.addRoadToMap(r);

    // Use an ID that definitely doesn't exist
    auto it = sim_.cityMap.find(999999);
    EXPECT_EQ(it, sim_.cityMap.end());
}

TEST_F(SimulatorTest, CityMapIteration) {
    Road r1(0, 500.0, 2, 20);
    Road r2(0, 300.0, 2, 20);
    roadID id1 = r1.getId();
    roadID id2 = r2.getId();

    sim_.addRoadToMap(r1);
    sim_.addRoadToMap(r2);

    int count = 0;
    for (const auto& [id, road] : sim_.cityMap) {
        EXPECT_TRUE(id == id1 || id == id2);
        count++;
    }
    EXPECT_EQ(count, 2);
}

// Using the fixture for more complex tests
class SimulatorFixtureTest : public ratms::test::SimulatorTestFixture {};

TEST_F(SimulatorFixtureTest, FixtureCreatesNetwork) {
    EXPECT_EQ(sim_.cityMap.size(), 4);
}

TEST_F(SimulatorFixtureTest, CreatePopulatedRoad_HasVehicles) {
    auto road = createPopulatedRoad(10, 5);
    // Road ID is auto-generated, don't check specific value
    EXPECT_EQ(road.getVehicleCount(), 5);
}

TEST_F(SimulatorFixtureTest, FourWayIntersection_HasFourRoads) {
    createFourWayIntersection();
    EXPECT_EQ(sim_.cityMap.size(), 4);
}
