/**
 * Test fixtures implementation
 */

#include "test_fixtures.h"
#include "../../utils/config.h"

#include <filesystem>

namespace ratms {
namespace test {

// DatabaseTestFixture implementation

void DatabaseTestFixture::SetUp() {
    // Use in-memory database for fast, isolated tests
    db_ = std::make_unique<data::DatabaseManager>(":memory:");
    ASSERT_TRUE(db_->initialize()) << "Failed to initialize in-memory database";

    // Run migrations
    std::string migrations_path = getMigrationsPath();
    if (std::filesystem::exists(migrations_path)) {
        ASSERT_TRUE(db_->runMigrations(migrations_path))
            << "Failed to run migrations from: " << migrations_path;
    }
}

void DatabaseTestFixture::TearDown() {
    if (db_) {
        db_->close();
        db_.reset();
    }
}

std::string DatabaseTestFixture::getMigrationsPath() const {
    // Try relative paths from common build locations
    std::vector<std::string> possible_paths = {
        "../../database/migrations",
        "../database/migrations",
        "../../../database/migrations",
        "database/migrations"
    };

    for (const auto& path : possible_paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    // Return default path even if it doesn't exist
    return "../../database/migrations";
}

// SimulatorTestFixture implementation

void SimulatorTestFixture::SetUp() {
    testNetwork_.clear();
    createTestNetwork(4);
}

void SimulatorTestFixture::TearDown() {
    testNetwork_.clear();
    sim_.cityMap.clear();
}

void SimulatorTestFixture::createTestNetwork(int roadCount) {
    for (int i = 0; i < roadCount; i++) {
        simulator::Road r(i, 500.0, 2, 20);  // id, length, lanes, maxSpeed
        testNetwork_.push_back(r);
        sim_.addRoadToMap(testNetwork_.back());
    }
}

simulator::Road SimulatorTestFixture::createPopulatedRoad(int id, int vehicleCount, double length) {
    simulator::Road r(id, length, 2, 20);

    for (int i = 0; i < vehicleCount; i++) {
        double pos = (length / (vehicleCount + 1)) * (i + 1);
        simulator::Vehicle v(pos, 5.0, 15.0);  // pos, length, maxVelocity
        r.addVehicle(v, 0);  // Add to lane 0
    }

    return r;
}

void SimulatorTestFixture::createFourWayIntersection() {
    testNetwork_.clear();
    sim_.cityMap.clear();

    // Create 4 roads forming an intersection
    // North-South road (id: 0, 1)
    // East-West road (id: 2, 3)

    simulator::Road northSouth(0, 300.0, 2, 20);
    simulator::Road southNorth(1, 300.0, 2, 20);
    simulator::Road eastWest(2, 300.0, 2, 20);
    simulator::Road westEast(3, 300.0, 2, 20);

    testNetwork_.push_back(northSouth);
    testNetwork_.push_back(southNorth);
    testNetwork_.push_back(eastWest);
    testNetwork_.push_back(westEast);

    for (auto& road : testNetwork_) {
        sim_.addRoadToMap(road);
    }
}

// IntegrationTestFixture implementation

void IntegrationTestFixture::SetUp() {
    DatabaseTestFixture::SetUp();
    simulator_ = std::make_unique<simulator::Simulator>();
}

void IntegrationTestFixture::TearDown() {
    simulator_.reset();
    DatabaseTestFixture::TearDown();
}

} // namespace test
} // namespace ratms
