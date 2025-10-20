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

    struct OptimizationRunRecord {
        int id;
        int network_id;
        std::string status;
        int population_size;
        int generations;
        double mutation_rate;
        double crossover_rate;
        double elitism_rate;
        double min_green_time;
        double max_green_time;
        double min_red_time;
        double max_red_time;
        int simulation_steps;
        double dt;
        double baseline_fitness;
        double best_fitness;
        double improvement_percent;
        long started_at;
        long completed_at;
        long duration_seconds;
        std::string created_by;
        std::string notes;
    };

    struct OptimizationGenerationRecord {
        int id;
        int optimization_run_id;
        int generation_number;
        double best_fitness;
        double average_fitness;
        double worst_fitness;
        long timestamp;
    };

    struct OptimizationSolutionRecord {
        int id;
        int optimization_run_id;
        bool is_best_solution;
        double fitness;
        std::string chromosome_json;
        int traffic_light_count;
        long created_at;
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

    // Optimization operations
    int createOptimizationRun(const OptimizationRunRecord& record);
    bool updateOptimizationRunStatus(int run_id, const std::string& status);
    bool completeOptimizationRun(int run_id, long completed_at, long duration_seconds,
                                double baseline_fitness, double best_fitness, double improvement_percent);
    OptimizationRunRecord getOptimizationRun(int run_id);
    std::vector<OptimizationRunRecord> getAllOptimizationRuns();
    std::vector<OptimizationRunRecord> getOptimizationRunsByStatus(const std::string& status);

    // Optimization generation operations
    bool insertOptimizationGeneration(const OptimizationGenerationRecord& record);
    bool insertOptimizationGenerationsBatch(const std::vector<OptimizationGenerationRecord>& records);
    std::vector<OptimizationGenerationRecord> getOptimizationGenerations(int run_id);

    // Optimization solution operations
    int insertOptimizationSolution(const OptimizationSolutionRecord& record);
    OptimizationSolutionRecord getBestOptimizationSolution(int run_id);
    std::vector<OptimizationSolutionRecord> getOptimizationSolutions(int run_id);

    // Analytics operations
    struct MetricStatistics {
        std::string metric_type;
        double min_value;
        double max_value;
        double mean_value;
        double median_value;
        double stddev_value;
        double p25_value;  // 25th percentile
        double p75_value;  // 75th percentile
        double p95_value;  // 95th percentile
        int sample_count;
    };

    struct ComparativeMetrics {
        int simulation_id;
        std::string simulation_name;
        std::vector<MetricRecord> metrics;
    };

    // Get statistics for a specific metric type across a simulation
    MetricStatistics getMetricStatistics(int simulation_id, const std::string& metric_type);

    // Get all metric statistics for a simulation (grouped by metric_type)
    std::map<std::string, MetricStatistics> getAllMetricStatistics(int simulation_id);

    // Get metrics for multiple simulations for comparison
    std::vector<ComparativeMetrics> getComparativeMetrics(const std::vector<int>& simulation_ids,
                                                          const std::string& metric_type);

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
