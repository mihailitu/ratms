#include "database_manager.h"
#include "../../utils/logger.h"
#include <fstream>
#include <sstream>
#include <ctime>

namespace ratms {
namespace data {

using namespace ratms;

DatabaseManager::DatabaseManager(const std::string& db_path)
    : db_(nullptr), db_path_(db_path) {
}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::initialize() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to open database: {}", last_error_);
        return false;
    }

    LOG_INFO(LogComponent::Database, "Database opened: {}", db_path_);
    return true;
}

bool DatabaseManager::runMigrations(const std::string& migrations_dir) {
    TIMED_SCOPE(LogComponent::Database, "database_migrations");

    // Run migration 001
    std::string migration_file_001 = migrations_dir + "/001_initial_schema.sql";
    LOG_INFO(LogComponent::Database, "Running database migration: {}", migration_file_001);

    if (!executeSQLFile(migration_file_001)) {
        LOG_ERROR(LogComponent::Database, "Migration 001 failed: {}", last_error_);
        return false;
    }

    // Run migration 002 (optimization runs)
    std::string migration_file_002 = migrations_dir + "/002_optimization_runs.sql";
    LOG_INFO(LogComponent::Database, "Running database migration: {}", migration_file_002);

    if (!executeSQLFile(migration_file_002)) {
        LOG_ERROR(LogComponent::Database, "Migration 002 failed: {}", last_error_);
        return false;
    }

    // Run migration 003 (traffic profiles)
    std::string migration_file_003 = migrations_dir + "/003_traffic_profiles.sql";
    LOG_INFO(LogComponent::Database, "Running database migration: {}", migration_file_003);

    if (!executeSQLFile(migration_file_003)) {
        LOG_ERROR(LogComponent::Database, "Migration 003 failed: {}", last_error_);
        return false;
    }

    // Run migration 004 (travel times)
    std::string migration_file_004 = migrations_dir + "/004_travel_times.sql";
    LOG_INFO(LogComponent::Database, "Running database migration: {}", migration_file_004);

    if (!executeSQLFile(migration_file_004)) {
        LOG_ERROR(LogComponent::Database, "Migration 004 failed: {}", last_error_);
        return false;
    }

    // Run migration 005 (traffic patterns for prediction)
    std::string migration_file_005 = migrations_dir + "/005_traffic_patterns.sql";
    LOG_INFO(LogComponent::Database, "Running database migration: {}", migration_file_005);

    if (!executeSQLFile(migration_file_005)) {
        LOG_ERROR(LogComponent::Database, "Migration 005 failed: {}", last_error_);
        return false;
    }

    // Run migration 006 (profile traffic lights)
    std::string migration_file_006 = migrations_dir + "/006_profile_traffic_lights.sql";
    LOG_INFO(LogComponent::Database, "Running database migration: {}", migration_file_006);

    if (!executeSQLFile(migration_file_006)) {
        LOG_ERROR(LogComponent::Database, "Migration 006 failed: {}", last_error_);
        return false;
    }

    LOG_INFO(LogComponent::Database, "Database migrations completed successfully");
    return true;
}

bool DatabaseManager::executeSQL(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        last_error_ = err_msg ? err_msg : "Unknown error";
        LOG_ERROR(LogComponent::Database, "SQL execution failed: {}", last_error_);
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }

    return true;
}

bool DatabaseManager::executeSQLFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        last_error_ = "Could not open file: " + file_path;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    return executeSQL(sql);
}

void DatabaseManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        LOG_INFO(LogComponent::Database, "Database closed");
    }
}

// Simulation operations
int DatabaseManager::createSimulation(const std::string& name, const std::string& description,
                                     int network_id, const std::string& config_json) {
    std::string sql = "INSERT INTO simulations (name, description, network_id, status, "
                     "start_time, config_json) VALUES (?, ?, ?, 'pending', ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to prepare statement: {}", last_error_);
        return -1;
    }

    long current_time = std::time(nullptr);

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, network_id);
    sqlite3_bind_int64(stmt, 4, current_time);
    sqlite3_bind_text(stmt, 5, config_json.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to insert simulation: {}", last_error_);
        return -1;
    }

    int sim_id = sqlite3_last_insert_rowid(db_);
    LOG_DEBUG(LogComponent::Database, "Created simulation with ID: {}", sim_id);
    return sim_id;
}

bool DatabaseManager::updateSimulationStatus(int sim_id, const std::string& status) {
    std::string sql = "UPDATE simulations SET status = ?, updated_at = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    long current_time = std::time(nullptr);

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, current_time);
    sqlite3_bind_int(stmt, 3, sim_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    LOG_DEBUG(LogComponent::Database, "Updated simulation {} status to: {}", sim_id, status);
    return true;
}

bool DatabaseManager::completeSimulation(int sim_id, long end_time, double duration) {
    std::string sql = "UPDATE simulations SET status = 'completed', end_time = ?, "
                     "duration_seconds = ?, updated_at = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    long current_time = std::time(nullptr);

    sqlite3_bind_int64(stmt, 1, end_time);
    sqlite3_bind_double(stmt, 2, duration);
    sqlite3_bind_int64(stmt, 3, current_time);
    sqlite3_bind_int(stmt, 4, sim_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

DatabaseManager::SimulationRecord DatabaseManager::getSimulation(int sim_id) {
    std::string sql = "SELECT id, name, description, network_id, status, start_time, "
                     "end_time, duration_seconds, config_json FROM simulations WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    SimulationRecord record = {};

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return record;
    }

    sqlite3_bind_int(stmt, 1, sim_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.network_id = sqlite3_column_int(stmt, 3);
        record.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.start_time = sqlite3_column_int64(stmt, 5);
        record.end_time = sqlite3_column_int64(stmt, 6);
        record.duration_seconds = sqlite3_column_double(stmt, 7);
        record.config_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    }

    sqlite3_finalize(stmt);
    return record;
}

std::vector<DatabaseManager::SimulationRecord> DatabaseManager::getAllSimulations() {
    std::vector<SimulationRecord> results;
    std::string sql = "SELECT id, name, description, network_id, status, start_time, "
                     "end_time, duration_seconds, config_json FROM simulations "
                     "ORDER BY created_at DESC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SimulationRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.network_id = sqlite3_column_int(stmt, 3);
        record.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.start_time = sqlite3_column_int64(stmt, 5);
        record.end_time = sqlite3_column_int64(stmt, 6);
        record.duration_seconds = sqlite3_column_double(stmt, 7);
        record.config_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

// Metrics operations
bool DatabaseManager::insertMetric(int simulation_id, double timestamp, const std::string& metric_type,
                                  int road_id, double value, const std::string& unit,
                                  const std::string& metadata_json) {
    std::string sql = "INSERT INTO metrics (simulation_id, timestamp, metric_type, road_id, "
                     "value, unit, metadata_json) VALUES (?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int(stmt, 1, simulation_id);
    sqlite3_bind_double(stmt, 2, timestamp);
    sqlite3_bind_text(stmt, 3, metric_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, road_id);
    sqlite3_bind_double(stmt, 5, value);
    sqlite3_bind_text(stmt, 6, unit.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, metadata_json.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

std::vector<DatabaseManager::MetricRecord> DatabaseManager::getMetrics(int simulation_id) {
    std::vector<MetricRecord> results;
    std::string sql = "SELECT id, simulation_id, timestamp, metric_type, road_id, value, "
                     "unit, metadata_json FROM metrics WHERE simulation_id = ? "
                     "ORDER BY timestamp ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, simulation_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MetricRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.simulation_id = sqlite3_column_int(stmt, 1);
        record.timestamp = sqlite3_column_double(stmt, 2);
        record.metric_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.road_id = sqlite3_column_int(stmt, 4);
        record.value = sqlite3_column_double(stmt, 5);
        record.unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.metadata_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<DatabaseManager::MetricRecord> DatabaseManager::getMetricsByType(int simulation_id, const std::string& metric_type) {
    std::vector<MetricRecord> results;
    std::string sql = "SELECT id, simulation_id, timestamp, metric_type, road_id, value, "
                     "unit, metadata_json FROM metrics WHERE simulation_id = ? AND metric_type = ? "
                     "ORDER BY timestamp ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, simulation_id);
    sqlite3_bind_text(stmt, 2, metric_type.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MetricRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.simulation_id = sqlite3_column_int(stmt, 1);
        record.timestamp = sqlite3_column_double(stmt, 2);
        record.metric_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.road_id = sqlite3_column_int(stmt, 4);
        record.value = sqlite3_column_double(stmt, 5);
        record.unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.metadata_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

// Network operations
int DatabaseManager::createNetwork(const std::string& name, const std::string& description,
                                  int road_count, int intersection_count, const std::string& config_json) {
    std::string sql = "INSERT INTO networks (name, description, road_count, intersection_count, "
                     "config_json) VALUES (?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, road_count);
    sqlite3_bind_int(stmt, 4, intersection_count);
    sqlite3_bind_text(stmt, 5, config_json.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return -1;
    }

    int network_id = sqlite3_last_insert_rowid(db_);
    LOG_DEBUG(LogComponent::Database, "Created network with ID: {}", network_id);
    return network_id;
}

DatabaseManager::NetworkRecord DatabaseManager::getNetwork(int network_id) {
    std::string sql = "SELECT id, name, description, road_count, intersection_count, "
                     "config_json FROM networks WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    NetworkRecord record = {};

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return record;
    }

    sqlite3_bind_int(stmt, 1, network_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.road_count = sqlite3_column_int(stmt, 3);
        record.intersection_count = sqlite3_column_int(stmt, 4);
        record.config_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    }

    sqlite3_finalize(stmt);
    return record;
}

std::vector<DatabaseManager::NetworkRecord> DatabaseManager::getAllNetworks() {
    std::vector<NetworkRecord> results;
    std::string sql = "SELECT id, name, description, road_count, intersection_count, "
                     "config_json FROM networks ORDER BY created_at DESC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        NetworkRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.road_count = sqlite3_column_int(stmt, 3);
        record.intersection_count = sqlite3_column_int(stmt, 4);
        record.config_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

// Optimization operations
int DatabaseManager::createOptimizationRun(const OptimizationRunRecord& record) {
    std::string sql = "INSERT INTO optimization_runs (network_id, status, population_size, "
                     "generations, mutation_rate, crossover_rate, elitism_rate, "
                     "min_green_time, max_green_time, min_red_time, max_red_time, "
                     "simulation_steps, dt, started_at, created_by, notes) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to prepare optimization run insert: {}", last_error_);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, record.network_id);
    sqlite3_bind_text(stmt, 2, record.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, record.population_size);
    sqlite3_bind_int(stmt, 4, record.generations);
    sqlite3_bind_double(stmt, 5, record.mutation_rate);
    sqlite3_bind_double(stmt, 6, record.crossover_rate);
    sqlite3_bind_double(stmt, 7, record.elitism_rate);
    sqlite3_bind_double(stmt, 8, record.min_green_time);
    sqlite3_bind_double(stmt, 9, record.max_green_time);
    sqlite3_bind_double(stmt, 10, record.min_red_time);
    sqlite3_bind_double(stmt, 11, record.max_red_time);
    sqlite3_bind_int(stmt, 12, record.simulation_steps);
    sqlite3_bind_double(stmt, 13, record.dt);
    sqlite3_bind_int64(stmt, 14, record.started_at);
    sqlite3_bind_text(stmt, 15, record.created_by.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 16, record.notes.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to insert optimization run: {}", last_error_);
        return -1;
    }

    int run_id = sqlite3_last_insert_rowid(db_);
    LOG_DEBUG(LogComponent::Database, "Created optimization run with ID: {}", run_id);
    return run_id;
}

bool DatabaseManager::updateOptimizationRunStatus(int run_id, const std::string& status) {
    std::string sql = "UPDATE optimization_runs SET status = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, run_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    LOG_DEBUG(LogComponent::Database, "Updated optimization run {} status to: {}", run_id, status);
    return true;
}

bool DatabaseManager::completeOptimizationRun(int run_id, long completed_at, long duration_seconds,
                                             double baseline_fitness, double best_fitness,
                                             double improvement_percent) {
    std::string sql = "UPDATE optimization_runs SET status = 'completed', completed_at = ?, "
                     "duration_seconds = ?, baseline_fitness = ?, best_fitness = ?, "
                     "improvement_percent = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int64(stmt, 1, completed_at);
    sqlite3_bind_int64(stmt, 2, duration_seconds);
    sqlite3_bind_double(stmt, 3, baseline_fitness);
    sqlite3_bind_double(stmt, 4, best_fitness);
    sqlite3_bind_double(stmt, 5, improvement_percent);
    sqlite3_bind_int(stmt, 6, run_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

DatabaseManager::OptimizationRunRecord DatabaseManager::getOptimizationRun(int run_id) {
    std::string sql = "SELECT id, network_id, status, population_size, generations, "
                     "mutation_rate, crossover_rate, elitism_rate, min_green_time, "
                     "max_green_time, min_red_time, max_red_time, simulation_steps, dt, "
                     "baseline_fitness, best_fitness, improvement_percent, started_at, "
                     "completed_at, duration_seconds, created_by, notes "
                     "FROM optimization_runs WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    OptimizationRunRecord record = {};

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return record;
    }

    sqlite3_bind_int(stmt, 1, run_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.network_id = sqlite3_column_int(stmt, 1);
        record.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.population_size = sqlite3_column_int(stmt, 3);
        record.generations = sqlite3_column_int(stmt, 4);
        record.mutation_rate = sqlite3_column_double(stmt, 5);
        record.crossover_rate = sqlite3_column_double(stmt, 6);
        record.elitism_rate = sqlite3_column_double(stmt, 7);
        record.min_green_time = sqlite3_column_double(stmt, 8);
        record.max_green_time = sqlite3_column_double(stmt, 9);
        record.min_red_time = sqlite3_column_double(stmt, 10);
        record.max_red_time = sqlite3_column_double(stmt, 11);
        record.simulation_steps = sqlite3_column_int(stmt, 12);
        record.dt = sqlite3_column_double(stmt, 13);
        record.baseline_fitness = sqlite3_column_double(stmt, 14);
        record.best_fitness = sqlite3_column_double(stmt, 15);
        record.improvement_percent = sqlite3_column_double(stmt, 16);
        record.started_at = sqlite3_column_int64(stmt, 17);
        record.completed_at = sqlite3_column_int64(stmt, 18);
        record.duration_seconds = sqlite3_column_int64(stmt, 19);

        const char* created_by_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20));
        if (created_by_text) record.created_by = created_by_text;

        const char* notes_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21));
        if (notes_text) record.notes = notes_text;
    }

    sqlite3_finalize(stmt);
    return record;
}

std::vector<DatabaseManager::OptimizationRunRecord> DatabaseManager::getAllOptimizationRuns() {
    std::vector<OptimizationRunRecord> results;
    std::string sql = "SELECT id, network_id, status, population_size, generations, "
                     "mutation_rate, crossover_rate, elitism_rate, min_green_time, "
                     "max_green_time, min_red_time, max_red_time, simulation_steps, dt, "
                     "baseline_fitness, best_fitness, improvement_percent, started_at, "
                     "completed_at, duration_seconds, created_by, notes "
                     "FROM optimization_runs ORDER BY started_at DESC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OptimizationRunRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.network_id = sqlite3_column_int(stmt, 1);
        record.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.population_size = sqlite3_column_int(stmt, 3);
        record.generations = sqlite3_column_int(stmt, 4);
        record.mutation_rate = sqlite3_column_double(stmt, 5);
        record.crossover_rate = sqlite3_column_double(stmt, 6);
        record.elitism_rate = sqlite3_column_double(stmt, 7);
        record.min_green_time = sqlite3_column_double(stmt, 8);
        record.max_green_time = sqlite3_column_double(stmt, 9);
        record.min_red_time = sqlite3_column_double(stmt, 10);
        record.max_red_time = sqlite3_column_double(stmt, 11);
        record.simulation_steps = sqlite3_column_int(stmt, 12);
        record.dt = sqlite3_column_double(stmt, 13);
        record.baseline_fitness = sqlite3_column_double(stmt, 14);
        record.best_fitness = sqlite3_column_double(stmt, 15);
        record.improvement_percent = sqlite3_column_double(stmt, 16);
        record.started_at = sqlite3_column_int64(stmt, 17);
        record.completed_at = sqlite3_column_int64(stmt, 18);
        record.duration_seconds = sqlite3_column_int64(stmt, 19);

        const char* created_by_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20));
        if (created_by_text) record.created_by = created_by_text;

        const char* notes_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21));
        if (notes_text) record.notes = notes_text;

        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<DatabaseManager::OptimizationRunRecord> DatabaseManager::getOptimizationRunsByStatus(const std::string& status) {
    std::vector<OptimizationRunRecord> results;
    std::string sql = "SELECT id, network_id, status, population_size, generations, "
                     "mutation_rate, crossover_rate, elitism_rate, min_green_time, "
                     "max_green_time, min_red_time, max_red_time, simulation_steps, dt, "
                     "baseline_fitness, best_fitness, improvement_percent, started_at, "
                     "completed_at, duration_seconds, created_by, notes "
                     "FROM optimization_runs WHERE status = ? ORDER BY started_at DESC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OptimizationRunRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.network_id = sqlite3_column_int(stmt, 1);
        record.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.population_size = sqlite3_column_int(stmt, 3);
        record.generations = sqlite3_column_int(stmt, 4);
        record.mutation_rate = sqlite3_column_double(stmt, 5);
        record.crossover_rate = sqlite3_column_double(stmt, 6);
        record.elitism_rate = sqlite3_column_double(stmt, 7);
        record.min_green_time = sqlite3_column_double(stmt, 8);
        record.max_green_time = sqlite3_column_double(stmt, 9);
        record.min_red_time = sqlite3_column_double(stmt, 10);
        record.max_red_time = sqlite3_column_double(stmt, 11);
        record.simulation_steps = sqlite3_column_int(stmt, 12);
        record.dt = sqlite3_column_double(stmt, 13);
        record.baseline_fitness = sqlite3_column_double(stmt, 14);
        record.best_fitness = sqlite3_column_double(stmt, 15);
        record.improvement_percent = sqlite3_column_double(stmt, 16);
        record.started_at = sqlite3_column_int64(stmt, 17);
        record.completed_at = sqlite3_column_int64(stmt, 18);
        record.duration_seconds = sqlite3_column_int64(stmt, 19);

        const char* created_by_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20));
        if (created_by_text) record.created_by = created_by_text;

        const char* notes_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21));
        if (notes_text) record.notes = notes_text;

        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

// Optimization generation operations
bool DatabaseManager::insertOptimizationGeneration(const OptimizationGenerationRecord& record) {
    std::string sql = "INSERT INTO optimization_generations (optimization_run_id, generation_number, "
                     "best_fitness, average_fitness, worst_fitness, timestamp) "
                     "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int(stmt, 1, record.optimization_run_id);
    sqlite3_bind_int(stmt, 2, record.generation_number);
    sqlite3_bind_double(stmt, 3, record.best_fitness);
    sqlite3_bind_double(stmt, 4, record.average_fitness);
    sqlite3_bind_double(stmt, 5, record.worst_fitness);
    sqlite3_bind_int64(stmt, 6, record.timestamp);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

bool DatabaseManager::insertOptimizationGenerationsBatch(const std::vector<OptimizationGenerationRecord>& records) {
    // Start transaction
    if (!executeSQL("BEGIN TRANSACTION")) {
        return false;
    }

    for (const auto& record : records) {
        if (!insertOptimizationGeneration(record)) {
            executeSQL("ROLLBACK");
            return false;
        }
    }

    if (!executeSQL("COMMIT")) {
        executeSQL("ROLLBACK");
        return false;
    }

    return true;
}

std::vector<DatabaseManager::OptimizationGenerationRecord> DatabaseManager::getOptimizationGenerations(int run_id) {
    std::vector<OptimizationGenerationRecord> results;
    std::string sql = "SELECT id, optimization_run_id, generation_number, best_fitness, "
                     "average_fitness, worst_fitness, timestamp FROM optimization_generations "
                     "WHERE optimization_run_id = ? ORDER BY generation_number ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, run_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OptimizationGenerationRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.optimization_run_id = sqlite3_column_int(stmt, 1);
        record.generation_number = sqlite3_column_int(stmt, 2);
        record.best_fitness = sqlite3_column_double(stmt, 3);
        record.average_fitness = sqlite3_column_double(stmt, 4);
        record.worst_fitness = sqlite3_column_double(stmt, 5);
        record.timestamp = sqlite3_column_int64(stmt, 6);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

// Optimization solution operations
int DatabaseManager::insertOptimizationSolution(const OptimizationSolutionRecord& record) {
    std::string sql = "INSERT INTO optimization_solutions (optimization_run_id, is_best_solution, "
                     "fitness, chromosome_json, traffic_light_count, created_at) "
                     "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to prepare solution insert: {}", last_error_);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, record.optimization_run_id);
    sqlite3_bind_int(stmt, 2, record.is_best_solution ? 1 : 0);
    sqlite3_bind_double(stmt, 3, record.fitness);
    sqlite3_bind_text(stmt, 4, record.chromosome_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, record.traffic_light_count);
    sqlite3_bind_int64(stmt, 6, record.created_at);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to insert optimization solution: {}", last_error_);
        return -1;
    }

    int solution_id = sqlite3_last_insert_rowid(db_);
    return solution_id;
}

DatabaseManager::OptimizationSolutionRecord DatabaseManager::getBestOptimizationSolution(int run_id) {
    std::string sql = "SELECT id, optimization_run_id, is_best_solution, fitness, "
                     "chromosome_json, traffic_light_count, created_at "
                     "FROM optimization_solutions WHERE optimization_run_id = ? "
                     "AND is_best_solution = 1 LIMIT 1";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    OptimizationSolutionRecord record = {};

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return record;
    }

    sqlite3_bind_int(stmt, 1, run_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.optimization_run_id = sqlite3_column_int(stmt, 1);
        record.is_best_solution = sqlite3_column_int(stmt, 2) == 1;
        record.fitness = sqlite3_column_double(stmt, 3);
        record.chromosome_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.traffic_light_count = sqlite3_column_int(stmt, 5);
        record.created_at = sqlite3_column_int64(stmt, 6);
    }

    sqlite3_finalize(stmt);
    return record;
}

std::vector<DatabaseManager::OptimizationSolutionRecord> DatabaseManager::getOptimizationSolutions(int run_id) {
    std::vector<OptimizationSolutionRecord> results;
    std::string sql = "SELECT id, optimization_run_id, is_best_solution, fitness, "
                     "chromosome_json, traffic_light_count, created_at "
                     "FROM optimization_solutions WHERE optimization_run_id = ? "
                     "ORDER BY fitness ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, run_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OptimizationSolutionRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.optimization_run_id = sqlite3_column_int(stmt, 1);
        record.is_best_solution = sqlite3_column_int(stmt, 2) == 1;
        record.fitness = sqlite3_column_double(stmt, 3);
        record.chromosome_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.traffic_light_count = sqlite3_column_int(stmt, 5);
        record.created_at = sqlite3_column_int64(stmt, 6);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

// Analytics operations
DatabaseManager::MetricStatistics DatabaseManager::getMetricStatistics(int simulation_id, const std::string& metric_type) {
    MetricStatistics stats;
    stats.metric_type = metric_type;
    stats.min_value = 0.0;
    stats.max_value = 0.0;
    stats.mean_value = 0.0;
    stats.median_value = 0.0;
    stats.stddev_value = 0.0;
    stats.p25_value = 0.0;
    stats.p75_value = 0.0;
    stats.p95_value = 0.0;
    stats.sample_count = 0;

    // Get basic statistics (min, max, mean, count, stddev)
    std::string sql_basic = "SELECT MIN(value), MAX(value), AVG(value), COUNT(*), "
                           "SQRT(MAX(0, AVG(value * value) - AVG(value) * AVG(value))) as stddev "
                           "FROM metrics WHERE simulation_id = ? AND metric_type = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql_basic.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return stats;
    }

    sqlite3_bind_int(stmt, 1, simulation_id);
    sqlite3_bind_text(stmt, 2, metric_type.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stats.min_value = sqlite3_column_double(stmt, 0);
        stats.max_value = sqlite3_column_double(stmt, 1);
        stats.mean_value = sqlite3_column_double(stmt, 2);
        stats.sample_count = sqlite3_column_int(stmt, 3);
        stats.stddev_value = sqlite3_column_double(stmt, 4);
    }

    sqlite3_finalize(stmt);

    // If no data, return early
    if (stats.sample_count == 0) {
        return stats;
    }

    // Get percentiles (median, p25, p75, p95)
    // Median (50th percentile)
    std::string sql_median = "SELECT value FROM metrics WHERE simulation_id = ? AND metric_type = ? "
                            "ORDER BY value LIMIT 1 OFFSET ?";
    rc = sqlite3_prepare_v2(db_, sql_median.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, simulation_id);
        sqlite3_bind_text(stmt, 2, metric_type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, stats.sample_count / 2);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.median_value = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // 25th percentile
    rc = sqlite3_prepare_v2(db_, sql_median.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, simulation_id);
        sqlite3_bind_text(stmt, 2, metric_type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, stats.sample_count / 4);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.p25_value = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // 75th percentile
    rc = sqlite3_prepare_v2(db_, sql_median.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, simulation_id);
        sqlite3_bind_text(stmt, 2, metric_type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, (stats.sample_count * 3) / 4);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.p75_value = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // 95th percentile
    rc = sqlite3_prepare_v2(db_, sql_median.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, simulation_id);
        sqlite3_bind_text(stmt, 2, metric_type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, (stats.sample_count * 95) / 100);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.p95_value = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}

std::map<std::string, DatabaseManager::MetricStatistics> DatabaseManager::getAllMetricStatistics(int simulation_id) {
    std::map<std::string, MetricStatistics> all_stats;

    // Get all unique metric types for this simulation
    std::string sql = "SELECT DISTINCT metric_type FROM metrics WHERE simulation_id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return all_stats;
    }

    sqlite3_bind_int(stmt, 1, simulation_id);

    std::vector<std::string> metric_types;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        metric_types.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);

    // Get statistics for each metric type
    for (const auto& metric_type : metric_types) {
        all_stats[metric_type] = getMetricStatistics(simulation_id, metric_type);
    }

    return all_stats;
}

std::vector<DatabaseManager::ComparativeMetrics> DatabaseManager::getComparativeMetrics(
    const std::vector<int>& simulation_ids, const std::string& metric_type) {

    std::vector<ComparativeMetrics> results;

    for (int sim_id : simulation_ids) {
        ComparativeMetrics comp;
        comp.simulation_id = sim_id;

        // Get simulation name
        SimulationRecord sim = getSimulation(sim_id);
        comp.simulation_name = sim.name;

        // Get metrics for this simulation
        comp.metrics = getMetricsByType(sim_id, metric_type);

        results.push_back(comp);
    }

    return results;
}

// Traffic snapshot operations
bool DatabaseManager::insertTrafficSnapshot(const TrafficSnapshotRecord& record) {
    std::string sql = "INSERT INTO traffic_snapshots "
                     "(timestamp, road_id, vehicle_count, queue_length, avg_speed, flow_rate) "
                     "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to prepare traffic snapshot insert: {}", last_error_);
        return false;
    }

    sqlite3_bind_int64(stmt, 1, record.timestamp);
    sqlite3_bind_int(stmt, 2, record.road_id);
    sqlite3_bind_int(stmt, 3, record.vehicle_count);
    sqlite3_bind_double(stmt, 4, record.queue_length);
    sqlite3_bind_double(stmt, 5, record.avg_speed);
    sqlite3_bind_double(stmt, 6, record.flow_rate);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

bool DatabaseManager::insertTrafficSnapshotsBatch(const std::vector<TrafficSnapshotRecord>& records) {
    if (records.empty()) return true;

    if (!executeSQL("BEGIN TRANSACTION")) {
        return false;
    }

    for (const auto& record : records) {
        if (!insertTrafficSnapshot(record)) {
            executeSQL("ROLLBACK");
            return false;
        }
    }

    if (!executeSQL("COMMIT")) {
        executeSQL("ROLLBACK");
        return false;
    }

    LOG_DEBUG(LogComponent::Database, "Inserted {} traffic snapshots", records.size());
    return true;
}

std::vector<DatabaseManager::TrafficSnapshotRecord> DatabaseManager::getTrafficSnapshots(int64_t since_timestamp) {
    std::vector<TrafficSnapshotRecord> results;
    std::string sql = "SELECT id, timestamp, road_id, vehicle_count, queue_length, avg_speed, flow_rate "
                     "FROM traffic_snapshots WHERE timestamp >= ? ORDER BY timestamp ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int64(stmt, 1, since_timestamp);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrafficSnapshotRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.timestamp = sqlite3_column_int64(stmt, 1);
        record.road_id = sqlite3_column_int(stmt, 2);
        record.vehicle_count = sqlite3_column_int(stmt, 3);
        record.queue_length = sqlite3_column_double(stmt, 4);
        record.avg_speed = sqlite3_column_double(stmt, 5);
        record.flow_rate = sqlite3_column_double(stmt, 6);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<DatabaseManager::TrafficSnapshotRecord> DatabaseManager::getTrafficSnapshotsForRoad(
    int road_id, int64_t since_timestamp) {
    std::vector<TrafficSnapshotRecord> results;
    std::string sql = "SELECT id, timestamp, road_id, vehicle_count, queue_length, avg_speed, flow_rate "
                     "FROM traffic_snapshots WHERE road_id = ? AND timestamp >= ? ORDER BY timestamp ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, road_id);
    sqlite3_bind_int64(stmt, 2, since_timestamp);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrafficSnapshotRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.timestamp = sqlite3_column_int64(stmt, 1);
        record.road_id = sqlite3_column_int(stmt, 2);
        record.vehicle_count = sqlite3_column_int(stmt, 3);
        record.queue_length = sqlite3_column_double(stmt, 4);
        record.avg_speed = sqlite3_column_double(stmt, 5);
        record.flow_rate = sqlite3_column_double(stmt, 6);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<DatabaseManager::TrafficSnapshotRecord> DatabaseManager::getTrafficSnapshotsRange(
    int64_t start_time, int64_t end_time) {
    std::vector<TrafficSnapshotRecord> results;
    std::string sql = "SELECT id, timestamp, road_id, vehicle_count, queue_length, avg_speed, flow_rate "
                     "FROM traffic_snapshots WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int64(stmt, 1, start_time);
    sqlite3_bind_int64(stmt, 2, end_time);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrafficSnapshotRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.timestamp = sqlite3_column_int64(stmt, 1);
        record.road_id = sqlite3_column_int(stmt, 2);
        record.vehicle_count = sqlite3_column_int(stmt, 3);
        record.queue_length = sqlite3_column_double(stmt, 4);
        record.avg_speed = sqlite3_column_double(stmt, 5);
        record.flow_rate = sqlite3_column_double(stmt, 6);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

int DatabaseManager::deleteTrafficSnapshotsBefore(int64_t timestamp) {
    std::string sql = "DELETE FROM traffic_snapshots WHERE timestamp < ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, timestamp);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return -1;
    }

    int deleted = sqlite3_changes(db_);
    LOG_INFO(LogComponent::Database, "Deleted {} traffic snapshots before timestamp {}", deleted, timestamp);
    return deleted;
}

// Traffic pattern operations
bool DatabaseManager::insertOrUpdateTrafficPattern(const TrafficPatternRecord& record) {
    std::string sql = "INSERT OR REPLACE INTO traffic_patterns "
                     "(road_id, day_of_week, time_slot, avg_vehicle_count, avg_queue_length, "
                     "avg_speed, avg_flow_rate, min_vehicle_count, max_vehicle_count, "
                     "stddev_vehicle_count, sample_count, last_updated) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        LOG_ERROR(LogComponent::Database, "Failed to prepare traffic pattern upsert: {}", last_error_);
        return false;
    }

    sqlite3_bind_int(stmt, 1, record.road_id);
    sqlite3_bind_int(stmt, 2, record.day_of_week);
    sqlite3_bind_int(stmt, 3, record.time_slot);
    sqlite3_bind_double(stmt, 4, record.avg_vehicle_count);
    sqlite3_bind_double(stmt, 5, record.avg_queue_length);
    sqlite3_bind_double(stmt, 6, record.avg_speed);
    sqlite3_bind_double(stmt, 7, record.avg_flow_rate);
    sqlite3_bind_double(stmt, 8, record.min_vehicle_count);
    sqlite3_bind_double(stmt, 9, record.max_vehicle_count);
    sqlite3_bind_double(stmt, 10, record.stddev_vehicle_count);
    sqlite3_bind_int(stmt, 11, record.sample_count);
    sqlite3_bind_int64(stmt, 12, record.last_updated);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

DatabaseManager::TrafficPatternRecord DatabaseManager::getTrafficPattern(
    int road_id, int day_of_week, int time_slot) {
    TrafficPatternRecord record = {};
    record.road_id = road_id;
    record.day_of_week = day_of_week;
    record.time_slot = time_slot;

    std::string sql = "SELECT id, road_id, day_of_week, time_slot, avg_vehicle_count, "
                     "avg_queue_length, avg_speed, avg_flow_rate, min_vehicle_count, "
                     "max_vehicle_count, stddev_vehicle_count, sample_count, last_updated "
                     "FROM traffic_patterns WHERE road_id = ? AND day_of_week = ? AND time_slot = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return record;
    }

    sqlite3_bind_int(stmt, 1, road_id);
    sqlite3_bind_int(stmt, 2, day_of_week);
    sqlite3_bind_int(stmt, 3, time_slot);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.road_id = sqlite3_column_int(stmt, 1);
        record.day_of_week = sqlite3_column_int(stmt, 2);
        record.time_slot = sqlite3_column_int(stmt, 3);
        record.avg_vehicle_count = sqlite3_column_double(stmt, 4);
        record.avg_queue_length = sqlite3_column_double(stmt, 5);
        record.avg_speed = sqlite3_column_double(stmt, 6);
        record.avg_flow_rate = sqlite3_column_double(stmt, 7);
        record.min_vehicle_count = sqlite3_column_double(stmt, 8);
        record.max_vehicle_count = sqlite3_column_double(stmt, 9);
        record.stddev_vehicle_count = sqlite3_column_double(stmt, 10);
        record.sample_count = sqlite3_column_int(stmt, 11);
        record.last_updated = sqlite3_column_int64(stmt, 12);
    }

    sqlite3_finalize(stmt);
    return record;
}

std::vector<DatabaseManager::TrafficPatternRecord> DatabaseManager::getTrafficPatterns(
    int day_of_week, int time_slot) {
    std::vector<TrafficPatternRecord> results;
    std::string sql = "SELECT id, road_id, day_of_week, time_slot, avg_vehicle_count, "
                     "avg_queue_length, avg_speed, avg_flow_rate, min_vehicle_count, "
                     "max_vehicle_count, stddev_vehicle_count, sample_count, last_updated "
                     "FROM traffic_patterns WHERE day_of_week = ? AND time_slot = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, day_of_week);
    sqlite3_bind_int(stmt, 2, time_slot);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrafficPatternRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.road_id = sqlite3_column_int(stmt, 1);
        record.day_of_week = sqlite3_column_int(stmt, 2);
        record.time_slot = sqlite3_column_int(stmt, 3);
        record.avg_vehicle_count = sqlite3_column_double(stmt, 4);
        record.avg_queue_length = sqlite3_column_double(stmt, 5);
        record.avg_speed = sqlite3_column_double(stmt, 6);
        record.avg_flow_rate = sqlite3_column_double(stmt, 7);
        record.min_vehicle_count = sqlite3_column_double(stmt, 8);
        record.max_vehicle_count = sqlite3_column_double(stmt, 9);
        record.stddev_vehicle_count = sqlite3_column_double(stmt, 10);
        record.sample_count = sqlite3_column_int(stmt, 11);
        record.last_updated = sqlite3_column_int64(stmt, 12);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<DatabaseManager::TrafficPatternRecord> DatabaseManager::getTrafficPatternsForRoad(int road_id) {
    std::vector<TrafficPatternRecord> results;
    std::string sql = "SELECT id, road_id, day_of_week, time_slot, avg_vehicle_count, "
                     "avg_queue_length, avg_speed, avg_flow_rate, min_vehicle_count, "
                     "max_vehicle_count, stddev_vehicle_count, sample_count, last_updated "
                     "FROM traffic_patterns WHERE road_id = ? ORDER BY day_of_week, time_slot";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, road_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrafficPatternRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.road_id = sqlite3_column_int(stmt, 1);
        record.day_of_week = sqlite3_column_int(stmt, 2);
        record.time_slot = sqlite3_column_int(stmt, 3);
        record.avg_vehicle_count = sqlite3_column_double(stmt, 4);
        record.avg_queue_length = sqlite3_column_double(stmt, 5);
        record.avg_speed = sqlite3_column_double(stmt, 6);
        record.avg_flow_rate = sqlite3_column_double(stmt, 7);
        record.min_vehicle_count = sqlite3_column_double(stmt, 8);
        record.max_vehicle_count = sqlite3_column_double(stmt, 9);
        record.stddev_vehicle_count = sqlite3_column_double(stmt, 10);
        record.sample_count = sqlite3_column_int(stmt, 11);
        record.last_updated = sqlite3_column_int64(stmt, 12);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<DatabaseManager::TrafficPatternRecord> DatabaseManager::getAllTrafficPatterns() {
    std::vector<TrafficPatternRecord> results;
    std::string sql = "SELECT id, road_id, day_of_week, time_slot, avg_vehicle_count, "
                     "avg_queue_length, avg_speed, avg_flow_rate, min_vehicle_count, "
                     "max_vehicle_count, stddev_vehicle_count, sample_count, last_updated "
                     "FROM traffic_patterns ORDER BY road_id, day_of_week, time_slot";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrafficPatternRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.road_id = sqlite3_column_int(stmt, 1);
        record.day_of_week = sqlite3_column_int(stmt, 2);
        record.time_slot = sqlite3_column_int(stmt, 3);
        record.avg_vehicle_count = sqlite3_column_double(stmt, 4);
        record.avg_queue_length = sqlite3_column_double(stmt, 5);
        record.avg_speed = sqlite3_column_double(stmt, 6);
        record.avg_flow_rate = sqlite3_column_double(stmt, 7);
        record.min_vehicle_count = sqlite3_column_double(stmt, 8);
        record.max_vehicle_count = sqlite3_column_double(stmt, 9);
        record.stddev_vehicle_count = sqlite3_column_double(stmt, 10);
        record.sample_count = sqlite3_column_int(stmt, 11);
        record.last_updated = sqlite3_column_int64(stmt, 12);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

// Traffic profile operations
int DatabaseManager::createProfile(const std::string& name, const std::string& description) {
    std::string sql = "INSERT INTO traffic_profiles (name, description, is_active, created_at) "
                     "VALUES (?, ?, 0, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, std::time(nullptr));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return -1;
    }

    int profile_id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    LOG_INFO(LogComponent::Database, "Created traffic profile '{}' with ID: {}", name, profile_id);
    return profile_id;
}

DatabaseManager::ProfileRecord DatabaseManager::getProfile(int profile_id) {
    ProfileRecord record{};

    std::string sql = "SELECT id, name, description, is_active, created_at FROM traffic_profiles WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return record;
    }

    sqlite3_bind_int(stmt, 1, profile_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.description = desc ? desc : "";
        record.is_active = sqlite3_column_int(stmt, 3) != 0;
        record.created_at = sqlite3_column_int64(stmt, 4);
    }

    sqlite3_finalize(stmt);
    return record;
}

DatabaseManager::ProfileRecord DatabaseManager::getProfileByName(const std::string& name) {
    ProfileRecord record{};

    std::string sql = "SELECT id, name, description, is_active, created_at FROM traffic_profiles WHERE name = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return record;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.description = desc ? desc : "";
        record.is_active = sqlite3_column_int(stmt, 3) != 0;
        record.created_at = sqlite3_column_int64(stmt, 4);
    }

    sqlite3_finalize(stmt);
    return record;
}

std::vector<DatabaseManager::ProfileRecord> DatabaseManager::getAllProfiles() {
    std::vector<ProfileRecord> results;

    std::string sql = "SELECT id, name, description, is_active, created_at FROM traffic_profiles ORDER BY name";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ProfileRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.description = desc ? desc : "";
        record.is_active = sqlite3_column_int(stmt, 3) != 0;
        record.created_at = sqlite3_column_int64(stmt, 4);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool DatabaseManager::updateProfile(int profile_id, const std::string& name, const std::string& description) {
    std::string sql = "UPDATE traffic_profiles SET name = ?, description = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, profile_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool DatabaseManager::deleteProfile(int profile_id) {
    // Delete related records first (cascade should handle but be explicit)
    clearProfileSpawnRates(profile_id);
    clearProfileTrafficLights(profile_id);

    std::string sql = "DELETE FROM traffic_profiles WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int(stmt, 1, profile_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        LOG_INFO(LogComponent::Database, "Deleted traffic profile ID: {}", profile_id);
        return true;
    }
    return false;
}

bool DatabaseManager::setActiveProfile(int profile_id) {
    // First, deactivate all profiles
    if (!executeSQL("UPDATE traffic_profiles SET is_active = 0")) {
        return false;
    }

    // Then activate the specified profile
    std::string sql = "UPDATE traffic_profiles SET is_active = 1 WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int(stmt, 1, profile_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        LOG_INFO(LogComponent::Database, "Set active traffic profile ID: {}", profile_id);
        return true;
    }
    return false;
}

DatabaseManager::ProfileRecord DatabaseManager::getActiveProfile() {
    ProfileRecord record{};

    std::string sql = "SELECT id, name, description, is_active, created_at FROM traffic_profiles WHERE is_active = 1 LIMIT 1";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return record;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.description = desc ? desc : "";
        record.is_active = true;
        record.created_at = sqlite3_column_int64(stmt, 4);
    }

    sqlite3_finalize(stmt);
    return record;
}

// Profile spawn rate operations
bool DatabaseManager::saveProfileSpawnRates(int profile_id, const std::vector<ProfileSpawnRateRecord>& rates) {
    // Clear existing rates first
    clearProfileSpawnRates(profile_id);

    if (rates.empty()) return true;

    executeSQL("BEGIN TRANSACTION");

    std::string sql = "INSERT INTO road_flow_rates (profile_id, road_id, lane, vehicles_per_minute, created_at) "
                     "VALUES (?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        executeSQL("ROLLBACK");
        return false;
    }

    int64_t now = std::time(nullptr);

    for (const auto& rate : rates) {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, profile_id);
        sqlite3_bind_int(stmt, 2, rate.road_id);
        sqlite3_bind_int(stmt, 3, rate.lane);
        sqlite3_bind_double(stmt, 4, rate.vehicles_per_minute);
        sqlite3_bind_int64(stmt, 5, now);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            last_error_ = sqlite3_errmsg(db_);
            sqlite3_finalize(stmt);
            executeSQL("ROLLBACK");
            return false;
        }
    }

    sqlite3_finalize(stmt);
    executeSQL("COMMIT");

    LOG_DEBUG(LogComponent::Database, "Saved {} spawn rates for profile {}", rates.size(), profile_id);
    return true;
}

std::vector<DatabaseManager::ProfileSpawnRateRecord> DatabaseManager::getProfileSpawnRates(int profile_id) {
    std::vector<ProfileSpawnRateRecord> results;

    std::string sql = "SELECT id, profile_id, road_id, lane, vehicles_per_minute, created_at "
                     "FROM road_flow_rates WHERE profile_id = ? ORDER BY road_id, lane";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, profile_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ProfileSpawnRateRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.profile_id = sqlite3_column_int(stmt, 1);
        record.road_id = sqlite3_column_int(stmt, 2);
        record.lane = sqlite3_column_int(stmt, 3);
        record.vehicles_per_minute = sqlite3_column_double(stmt, 4);
        record.created_at = sqlite3_column_int64(stmt, 5);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool DatabaseManager::clearProfileSpawnRates(int profile_id) {
    std::string sql = "DELETE FROM road_flow_rates WHERE profile_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int(stmt, 1, profile_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

// Profile traffic light operations
bool DatabaseManager::saveProfileTrafficLights(int profile_id, const std::vector<ProfileTrafficLightRecord>& lights) {
    // Clear existing lights first
    clearProfileTrafficLights(profile_id);

    if (lights.empty()) return true;

    executeSQL("BEGIN TRANSACTION");

    std::string sql = "INSERT INTO profile_traffic_lights (profile_id, road_id, lane, green_time, yellow_time, red_time, created_at) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        executeSQL("ROLLBACK");
        return false;
    }

    int64_t now = std::time(nullptr);

    for (const auto& light : lights) {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, profile_id);
        sqlite3_bind_int(stmt, 2, light.road_id);
        sqlite3_bind_int(stmt, 3, light.lane);
        sqlite3_bind_double(stmt, 4, light.green_time);
        sqlite3_bind_double(stmt, 5, light.yellow_time);
        sqlite3_bind_double(stmt, 6, light.red_time);
        sqlite3_bind_int64(stmt, 7, now);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            last_error_ = sqlite3_errmsg(db_);
            sqlite3_finalize(stmt);
            executeSQL("ROLLBACK");
            return false;
        }
    }

    sqlite3_finalize(stmt);
    executeSQL("COMMIT");

    LOG_DEBUG(LogComponent::Database, "Saved {} traffic lights for profile {}", lights.size(), profile_id);
    return true;
}

std::vector<DatabaseManager::ProfileTrafficLightRecord> DatabaseManager::getProfileTrafficLights(int profile_id) {
    std::vector<ProfileTrafficLightRecord> results;

    std::string sql = "SELECT id, profile_id, road_id, lane, green_time, yellow_time, red_time, created_at "
                     "FROM profile_traffic_lights WHERE profile_id = ? ORDER BY road_id, lane";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return results;
    }

    sqlite3_bind_int(stmt, 1, profile_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ProfileTrafficLightRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.profile_id = sqlite3_column_int(stmt, 1);
        record.road_id = sqlite3_column_int(stmt, 2);
        record.lane = sqlite3_column_int(stmt, 3);
        record.green_time = sqlite3_column_double(stmt, 4);
        record.yellow_time = sqlite3_column_double(stmt, 5);
        record.red_time = sqlite3_column_double(stmt, 6);
        record.created_at = sqlite3_column_int64(stmt, 7);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool DatabaseManager::clearProfileTrafficLights(int profile_id) {
    std::string sql = "DELETE FROM profile_traffic_lights WHERE profile_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int(stmt, 1, profile_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

} // namespace data
} // namespace ratms
