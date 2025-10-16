#include "database_manager.h"
#include "../../utils/logger.h"
#include <fstream>
#include <sstream>
#include <ctime>

namespace ratms {
namespace data {

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
        log_error("Failed to open database: %s", last_error_.c_str());
        return false;
    }

    log_info("Database opened successfully: %s", db_path_.c_str());
    return true;
}

bool DatabaseManager::runMigrations(const std::string& migrations_dir) {
    std::string migration_file = migrations_dir + "/001_initial_schema.sql";
    log_info("Running database migration: %s", migration_file.c_str());

    if (!executeSQLFile(migration_file)) {
        log_error("Migration failed: %s", last_error_.c_str());
        return false;
    }

    log_info("Database migrations completed successfully");
    return true;
}

bool DatabaseManager::executeSQL(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        last_error_ = err_msg ? err_msg : "Unknown error";
        log_error("SQL execution failed: %s", last_error_.c_str());
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
        log_info("Database closed");
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
        log_error("Failed to prepare statement: %s", last_error_.c_str());
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
        log_error("Failed to insert simulation: %s", last_error_.c_str());
        return -1;
    }

    int sim_id = sqlite3_last_insert_rowid(db_);
    log_info("Created simulation with ID: %d", sim_id);
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

    log_info("Updated simulation %d status to: %s", sim_id, status.c_str());
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
    log_info("Created network with ID: %d", network_id);
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

} // namespace data
} // namespace ratms
