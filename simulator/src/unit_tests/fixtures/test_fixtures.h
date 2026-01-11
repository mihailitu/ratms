/**
 * Test fixtures for RATMS backend testing
 * Provides reusable setup/teardown for common test scenarios
 */

#ifndef TEST_FIXTURES_H
#define TEST_FIXTURES_H

#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "../../core/simulator.h"
#include "../../core/road.h"
#include "../../core/vehicle.h"
#include "../../data/storage/database_manager.h"

namespace ratms {
namespace test {

/**
 * Base fixture with in-memory SQLite database
 * Use for testing database operations without file I/O
 */
class DatabaseTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<data::DatabaseManager> db_;

    void SetUp() override;
    void TearDown() override;

    // Helper to get migrations directory path
    std::string getMigrationsPath() const;
};

/**
 * Fixture with a pre-built simulator and road network
 * Use for testing simulation logic
 */
class SimulatorTestFixture : public ::testing::Test {
protected:
    simulator::Simulator sim_;
    std::vector<simulator::Road> testNetwork_;

    void SetUp() override;
    void TearDown() override;

    // Create a simple test network with N roads
    void createTestNetwork(int roadCount = 4);

    // Create a populated road with vehicles
    simulator::Road createPopulatedRoad(int id, int vehicleCount, double length = 500.0);

    // Create a four-way intersection network
    void createFourWayIntersection();
};

/**
 * Combined fixture for integration tests
 * Provides both database and simulator
 */
class IntegrationTestFixture : public DatabaseTestFixture {
protected:
    std::unique_ptr<simulator::Simulator> simulator_;

    void SetUp() override;
    void TearDown() override;
};

} // namespace test
} // namespace ratms

#endif // TEST_FIXTURES_H
