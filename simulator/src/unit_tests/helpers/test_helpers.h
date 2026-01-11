/**
 * Test helper utilities for RATMS backend testing
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <thread>

#include "../../core/road.h"
#include "../../core/vehicle.h"
#include "../../data/storage/database_manager.h"

namespace ratms {
namespace test {

/**
 * Create a road with specified parameters
 */
simulator::Road createTestRoad(int id, double length = 500.0, int lanes = 2, double maxSpeed = 20.0);

/**
 * Create a vehicle at a specific position
 */
simulator::Vehicle createTestVehicle(double pos = 0.0, double length = 5.0, double maxVelocity = 15.0);

/**
 * Create a traffic light vehicle
 */
simulator::Vehicle createTestTrafficLight(double pos = 100.0);

/**
 * Create a connected road network with specified topology
 */
std::vector<simulator::Road> createConnectedNetwork(int roadCount);

/**
 * Create test traffic pattern data
 */
data::DatabaseManager::TrafficPatternRecord createTestPattern(
    int roadId,
    int dayOfWeek = 1,
    int timeSlot = 12,
    double avgVehicleCount = 10.0
);

/**
 * Create test simulation record
 */
data::DatabaseManager::SimulationRecord createTestSimulation(
    int id = 1,
    const std::string& name = "Test Simulation",
    const std::string& status = "running"
);

/**
 * Create test optimization run record
 */
data::DatabaseManager::OptimizationRunRecord createTestOptimizationRun(
    int id = 1,
    int populationSize = 50,
    int generations = 100
);

/**
 * Wait for a condition with timeout
 * Returns true if condition was met, false on timeout
 */
template<typename Predicate>
bool waitFor(Predicate pred, int timeoutMs = 5000) {
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        ).count();
        if (elapsed >= timeoutMs) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

/**
 * Compare floating point values with tolerance
 */
bool approximatelyEqual(double a, double b, double epsilon = 1e-6);

/**
 * Get current timestamp in milliseconds
 */
int64_t currentTimestampMs();

/**
 * Generate a unique test name with timestamp
 */
std::string uniqueTestName(const std::string& prefix = "test");

} // namespace test
} // namespace ratms

#endif // TEST_HELPERS_H
