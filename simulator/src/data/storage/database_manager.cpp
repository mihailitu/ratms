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

} // namespace data
} // namespace ratms
