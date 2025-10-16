#ifndef METRICS_H
#define METRICS_H

#include "../core/simulator.h"
#include "genetic_algorithm.h"
#include <vector>

namespace simulator {

/**
 * @brief SimulationMetrics - Stores performance metrics from a simulation run
 */
struct SimulationMetrics {
    double averageQueueLength = 0.0;      // Average vehicles waiting at red lights
    double maxQueueLength = 0.0;          // Maximum queue observed
    double totalVehicles = 0.0;           // Total vehicles in simulation
    double vehiclesExited = 0.0;          // Vehicles that completed their route
    double averageSpeed = 0.0;            // Average speed across all vehicles
    int sampleCount = 0;                  // Number of samples taken

    // Compute a fitness score (lower is better)
    double getFitness() const;
};

/**
 * @brief MetricsCollector - Collects metrics during simulation
 */
class MetricsCollector {
private:
    SimulationMetrics metrics_;
    int simulationSteps_;

public:
    MetricsCollector() : simulationSteps_(0) {}

    // Collect metrics from current simulation state
    void collectMetrics(const Simulator::CityMap& cityMap, double dt);

    // Get final aggregated metrics
    SimulationMetrics getMetrics() const { return metrics_; }

    // Get mutable reference to metrics
    SimulationMetrics& getMetricsMutable() { return metrics_; }

    // Reset for new simulation
    void reset();
};

/**
 * @brief FitnessEvaluator - Evaluates fitness by running simulations
 */
class FitnessEvaluator {
private:
    int simulationSteps_;
    double dt_;

public:
    FitnessEvaluator(int simulationSteps = 1000, double dt = 0.1)
        : simulationSteps_(simulationSteps), dt_(dt) {}

    /**
     * @brief evaluate - Run simulation with given chromosome and return fitness
     * @param chromosome - Traffic light configuration to test
     * @param roadNetwork - Test road network
     * @return fitness value (lower is better)
     */
    double evaluate(const Chromosome& chromosome, std::vector<Road>& roadNetwork);

private:
    // Apply chromosome parameters to road network
    void applyChromosome(const Chromosome& chromosome, Simulator::CityMap& cityMap);

    // Count total traffic lights in network
    static size_t countTrafficLights(const std::vector<Road>& roadNetwork);
};

} // namespace simulator

#endif // METRICS_H
