/**
 * Unit tests for NetworkLoader class
 * Tests JSON network loading and parsing
 */

#include <gtest/gtest.h>
#include "../mapping/network_loader.h"
#include "../core/road.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

using namespace mapping;
using namespace simulator;

class NetworkLoaderTest : public ::testing::Test {
protected:
    std::string testJsonPath_;
    std::string invalidJsonPath_;
    std::string malformedJsonPath_;

    void SetUp() override {
        // Create temporary test files
        testJsonPath_ = "/tmp/test_network.json";
        invalidJsonPath_ = "/tmp/nonexistent.json";
        malformedJsonPath_ = "/tmp/malformed.json";

        // Create a valid test JSON file
        std::ofstream valid(testJsonPath_);
        valid << R"({
            "metadata": {
                "name": "Test Network",
                "version": "1.0",
                "bbox": {
                    "min_lon": -122.5,
                    "min_lat": 37.5,
                    "max_lon": -122.0,
                    "max_lat": 38.0
                }
            },
            "roads": [
                {
                    "id": 1,
                    "length": 500.0,
                    "lanes": 2,
                    "maxSpeed": 20.0,
                    "startLon": -122.4,
                    "startLat": 37.7,
                    "endLon": -122.3,
                    "endLat": 37.8
                },
                {
                    "id": 2,
                    "length": 300.0,
                    "lanes": 3,
                    "maxSpeed": 25.0,
                    "startLon": -122.3,
                    "startLat": 37.8,
                    "endLon": -122.2,
                    "endLat": 37.9
                }
            ],
            "intersections": [],
            "connections": []
        })";
        valid.close();

        // Create malformed JSON
        std::ofstream malformed(malformedJsonPath_);
        malformed << "{ this is not valid JSON }}}";
        malformed.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove(testJsonPath_);
        std::filesystem::remove(malformedJsonPath_);
    }
};

// LoadFromJson tests
TEST_F(NetworkLoaderTest, LoadFromJson_ValidFile) {
    auto roads = NetworkLoader::loadFromJson(testJsonPath_);

    EXPECT_EQ(roads.size(), 2);
}

TEST_F(NetworkLoaderTest, LoadFromJson_CorrectRoadProperties) {
    auto roads = NetworkLoader::loadFromJson(testJsonPath_);

    ASSERT_GE(roads.size(), 1);

    // Find road with ID 1
    auto it = std::find_if(roads.begin(), roads.end(),
        [](const Road& r) { return r.getId() == 1; });

    ASSERT_NE(it, roads.end());
    EXPECT_DOUBLE_EQ(it->getLength(), 500.0);
    EXPECT_EQ(it->getLanesNo(), 2);
}

TEST_F(NetworkLoaderTest, LoadFromJson_NonExistentFile) {
    EXPECT_THROW(
        NetworkLoader::loadFromJson(invalidJsonPath_),
        std::exception
    );
}

TEST_F(NetworkLoaderTest, LoadFromJson_MalformedJson) {
    EXPECT_THROW(
        NetworkLoader::loadFromJson(malformedJsonPath_),
        std::exception
    );
}

// LoadIntoCityMap tests
TEST_F(NetworkLoaderTest, LoadIntoCityMap_PopulatesMap) {
    std::map<roadID, Road> cityMap;
    NetworkLoader::loadIntoCityMap(testJsonPath_, cityMap);

    EXPECT_EQ(cityMap.size(), 2);
    EXPECT_TRUE(cityMap.find(1) != cityMap.end());
    EXPECT_TRUE(cityMap.find(2) != cityMap.end());
}

TEST_F(NetworkLoaderTest, LoadIntoCityMap_PreservesExisting) {
    std::map<roadID, Road> cityMap;

    // Add an existing road
    Road existing(100, 200.0, 1, 15);
    cityMap[100] = existing;

    NetworkLoader::loadIntoCityMap(testJsonPath_, cityMap);

    // Should have existing + loaded roads
    EXPECT_EQ(cityMap.size(), 3);
    EXPECT_TRUE(cityMap.find(100) != cityMap.end());
}

// GetNetworkInfo tests
TEST_F(NetworkLoaderTest, GetNetworkInfo_ReturnsMetadata) {
    NetworkLoader::NetworkInfo info = NetworkLoader::getNetworkInfo(testJsonPath_);

    EXPECT_EQ(info.name, "Test Network");
    EXPECT_EQ(info.version, "1.0");
}

TEST_F(NetworkLoaderTest, GetNetworkInfo_ReturnsBbox) {
    NetworkLoader::NetworkInfo info = NetworkLoader::getNetworkInfo(testJsonPath_);

    EXPECT_DOUBLE_EQ(info.bboxMinLon, -122.5);
    EXPECT_DOUBLE_EQ(info.bboxMinLat, 37.5);
    EXPECT_DOUBLE_EQ(info.bboxMaxLon, -122.0);
    EXPECT_DOUBLE_EQ(info.bboxMaxLat, 38.0);
}

TEST_F(NetworkLoaderTest, GetNetworkInfo_CountsRoads) {
    NetworkLoader::NetworkInfo info = NetworkLoader::getNetworkInfo(testJsonPath_);

    EXPECT_EQ(info.totalRoads, 2);
}

TEST_F(NetworkLoaderTest, GetNetworkInfo_NonExistentFile) {
    EXPECT_THROW(
        NetworkLoader::getNetworkInfo(invalidJsonPath_),
        std::exception
    );
}

// Edge cases
TEST_F(NetworkLoaderTest, LoadFromJson_EmptyRoadsArray) {
    std::string emptyRoadsPath = "/tmp/empty_roads.json";
    std::ofstream f(emptyRoadsPath);
    f << R"({
        "metadata": {"name": "Empty"},
        "roads": [],
        "intersections": [],
        "connections": []
    })";
    f.close();

    auto roads = NetworkLoader::loadFromJson(emptyRoadsPath);
    EXPECT_TRUE(roads.empty());

    std::filesystem::remove(emptyRoadsPath);
}
