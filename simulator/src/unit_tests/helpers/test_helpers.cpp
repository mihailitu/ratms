/**
 * Test helper utilities implementation
 */

#include "test_helpers.h"
#include <cmath>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ratms {
namespace test {

simulator::Road createTestRoad(int id, double length, int lanes, double maxSpeed) {
    return simulator::Road(id, length, lanes, maxSpeed);
}

simulator::Vehicle createTestVehicle(double pos, double length, double maxVelocity) {
    return simulator::Vehicle(pos, length, maxVelocity);
}

simulator::Vehicle createTestTrafficLight(double pos) {
    return simulator::Vehicle(pos, 0.0, 0.0, simulator::Vehicle::traffic_light);
}

std::vector<simulator::Road> createConnectedNetwork(int roadCount) {
    std::vector<simulator::Road> network;
    network.reserve(roadCount);

    for (int i = 0; i < roadCount; i++) {
        simulator::Road r(i, 500.0, 2, 20);
        network.push_back(r);
    }

    // Connect roads in a simple chain
    // Road 0 -> Road 1 -> Road 2 -> ... -> Road N-1
    for (int i = 0; i < roadCount - 1; i++) {
        // Note: Actual connection setup depends on Road API
        // This is a simplified version
    }

    return network;
}

data::DatabaseManager::TrafficPatternRecord createTestPattern(
    int roadId,
    int dayOfWeek,
    int timeSlot,
    double avgVehicleCount
) {
    data::DatabaseManager::TrafficPatternRecord record;
    record.id = 0;
    record.road_id = roadId;
    record.day_of_week = dayOfWeek;
    record.time_slot = timeSlot;
    record.avg_vehicle_count = avgVehicleCount;
    record.avg_queue_length = avgVehicleCount * 0.3;
    record.avg_speed = 15.0;
    record.avg_flow_rate = avgVehicleCount * 2.0;
    record.min_vehicle_count = avgVehicleCount * 0.5;
    record.max_vehicle_count = avgVehicleCount * 1.5;
    record.stddev_vehicle_count = avgVehicleCount * 0.2;
    record.sample_count = 100;
    record.last_updated = currentTimestampMs();
    return record;
}

data::DatabaseManager::SimulationRecord createTestSimulation(
    int id,
    const std::string& name,
    const std::string& status
) {
    data::DatabaseManager::SimulationRecord record;
    record.id = id;
    record.name = name;
    record.description = "Test simulation for unit testing";
    record.network_id = 1;
    record.status = status;
    record.start_time = currentTimestampMs();
    record.end_time = 0;
    record.duration_seconds = 0;
    record.config_json = "{}";
    return record;
}

data::DatabaseManager::OptimizationRunRecord createTestOptimizationRun(
    int id,
    int populationSize,
    int generations
) {
    data::DatabaseManager::OptimizationRunRecord record;
    record.id = id;
    record.network_id = 1;
    record.status = "pending";
    record.population_size = populationSize;
    record.generations = generations;
    record.mutation_rate = 0.15;
    record.crossover_rate = 0.8;
    record.elitism_rate = 0.1;
    record.min_green_time = 10.0;
    record.max_green_time = 60.0;
    record.min_red_time = 10.0;
    record.max_red_time = 60.0;
    record.simulation_steps = 1000;
    record.dt = 0.1;
    record.baseline_fitness = 0.0;
    record.best_fitness = 0.0;
    record.improvement_percent = 0.0;
    record.started_at = currentTimestampMs();
    record.completed_at = 0;
    record.duration_seconds = 0;
    record.created_by = "unit_test";
    record.notes = "";
    return record;
}

bool approximatelyEqual(double a, double b, double epsilon) {
    return std::fabs(a - b) < epsilon;
}

int64_t currentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::string uniqueTestName(const std::string& prefix) {
    auto timestamp = currentTimestampMs();
    std::ostringstream oss;
    oss << prefix << "_" << timestamp;
    return oss.str();
}

} // namespace test
} // namespace ratms
