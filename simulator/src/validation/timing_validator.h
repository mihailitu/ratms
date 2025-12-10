#ifndef TIMING_VALIDATOR_H
#define TIMING_VALIDATOR_H

#include "../optimization/genetic_algorithm.h"
#include "../optimization/metrics.h"
#include "../core/road.h"
#include <vector>
#include <string>
#include <cstdint>

namespace ratms {
namespace validation {

/**
 * @brief Configuration for timing validation
 */
struct ValidationConfig {
    int simulationSteps = 500;           // Steps to run validation simulation
    double dt = 0.1;                     // Time step (seconds)
    double improvementThreshold = 5.0;   // Minimum improvement % to pass validation
    double regressionThreshold = 10.0;   // Max regression % before rejection
};

/**
 * @brief Result of a timing validation run
 */
struct ValidationResult {
    bool passed;                         // Whether validation passed
    double baselineFitness;              // Fitness with current timings
    double optimizedFitness;             // Fitness with new timings
    double improvementPercent;           // (baseline - optimized) / baseline * 100
    std::string reason;                  // Why it passed/failed
    int64_t timestamp;                   // When validation was performed
};

/**
 * @brief TimingValidator - Validates optimized traffic light timings before application
 *
 * This class runs a side simulation to compare baseline (current) timing performance
 * against proposed optimized timings. It helps prevent applying changes that would
 * actually degrade traffic flow.
 *
 * Validation criteria:
 * - PASS: Improvement >= improvementThreshold OR no regression
 * - FAIL: Regression > regressionThreshold
 * - WARN: Minor regression within tolerance (still passes)
 */
class TimingValidator {
public:
    explicit TimingValidator(ValidationConfig config = {});

    /**
     * @brief Validate a chromosome against current network state
     * @param network - Road network to test (will be copied, not modified)
     * @param chromosome - Proposed traffic light timings
     * @return ValidationResult with pass/fail and metrics
     */
    ValidationResult validate(
        const std::vector<simulator::Road>& network,
        const simulator::Chromosome& chromosome);

    // Configuration
    void setConfig(const ValidationConfig& config);
    ValidationConfig getConfig() const { return config_; }

private:
    /**
     * @brief Run simulation and return fitness value
     * @param network - Road network (copied for simulation)
     * @return Fitness value (lower is better)
     */
    double runSimulation(std::vector<simulator::Road> network);

    /**
     * @brief Apply chromosome timings to network
     * @param network - Road network to modify
     * @param chromosome - Timings to apply
     */
    void applyChromosome(
        std::vector<simulator::Road>& network,
        const simulator::Chromosome& chromosome);

    /**
     * @brief Deep copy network for simulation
     */
    std::vector<simulator::Road> copyNetwork(
        const std::vector<simulator::Road>& network);

    ValidationConfig config_;
};

} // namespace validation
} // namespace ratms

#endif // TIMING_VALIDATOR_H
