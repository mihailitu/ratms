#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace ratms {
namespace data {

/**
 * @brief Database manager for RATMS using SQLite3
 *
 * Handles all database operations including:
 * - Simulation metadata storage
 * - Time-series metrics
 * - Network configurations
 * - Optimization results
 */
class DatabaseManager {
public:
    struct SimulationRecord {
        int id;
        std::string name;
        std::string description;
        int network_id;
        std::string status;
        long start_time;
        long end_time;
        double duration_seconds;
        std::string config_json;
    };

    struct MetricRecord {
        int id;
        int simulation_id;
        double timestamp;
        std::string metric_type;
        int road_id;
        double value;
        std::string unit;
        std::string metadata_json;
    };

    struct NetworkRecord {
        int id;
        std::string name;
        std::string description;
        int road_count;
        int intersection_count;
        std::string config_json;
    };

    DatabaseManager(const std::string& db_path = "ratms.db");
    ~DatabaseManager();

    // Initialize database and run migrations
    bool initialize();
    bool runMigrations(const std::string& migrations_dir);

    // Simulation operations
    int createSimulation(const std::string& name, const std::string& description,
                        int network_id, const std::string& config_json);
    bool updateSimulationStatus(int sim_id, const std::string& status);
    bool completeSimulation(int sim_id, long end_time, double duration);
    SimulationRecord getSimulation(int sim_id);
    std::vector<SimulationRecord> getAllSimulations();
    std::vector<SimulationRecord> getSimulationsByStatus(const std::string& status);

    // Metrics operations
    bool insertMetric(int simulation_id, double timestamp, const std::string& metric_type,
                     int road_id, double value, const std::string& unit = "",
                     const std::string& metadata_json = "");
    bool insertMetricsBatch(const std::vector<MetricRecord>& metrics);
    std::vector<MetricRecord> getMetrics(int simulation_id);
    std::vector<MetricRecord> getMetricsByType(int simulation_id, const std::string& metric_type);
    std::vector<MetricRecord> getMetricsTimeRange(int simulation_id, double start_time, double end_time);

    // Network operations
    int createNetwork(const std::string& name, const std::string& description,
                     int road_count, int intersection_count, const std::string& config_json);
    NetworkRecord getNetwork(int network_id);
    std::vector<NetworkRecord> getAllNetworks();
    bool deleteNetwork(int network_id);

    // Utility
    bool isConnected() const { return db_ != nullptr; }
    std::string getLastError() const { return last_error_; }
    void close();

private:
    bool executeSQL(const std::string& sql);
    bool executeSQLFile(const std::string& file_path);

    sqlite3* db_;
    std::string db_path_;
    std::string last_error_;
};

} // namespace data
} // namespace ratms

#endif // DATABASE_MANAGER_H
