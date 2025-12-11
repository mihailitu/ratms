/**
 * Mock DatabaseManager for unit testing
 * Uses Google Mock to create test doubles
 */

#ifndef MOCK_DATABASE_MANAGER_H
#define MOCK_DATABASE_MANAGER_H

#include <gmock/gmock.h>
#include "../../data/storage/database_manager.h"

namespace ratms {
namespace test {

class MockDatabaseManager {
public:
    // Simulation operations
    MOCK_METHOD(int, createSimulation, (const std::string& name, const std::string& description,
                int network_id, const std::string& config_json));
    MOCK_METHOD(bool, updateSimulationStatus, (int sim_id, const std::string& status));
    MOCK_METHOD(bool, completeSimulation, (int sim_id, long end_time, double duration));
    MOCK_METHOD(data::DatabaseManager::SimulationRecord, getSimulation, (int sim_id));
    MOCK_METHOD(std::vector<data::DatabaseManager::SimulationRecord>, getAllSimulations, ());

    // Metrics operations
    MOCK_METHOD(bool, insertMetric, (int simulation_id, double timestamp, const std::string& metric_type,
                int road_id, double value, const std::string& unit, const std::string& metadata_json));
    MOCK_METHOD(std::vector<data::DatabaseManager::MetricRecord>, getMetrics, (int simulation_id));
    MOCK_METHOD(std::vector<data::DatabaseManager::MetricRecord>, getMetricsByType,
                (int simulation_id, const std::string& metric_type));

    // Network operations
    MOCK_METHOD(int, createNetwork, (const std::string& name, const std::string& description,
                int road_count, int intersection_count, const std::string& config_json));
    MOCK_METHOD(data::DatabaseManager::NetworkRecord, getNetwork, (int network_id));
    MOCK_METHOD(std::vector<data::DatabaseManager::NetworkRecord>, getAllNetworks, ());
    MOCK_METHOD(bool, deleteNetwork, (int network_id));

    // Optimization operations
    MOCK_METHOD(int, createOptimizationRun, (const data::DatabaseManager::OptimizationRunRecord& record));
    MOCK_METHOD(bool, updateOptimizationRunStatus, (int run_id, const std::string& status));
    MOCK_METHOD(bool, completeOptimizationRun, (int run_id, long completed_at, long duration_seconds,
                double baseline_fitness, double best_fitness, double improvement_percent));
    MOCK_METHOD(data::DatabaseManager::OptimizationRunRecord, getOptimizationRun, (int run_id));
    MOCK_METHOD(std::vector<data::DatabaseManager::OptimizationRunRecord>, getAllOptimizationRuns, ());

    // Traffic snapshot operations
    MOCK_METHOD(bool, insertTrafficSnapshot, (const data::DatabaseManager::TrafficSnapshotRecord& record));
    MOCK_METHOD(std::vector<data::DatabaseManager::TrafficSnapshotRecord>, getTrafficSnapshots, (int64_t since_timestamp));

    // Traffic pattern operations
    MOCK_METHOD(bool, insertOrUpdateTrafficPattern, (const data::DatabaseManager::TrafficPatternRecord& record));
    MOCK_METHOD(data::DatabaseManager::TrafficPatternRecord, getTrafficPattern, (int road_id, int day_of_week, int time_slot));
    MOCK_METHOD(std::vector<data::DatabaseManager::TrafficPatternRecord>, getAllTrafficPatterns, ());

    // Profile operations
    MOCK_METHOD(int, createProfile, (const std::string& name, const std::string& description));
    MOCK_METHOD(data::DatabaseManager::ProfileRecord, getProfile, (int profile_id));
    MOCK_METHOD(std::vector<data::DatabaseManager::ProfileRecord>, getAllProfiles, ());
    MOCK_METHOD(bool, deleteProfile, (int profile_id));

    // Utility
    MOCK_METHOD(bool, isConnected, (), (const));
    MOCK_METHOD(std::string, getLastError, (), (const));
};

} // namespace test
} // namespace ratms

#endif // MOCK_DATABASE_MANAGER_H
