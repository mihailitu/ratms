/**
 * Integration tests for DatabaseManager
 * Tests database operations with in-memory SQLite
 */

#include <gtest/gtest.h>
#include "../../data/storage/database_manager.h"
#include "../fixtures/test_fixtures.h"
#include "../helpers/test_helpers.h"

using namespace ratms::data;
using namespace ratms::test;

class DatabaseManagerIntegrationTest : public DatabaseTestFixture {};

// Connection tests
TEST_F(DatabaseManagerIntegrationTest, Initialize_InMemory) {
    EXPECT_TRUE(db_->isConnected());
}

TEST_F(DatabaseManagerIntegrationTest, GetLastError_EmptyOnSuccess) {
    EXPECT_TRUE(db_->getLastError().empty());
}

// Simulation CRUD tests
TEST_F(DatabaseManagerIntegrationTest, CreateSimulation_ReturnsValidId) {
    int id = db_->createSimulation("Test Sim", "Description", 1, "{}");
    EXPECT_GT(id, 0);
}

TEST_F(DatabaseManagerIntegrationTest, CreateSimulation_MultipleSims) {
    int id1 = db_->createSimulation("Sim 1", "Desc 1", 1, "{}");
    int id2 = db_->createSimulation("Sim 2", "Desc 2", 1, "{}");

    EXPECT_NE(id1, id2);
    EXPECT_GT(id2, id1);
}

TEST_F(DatabaseManagerIntegrationTest, GetSimulation_ReturnsCorrectData) {
    int id = db_->createSimulation("My Sim", "My Description", 42, "{\"key\":\"value\"}");

    DatabaseManager::SimulationRecord record = db_->getSimulation(id);

    EXPECT_EQ(record.id, id);
    EXPECT_EQ(record.name, "My Sim");
    EXPECT_EQ(record.description, "My Description");
    EXPECT_EQ(record.network_id, 42);
}

TEST_F(DatabaseManagerIntegrationTest, UpdateSimulationStatus) {
    int id = db_->createSimulation("Test", "Desc", 1, "{}");

    bool result = db_->updateSimulationStatus(id, "running");
    EXPECT_TRUE(result);

    DatabaseManager::SimulationRecord record = db_->getSimulation(id);
    EXPECT_EQ(record.status, "running");
}

TEST_F(DatabaseManagerIntegrationTest, CompleteSimulation) {
    int id = db_->createSimulation("Test", "Desc", 1, "{}");
    db_->updateSimulationStatus(id, "running");

    long endTime = currentTimestampMs();
    bool result = db_->completeSimulation(id, endTime, 120.5);
    EXPECT_TRUE(result);

    DatabaseManager::SimulationRecord record = db_->getSimulation(id);
    EXPECT_EQ(record.status, "completed");
}

TEST_F(DatabaseManagerIntegrationTest, GetAllSimulations_Empty) {
    auto sims = db_->getAllSimulations();
    EXPECT_TRUE(sims.empty());
}

TEST_F(DatabaseManagerIntegrationTest, GetAllSimulations_ReturnsList) {
    db_->createSimulation("Sim 1", "", 1, "{}");
    db_->createSimulation("Sim 2", "", 1, "{}");
    db_->createSimulation("Sim 3", "", 1, "{}");

    auto sims = db_->getAllSimulations();
    EXPECT_EQ(sims.size(), 3);
}

// TODO: Implement getSimulationsByStatus in DatabaseManager
// TEST_F(DatabaseManagerIntegrationTest, GetSimulationsByStatus) {
//     int id1 = db_->createSimulation("Running Sim", "", 1, "{}");
//     int id2 = db_->createSimulation("Completed Sim", "", 1, "{}");
//     db_->updateSimulationStatus(id1, "running");
//     db_->updateSimulationStatus(id2, "completed");
//     auto running = db_->getSimulationsByStatus("running");
//     auto completed = db_->getSimulationsByStatus("completed");
//     EXPECT_EQ(running.size(), 1);
//     EXPECT_EQ(completed.size(), 1);
// }

// Network CRUD tests
TEST_F(DatabaseManagerIntegrationTest, CreateNetwork_ReturnsValidId) {
    int id = db_->createNetwork("Test Network", "Description", 10, 5, "{}");
    EXPECT_GT(id, 0);
}

TEST_F(DatabaseManagerIntegrationTest, GetNetwork_ReturnsCorrectData) {
    int id = db_->createNetwork("My Network", "Network desc", 20, 8, "{}");

    DatabaseManager::NetworkRecord record = db_->getNetwork(id);

    EXPECT_EQ(record.id, id);
    EXPECT_EQ(record.name, "My Network");
    EXPECT_EQ(record.road_count, 20);
    EXPECT_EQ(record.intersection_count, 8);
}

TEST_F(DatabaseManagerIntegrationTest, GetAllNetworks) {
    db_->createNetwork("Net 1", "", 5, 2, "{}");
    db_->createNetwork("Net 2", "", 10, 4, "{}");

    auto networks = db_->getAllNetworks();
    EXPECT_EQ(networks.size(), 2);
}

// TODO: Implement deleteNetwork in DatabaseManager
// TEST_F(DatabaseManagerIntegrationTest, DeleteNetwork) {
//     int id = db_->createNetwork("To Delete", "", 5, 2, "{}");
//     EXPECT_EQ(db_->getAllNetworks().size(), 1);
//     bool result = db_->deleteNetwork(id);
//     EXPECT_TRUE(result);
//     EXPECT_TRUE(db_->getAllNetworks().empty());
// }

// Metrics tests
TEST_F(DatabaseManagerIntegrationTest, InsertMetric) {
    int simId = db_->createSimulation("Test", "", 1, "{}");

    bool result = db_->insertMetric(simId, 1.0, "queue_length", 1, 5.5, "meters", "{}");
    EXPECT_TRUE(result);
}

TEST_F(DatabaseManagerIntegrationTest, GetMetrics) {
    int simId = db_->createSimulation("Test", "", 1, "{}");

    db_->insertMetric(simId, 1.0, "queue_length", 1, 5.5);
    db_->insertMetric(simId, 2.0, "queue_length", 1, 6.0);
    db_->insertMetric(simId, 3.0, "avg_speed", 1, 15.0);

    auto metrics = db_->getMetrics(simId);
    EXPECT_EQ(metrics.size(), 3);
}

TEST_F(DatabaseManagerIntegrationTest, GetMetricsByType) {
    int simId = db_->createSimulation("Test", "", 1, "{}");

    db_->insertMetric(simId, 1.0, "queue_length", 1, 5.5);
    db_->insertMetric(simId, 2.0, "queue_length", 1, 6.0);
    db_->insertMetric(simId, 3.0, "avg_speed", 1, 15.0);

    auto queueMetrics = db_->getMetricsByType(simId, "queue_length");
    EXPECT_EQ(queueMetrics.size(), 2);

    auto speedMetrics = db_->getMetricsByType(simId, "avg_speed");
    EXPECT_EQ(speedMetrics.size(), 1);
}

// TODO: Implement getMetricsTimeRange in DatabaseManager
// TEST_F(DatabaseManagerIntegrationTest, GetMetricsTimeRange) {
//     int simId = db_->createSimulation("Test", "", 1, "{}");
//     db_->insertMetric(simId, 1.0, "queue_length", 1, 5.5);
//     db_->insertMetric(simId, 5.0, "queue_length", 1, 6.0);
//     db_->insertMetric(simId, 10.0, "queue_length", 1, 7.0);
//     auto metrics = db_->getMetricsTimeRange(simId, 2.0, 8.0);
//     EXPECT_EQ(metrics.size(), 1);  // Only the one at t=5.0
// }

// Optimization run tests
TEST_F(DatabaseManagerIntegrationTest, CreateOptimizationRun) {
    auto record = createTestOptimizationRun();
    int id = db_->createOptimizationRun(record);
    EXPECT_GT(id, 0);
}

TEST_F(DatabaseManagerIntegrationTest, GetOptimizationRun) {
    auto record = createTestOptimizationRun();
    record.population_size = 75;
    record.generations = 150;

    int id = db_->createOptimizationRun(record);
    auto retrieved = db_->getOptimizationRun(id);

    EXPECT_EQ(retrieved.id, id);
    EXPECT_EQ(retrieved.population_size, 75);
    EXPECT_EQ(retrieved.generations, 150);
}

TEST_F(DatabaseManagerIntegrationTest, UpdateOptimizationRunStatus) {
    auto record = createTestOptimizationRun();
    int id = db_->createOptimizationRun(record);

    bool result = db_->updateOptimizationRunStatus(id, "running");
    EXPECT_TRUE(result);

    auto retrieved = db_->getOptimizationRun(id);
    EXPECT_EQ(retrieved.status, "running");
}

TEST_F(DatabaseManagerIntegrationTest, CompleteOptimizationRun) {
    auto record = createTestOptimizationRun();
    int id = db_->createOptimizationRun(record);

    long completedAt = currentTimestampMs();
    bool result = db_->completeOptimizationRun(id, completedAt, 300, 100.0, 50.0, 50.0);
    EXPECT_TRUE(result);

    auto retrieved = db_->getOptimizationRun(id);
    EXPECT_EQ(retrieved.status, "completed");
    EXPECT_DOUBLE_EQ(retrieved.baseline_fitness, 100.0);
    EXPECT_DOUBLE_EQ(retrieved.best_fitness, 50.0);
}

// Optimization generation tests
TEST_F(DatabaseManagerIntegrationTest, InsertOptimizationGeneration) {
    auto runRecord = createTestOptimizationRun();
    int runId = db_->createOptimizationRun(runRecord);

    DatabaseManager::OptimizationGenerationRecord gen;
    gen.optimization_run_id = runId;
    gen.generation_number = 1;
    gen.best_fitness = 100.0;
    gen.average_fitness = 150.0;
    gen.worst_fitness = 200.0;
    gen.timestamp = currentTimestampMs();

    bool result = db_->insertOptimizationGeneration(gen);
    EXPECT_TRUE(result);
}

TEST_F(DatabaseManagerIntegrationTest, GetOptimizationGenerations) {
    auto runRecord = createTestOptimizationRun();
    int runId = db_->createOptimizationRun(runRecord);

    // Insert several generations
    for (int i = 0; i < 5; i++) {
        DatabaseManager::OptimizationGenerationRecord gen;
        gen.optimization_run_id = runId;
        gen.generation_number = i;
        gen.best_fitness = 100.0 - i * 10;
        gen.average_fitness = 150.0 - i * 10;
        gen.worst_fitness = 200.0 - i * 10;
        gen.timestamp = currentTimestampMs();
        db_->insertOptimizationGeneration(gen);
    }

    auto generations = db_->getOptimizationGenerations(runId);
    EXPECT_EQ(generations.size(), 5);
}

// Traffic snapshot tests
TEST_F(DatabaseManagerIntegrationTest, InsertTrafficSnapshot) {
    DatabaseManager::TrafficSnapshotRecord snap;
    snap.timestamp = currentTimestampMs();
    snap.road_id = 1;
    snap.vehicle_count = 10;
    snap.queue_length = 5.5;
    snap.avg_speed = 12.0;
    snap.flow_rate = 20.0;

    bool result = db_->insertTrafficSnapshot(snap);
    EXPECT_TRUE(result);
}

TEST_F(DatabaseManagerIntegrationTest, GetTrafficSnapshots) {
    int64_t startTime = currentTimestampMs();

    for (int i = 0; i < 3; i++) {
        DatabaseManager::TrafficSnapshotRecord snap;
        snap.timestamp = startTime + i * 1000;
        snap.road_id = 1;
        snap.vehicle_count = 10 + i;
        snap.queue_length = 5.0;
        snap.avg_speed = 12.0;
        snap.flow_rate = 20.0;
        db_->insertTrafficSnapshot(snap);
    }

    auto snapshots = db_->getTrafficSnapshots(startTime - 1000);
    EXPECT_EQ(snapshots.size(), 3);
}

// Traffic pattern tests
TEST_F(DatabaseManagerIntegrationTest, InsertOrUpdateTrafficPattern) {
    auto pattern = createTestPattern(1, 1, 12);
    bool result = db_->insertOrUpdateTrafficPattern(pattern);
    EXPECT_TRUE(result);
}

TEST_F(DatabaseManagerIntegrationTest, GetTrafficPattern) {
    auto pattern = createTestPattern(1, 2, 14, 15.0);
    db_->insertOrUpdateTrafficPattern(pattern);

    auto retrieved = db_->getTrafficPattern(1, 2, 14);
    EXPECT_EQ(retrieved.road_id, 1);
    EXPECT_EQ(retrieved.day_of_week, 2);
    EXPECT_EQ(retrieved.time_slot, 14);
    EXPECT_DOUBLE_EQ(retrieved.avg_vehicle_count, 15.0);
}

TEST_F(DatabaseManagerIntegrationTest, GetAllTrafficPatterns) {
    db_->insertOrUpdateTrafficPattern(createTestPattern(1, 0, 10));
    db_->insertOrUpdateTrafficPattern(createTestPattern(2, 0, 10));
    db_->insertOrUpdateTrafficPattern(createTestPattern(1, 1, 10));

    auto patterns = db_->getAllTrafficPatterns();
    EXPECT_EQ(patterns.size(), 3);
}

// Profile tests
TEST_F(DatabaseManagerIntegrationTest, CreateProfile) {
    int id = db_->createProfile("Rush Hour", "Morning rush hour settings");
    EXPECT_GT(id, 0);
}

TEST_F(DatabaseManagerIntegrationTest, GetProfile) {
    int id = db_->createProfile("Test Profile", "Description");

    auto profile = db_->getProfile(id);
    EXPECT_EQ(profile.id, id);
    EXPECT_EQ(profile.name, "Test Profile");
    EXPECT_EQ(profile.description, "Description");
}

TEST_F(DatabaseManagerIntegrationTest, GetProfileByName) {
    db_->createProfile("Unique Name", "Desc");

    auto profile = db_->getProfileByName("Unique Name");
    EXPECT_EQ(profile.name, "Unique Name");
}

TEST_F(DatabaseManagerIntegrationTest, GetAllProfiles) {
    db_->createProfile("Profile 1", "");
    db_->createProfile("Profile 2", "");
    db_->createProfile("Profile 3", "");

    auto profiles = db_->getAllProfiles();
    EXPECT_EQ(profiles.size(), 3);
}

TEST_F(DatabaseManagerIntegrationTest, UpdateProfile) {
    int id = db_->createProfile("Original", "Old desc");

    bool result = db_->updateProfile(id, "Updated", "New desc");
    EXPECT_TRUE(result);

    auto profile = db_->getProfile(id);
    EXPECT_EQ(profile.name, "Updated");
    EXPECT_EQ(profile.description, "New desc");
}

TEST_F(DatabaseManagerIntegrationTest, DeleteProfile) {
    int id = db_->createProfile("To Delete", "");
    EXPECT_EQ(db_->getAllProfiles().size(), 1);

    bool result = db_->deleteProfile(id);
    EXPECT_TRUE(result);
    EXPECT_TRUE(db_->getAllProfiles().empty());
}

TEST_F(DatabaseManagerIntegrationTest, SetActiveProfile) {
    int id1 = db_->createProfile("Profile 1", "");
    int id2 = db_->createProfile("Profile 2", "");

    db_->setActiveProfile(id1);
    auto active = db_->getActiveProfile();
    EXPECT_EQ(active.id, id1);

    db_->setActiveProfile(id2);
    active = db_->getActiveProfile();
    EXPECT_EQ(active.id, id2);
}

// Analytics tests
TEST_F(DatabaseManagerIntegrationTest, GetMetricStatistics) {
    int simId = db_->createSimulation("Test", "", 1, "{}");

    // Insert varied metrics
    for (int i = 0; i < 10; i++) {
        db_->insertMetric(simId, i * 1.0, "queue_length", 1, 5.0 + i);
    }

    auto stats = db_->getMetricStatistics(simId, "queue_length");

    EXPECT_EQ(stats.sample_count, 10);
    EXPECT_DOUBLE_EQ(stats.min_value, 5.0);
    EXPECT_DOUBLE_EQ(stats.max_value, 14.0);
}

TEST_F(DatabaseManagerIntegrationTest, GetAllMetricStatistics) {
    int simId = db_->createSimulation("Test", "", 1, "{}");

    db_->insertMetric(simId, 1.0, "queue_length", 1, 5.0);
    db_->insertMetric(simId, 2.0, "avg_speed", 1, 15.0);

    auto allStats = db_->getAllMetricStatistics(simId);

    EXPECT_EQ(allStats.size(), 2);
    EXPECT_TRUE(allStats.find("queue_length") != allStats.end());
    EXPECT_TRUE(allStats.find("avg_speed") != allStats.end());
}
