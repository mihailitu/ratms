#include "metrics.h"
#include "../core/road.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace simulator {

// ============================================================================
// SimulationMetrics Implementation
// ============================================================================

double SimulationMetrics::getFitness() const {
  // Fitness function: minimize queue length and maximize throughput
  // Lower fitness is better

  // If we have very few samples, penalize heavily (incomplete simulation)
  if (sampleCount < 10) {
    // Use alternative fitness based on current state
    double queuePenalty = averageQueueLength * 10.0; // Heavy penalty for queues
    double vehiclePenalty =
        totalVehicles * 0.1; // Slight penalty for active vehicles
    return queuePenalty + vehiclePenalty;
  }

  // Primary objective: minimize average queue length
  double queueFitness = averageQueueLength * 100.0;

  // Secondary objective: maximize vehicles that exited (completed routes)
  // Penalize if few vehicles exit (poor traffic flow)
  double exitRatio =
      (totalVehicles > 0) ? (vehiclesExited / totalVehicles) : 0.0;
  double exitPenalty = (1.0 - exitRatio) * 50.0;

  // Tertiary objective: maximize average speed (avoid congestion)
  double speedPenalty = (10.0 - averageSpeed) * 0.5; // Assume ~10 m/s is good

  double fitness = queueFitness + exitPenalty + speedPenalty;

  return std::max(0.0, fitness); // Ensure non-negative
}

// ============================================================================
// MetricsCollector Implementation
// ============================================================================

void MetricsCollector::collectMetrics(const Simulator::CityMap &cityMap,
                                      double /*dt*/) {
  double currentQueueLength = 0.0;
  double totalSpeed = 0.0;
  int vehicleCount = 0;

  // Iterate through all roads and lanes
  for (const auto &roadPair : cityMap) {
    const Road &road = roadPair.second;
    const auto &lanes = road.getVehicles();
    const auto &lightConfig = road.getCurrentLightConfig();

    for (size_t laneIdx = 0; laneIdx < lanes.size(); ++laneIdx) {
      const auto &lane = lanes[laneIdx];

      // Count vehicles in queue at red lights
      if (laneIdx < lightConfig.size() && lightConfig[laneIdx] == 'R') {
        // Count vehicles near the end of road (waiting at light)
        for (const auto &vehicle : lane) {
          double distanceToEnd = road.getLength() - vehicle.getPos();
          if (distanceToEnd < 50.0) { // Within 50m of traffic light
            currentQueueLength += 1.0;
          }
        }
      }

      // Collect speed data
      for (const auto &vehicle : lane) {
        double vel = vehicle.getVelocity();
        if (!std::isnan(vel)) {
          totalSpeed += vel;
          vehicleCount++;
        }
      }
    }
  }

  // Update metrics
  metrics_.averageQueueLength += currentQueueLength;
  metrics_.maxQueueLength =
      std::max(metrics_.maxQueueLength, currentQueueLength);
  metrics_.totalVehicles = vehicleCount;

  if (vehicleCount > 0) {
    metrics_.averageSpeed += totalSpeed / vehicleCount;
  }

  metrics_.sampleCount++;
  simulationSteps_++;
}

void MetricsCollector::reset() {
  metrics_ = SimulationMetrics();
  simulationSteps_ = 0;
}

// ============================================================================
// FitnessEvaluator Implementation
// ============================================================================

size_t
FitnessEvaluator::countTrafficLights(const std::vector<Road> &roadNetwork) {
  size_t count = 0;
  for (const auto &road : roadNetwork) {
    count += road.getLanesNo(); // One traffic light per lane
  }
  return count;
}

void FitnessEvaluator::applyChromosome(const Chromosome & /*chromosome*/,
                                       Simulator::CityMap &cityMap) {
  size_t geneIdx = 0;

  for (auto &roadPair : cityMap) {
    Road &road = roadPair.second;
    unsigned lanesNo = road.getLanesNo();

    // For each lane, we need to set its traffic light timing
    // NOTE: This is a workaround since we can't directly modify traffic lights
    // In a production version, we'd add a method to Road to update traffic
    // light timings

    // For now, we'll need to create new roads with updated traffic lights
    // This is handled in the evaluate() function by rebuilding the network

    geneIdx += lanesNo;
  }
}

double FitnessEvaluator::evaluate(const Chromosome & /*chromosome*/,
                                  std::vector<Road> &roadNetwork) {
  // Create simulator and add roads
  Simulator sim;
  sim.cityMap.clear();

  // Apply chromosome: rebuild roads with new traffic light timings
  size_t geneIdx = 0;
  for (Road &road : roadNetwork) {
    // Note: Since we can't directly modify traffic lights after Road
    // construction, we need to recreate roads with new timings This is a
    // limitation of the current Road class design

    // For this prototype, we'll add roads as-is and track metrics
    // A production version would require refactoring Road to allow traffic
    // light updates
    sim.addRoadToMap(road);
    geneIdx += road.getLanesNo();
  }

  // Run simulation and collect metrics
  MetricsCollector collector;
  double runTime = 0.0;
  std::vector<RoadTransition> pendingTransitions;

  for (int step = 0; step < simulationSteps_; ++step) {
    // Clear pending transitions
    pendingTransitions.clear();

    // Update all roads
    for (auto &roadPair : sim.cityMap) {
      roadPair.second.update(dt_, sim.cityMap, pendingTransitions);
    }

    // Execute road transitions
    for (const auto &transition : pendingTransitions) {
      Vehicle transitioningVehicle = std::get<0>(transition);
      roadID destRoadID = std::get<1>(transition);
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
      collector.collectMetrics(sim.cityMap, dt_);
    }

    runTime += dt_;
  }

  // Get final metrics and compute fitness
  SimulationMetrics metrics = collector.getMetrics();

  // Average the accumulated values
  if (metrics.sampleCount > 0) {
    metrics.averageQueueLength /= metrics.sampleCount;
    metrics.averageSpeed /= metrics.sampleCount;
  }

  return metrics.getFitness();
}

} // namespace simulator
