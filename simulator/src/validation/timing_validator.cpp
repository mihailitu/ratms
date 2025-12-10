#include "timing_validator.h"
#include "../core/simulator.h"
#include <chrono>
#include <cmath>

namespace ratms {
namespace validation {

TimingValidator::TimingValidator(ValidationConfig config)
    : config_(config) {}

ValidationResult TimingValidator::validate(
    const std::vector<simulator::Road>& network,
    const simulator::Chromosome& chromosome) {

    ValidationResult result;
    result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Run baseline simulation (current timings)
    auto baselineNetwork = copyNetwork(network);
    result.baselineFitness = runSimulation(std::move(baselineNetwork));

    // Run optimized simulation (apply chromosome)
    auto optimizedNetwork = copyNetwork(network);
    applyChromosome(optimizedNetwork, chromosome);
    result.optimizedFitness = runSimulation(std::move(optimizedNetwork));

    // Calculate improvement (lower fitness is better)
    // Positive improvement means optimized is better
    if (result.baselineFitness > 0) {
        result.improvementPercent =
            (result.baselineFitness - result.optimizedFitness) / result.baselineFitness * 100.0;
    } else {
        // Handle edge case where baseline is 0 or negative
        result.improvementPercent = result.optimizedFitness < result.baselineFitness ? 100.0 : -100.0;
    }

    // Determine if validation passed
    if (result.improvementPercent >= config_.improvementThreshold) {
        result.passed = true;
        result.reason = "Significant improvement: " +
            std::to_string(result.improvementPercent) + "% (threshold: " +
            std::to_string(config_.improvementThreshold) + "%)";
    } else if (result.improvementPercent >= 0) {
        result.passed = true;
        result.reason = "Minor improvement: " +
            std::to_string(result.improvementPercent) + "% (no regression)";
    } else if (std::abs(result.improvementPercent) <= config_.regressionThreshold) {
        result.passed = true;
        result.reason = "Minor regression within tolerance: " +
            std::to_string(result.improvementPercent) + "% (threshold: -" +
            std::to_string(config_.regressionThreshold) + "%)";
    } else {
        result.passed = false;
        result.reason = "Significant regression: " +
            std::to_string(result.improvementPercent) + "% (threshold: -" +
            std::to_string(config_.regressionThreshold) + "%)";
    }

    return result;
}

void TimingValidator::setConfig(const ValidationConfig& config) {
    config_ = config;
}

double TimingValidator::runSimulation(std::vector<simulator::Road> network) {
    // Create simulator and populate with network
    simulator::Simulator sim;
    for (auto& road : network) {
        sim.addRoadToMap(road);
    }

    // Run simulation and collect metrics
    simulator::MetricsCollector collector;
    std::vector<simulator::RoadTransition> pendingTransitions;

    for (int step = 0; step < config_.simulationSteps; ++step) {
        pendingTransitions.clear();

        // Update all roads
        for (auto& [id, road] : sim.cityMap) {
            road.update(config_.dt, sim.cityMap, pendingTransitions);
        }

        // Execute road transitions
        for (const auto& transition : pendingTransitions) {
            simulator::Vehicle transitioningVehicle = std::get<0>(transition);
            simulator::roadID destRoadID = std::get<1>(transition);
            unsigned destLane = std::get<2>(transition);

            auto destRoadIt = sim.cityMap.find(destRoadID);
            if (destRoadIt != sim.cityMap.end()) {
                transitioningVehicle.setPos(0.0);
                destRoadIt->second.addVehicle(transitioningVehicle, destLane);
            } else {
                // Vehicle exited the network
                collector.getMetricsMutable().vehiclesExited += 1.0;
            }
        }

        // Collect metrics every 10 steps
        if (step % 10 == 0) {
            collector.collectMetrics(sim.cityMap, config_.dt);
        }
    }

    // Get final metrics and compute fitness
    simulator::SimulationMetrics metrics = collector.getMetrics();

    // Average the accumulated values
    if (metrics.sampleCount > 0) {
        metrics.averageQueueLength /= metrics.sampleCount;
        metrics.averageSpeed /= metrics.sampleCount;
    }

    return metrics.getFitness();
}

void TimingValidator::applyChromosome(
    std::vector<simulator::Road>& network,
    const simulator::Chromosome& chromosome) {

    size_t geneIdx = 0;
    for (auto& road : network) {
        auto& trafficLights = road.getTrafficLightsMutable();
        for (size_t lane = 0; lane < trafficLights.size() && geneIdx < chromosome.genes.size(); ++lane) {
            const auto& gene = chromosome.genes[geneIdx];
            trafficLights[lane].setTimings(
                gene.greenTime,
                3.0,  // Fixed yellow time for safety
                gene.redTime
            );
            geneIdx++;
        }
    }
}

std::vector<simulator::Road> TimingValidator::copyNetwork(
    const std::vector<simulator::Road>& network) {
    // Simple copy - Road should be copyable
    return network;
}

} // namespace validation
} // namespace ratms
