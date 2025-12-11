/**
 * Unit tests for Simulator class
 * Tests simulation orchestration, road management, and serialization
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
    Road r(1, 500.0, 2, 20);
    sim_.addRoadToMap(r);

    EXPECT_EQ(sim_.cityMap.size(), 1);
    EXPECT_TRUE(sim_.cityMap.find(1) != sim_.cityMap.end());
}

TEST_F(SimulatorTest, AddRoadToMap_MultipleRoads) {
    Road r1(1, 500.0, 2, 20);
    Road r2(2, 300.0, 3, 25);
    Road r3(3, 1000.0, 1, 15);

    sim_.addRoadToMap(r1);
    sim_.addRoadToMap(r2);
    sim_.addRoadToMap(r3);

    EXPECT_EQ(sim_.cityMap.size(), 3);
    EXPECT_TRUE(sim_.cityMap.find(1) != sim_.cityMap.end());
    EXPECT_TRUE(sim_.cityMap.find(2) != sim_.cityMap.end());
    EXPECT_TRUE(sim_.cityMap.find(3) != sim_.cityMap.end());
}

TEST_F(SimulatorTest, AddRoadToMap_DuplicateIdOverwrites) {
    Road r1(1, 500.0, 2, 20);
    sim_.addRoadToMap(r1);

    Road r2(1, 300.0, 3, 25);  // Same ID, different properties
    sim_.addRoadToMap(r2);

    EXPECT_EQ(sim_.cityMap.size(), 1);
    EXPECT_DOUBLE_EQ(sim_.cityMap[1].getLength(), 300.0);
}

TEST_F(SimulatorTest, AddRoadNetToMap_EmptyNetwork) {
    std::vector<Road> emptyNet;
    sim_.addRoadNetToMap(emptyNet);

    EXPECT_TRUE(sim_.cityMap.empty());
}

TEST_F(SimulatorTest, AddRoadNetToMap_MultipleRoads) {
    std::vector<Road> roadNet;
    roadNet.push_back(Road(1, 500.0, 2, 20));
    roadNet.push_back(Road(2, 300.0, 2, 20));
    roadNet.push_back(Road(3, 400.0, 2, 20));

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
    Road r(1, 500.0, 2, 20);
    sim_.addRoadToMap(r);

    std::ostringstream output;
    sim_.serialize(1.5, output);

    std::string result = output.str();
    EXPECT_FALSE(result.empty());
    // Should contain time and road ID
    EXPECT_TRUE(result.find("1.5") != std::string::npos || result.find("1.50") != std::string::npos);
}

TEST_F(SimulatorTest, Serialize_RoadWithVehicles) {
    Road r(1, 500.0, 2, 20);
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
    Road r(42, 500.0, 2, 20);
    sim_.addRoadToMap(r);

    auto it = sim_.cityMap.find(42);
    EXPECT_NE(it, sim_.cityMap.end());
    EXPECT_EQ(it->second.getId(), 42);
}

TEST_F(SimulatorTest, CityMapAccess_FindNonExistentRoad) {
    Road r(1, 500.0, 2, 20);
    sim_.addRoadToMap(r);

    auto it = sim_.cityMap.find(999);
    EXPECT_EQ(it, sim_.cityMap.end());
}

TEST_F(SimulatorTest, CityMapIteration) {
    Road r1(1, 500.0, 2, 20);
    Road r2(2, 300.0, 2, 20);
    sim_.addRoadToMap(r1);
    sim_.addRoadToMap(r2);

    int count = 0;
    for (const auto& [id, road] : sim_.cityMap) {
        EXPECT_TRUE(id == 1 || id == 2);
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
    EXPECT_EQ(road.getId(), 10);
    EXPECT_EQ(road.getVehicleCount(), 5);
}

TEST_F(SimulatorFixtureTest, FourWayIntersection_HasFourRoads) {
    createFourWayIntersection();
    EXPECT_EQ(sim_.cityMap.size(), 4);
}
