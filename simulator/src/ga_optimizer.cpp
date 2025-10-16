#include "optimization/genetic_algorithm.h"
#include "optimization/metrics.h"
#include "core/simulator.h"
#include "core/road.h"
#include "core/vehicle.h"
#include "core/trafficlight.h"
#include "utils/config.h"

#include <iostream>
#include <vector>
#include <memory>
#include <chrono>

using namespace simulator;

/**
 * @brief createTestIntersection - Creates a 4-way intersection for testing
 * Returns 4 roads (North, South, East, West approaches) with traffic lights
 */
std::vector<Road> createTestIntersection() {
    std::vector<Road> roads;

    // Road 0: North approach (heading South into intersection)
    Road north(0, 300, 2, 15);  // 300m, 2 lanes, 15 m/s max speed
    north.setCardinalCoordinates({500, 0}, {500, 300});
    // Add some vehicles
    north.addVehicle(Vehicle(50.0, 5.0, 10.0), 0);
    north.addVehicle(Vehicle(100.0, 5.0, 12.0), 0);
    north.addVehicle(Vehicle(150.0, 5.0, 8.0), 1);
    // Connect to other roads
    north.addLaneConnection(0, 2, 0.7);  // Mostly go straight (East)
    north.addLaneConnection(0, 3, 0.3);  // Some go right (West)
    north.addLaneConnection(1, 2, 0.5);  // Lane 1 can go straight
    north.addLaneConnection(1, 1, 0.5);  // or left (South)

    // Road 1: South approach (heading North into intersection)
    Road south(1, 300, 2, 15);
    south.setCardinalCoordinates({500, 1000}, {500, 700});
    south.addVehicle(Vehicle(50.0, 5.0, 11.0), 0);
    south.addVehicle(Vehicle(120.0, 5.0, 9.0), 1);
    south.addLaneConnection(0, 3, 0.6);
    south.addLaneConnection(0, 2, 0.4);
    south.addLaneConnection(1, 3, 0.5);
    south.addLaneConnection(1, 0, 0.5);

    // Road 2: East approach (heading West into intersection)
    Road east(2, 300, 1, 15);
    east.setCardinalCoordinates({1000, 500}, {700, 500});
    east.addVehicle(Vehicle(80.0, 5.0, 10.0), 0);
    east.addVehicle(Vehicle(180.0, 5.0, 11.0), 0);
    east.addLaneConnection(0, 3, 0.7);
    east.addLaneConnection(0, 0, 0.3);

    // Road 3: West approach (heading East into intersection)
    Road west(3, 300, 1, 15);
    west.setCardinalCoordinates({0, 500}, {300, 500});
    west.addVehicle(Vehicle(60.0, 5.0, 12.0), 0);
    west.addVehicle(Vehicle(140.0, 5.0, 9.0), 0);
    west.addLaneConnection(0, 2, 0.6);
    west.addLaneConnection(0, 1, 0.4);

    roads.push_back(north);
    roads.push_back(south);
    roads.push_back(east);
    roads.push_back(west);

    return roads;
}

/**
 * @brief runBaselineSimulation - Run simulation with fixed traffic light timings
 */
SimulationMetrics runBaselineSimulation(std::vector<Road>& roadNetwork, int steps, double dt) {
    Simulator sim;
    sim.cityMap.clear();

    for (Road& road : roadNetwork) {
        sim.addRoadToMap(road);
    }

    MetricsCollector collector;
    std::vector<RoadTransition> pendingTransitions;

    for (int step = 0; step < steps; ++step) {
        pendingTransitions.clear();

        for (auto& roadPair : sim.cityMap) {
            roadPair.second.update(dt, sim.cityMap, pendingTransitions);
        }

        for (const auto& transition : pendingTransitions) {
            Vehicle transitioningVehicle = std::get<0>(transition);
            roadID destRoadID = std::get<1>(transition);
            unsigned destLane = std::get<2>(transition);

            auto destRoadIt = sim.cityMap.find(destRoadID);
            if (destRoadIt != sim.cityMap.end()) {
                transitioningVehicle.setPos(0.0);
                destRoadIt->second.addVehicle(transitioningVehicle, destLane);
            } else {
                collector.getMetricsMutable().vehiclesExited += 1.0;
            }
        }

        if (step % 10 == 0) {
            collector.collectMetrics(sim.cityMap, dt);
        }
    }

    SimulationMetrics metrics = collector.getMetrics();
    if (metrics.sampleCount > 0) {
        metrics.averageQueueLength /= metrics.sampleCount;
        metrics.averageSpeed /= metrics.sampleCount;
    }

    return metrics;
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║   RATMS - Genetic Algorithm Traffic Light Optimizer     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    // Configuration
    int simulationSteps = 1000;
    double dt = 0.1;

    GeneticAlgorithm::Parameters gaParams;
    gaParams.populationSize = 30;
    gaParams.generations = 50;
    gaParams.mutationRate = 0.15;
    gaParams.mutationStdDev = 5.0;
    gaParams.crossoverRate = 0.8;
    gaParams.tournamentSize = 3;
    gaParams.elitismRate = 0.1;
    gaParams.minGreenTime = 10.0;
    gaParams.maxGreenTime = 60.0;
    gaParams.minRedTime = 10.0;
    gaParams.maxRedTime = 60.0;
    gaParams.seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--pop" && i + 1 < argc) {
            gaParams.populationSize = std::stoi(argv[++i]);
        } else if (arg == "--gen" && i + 1 < argc) {
            gaParams.generations = std::stoi(argv[++i]);
        } else if (arg == "--steps" && i + 1 < argc) {
            simulationSteps = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --pop N      Population size (default: 30)\n";
            std::cout << "  --gen N      Number of generations (default: 50)\n";
            std::cout << "  --steps N    Simulation steps per evaluation (default: 1000)\n";
            std::cout << "  --help       Show this help message\n";
            return 0;
        }
    }

    std::cout << "Configuration:\n";
    std::cout << "  Population size: " << gaParams.populationSize << "\n";
    std::cout << "  Generations: " << gaParams.generations << "\n";
    std::cout << "  Simulation steps: " << simulationSteps << "\n";
    std::cout << "  Time step (dt): " << dt << " seconds\n";
    std::cout << "\n";

    // Create test network
    std::cout << "Creating test intersection network...\n";
    std::vector<Road> testNetwork = createTestIntersection();

    size_t totalTrafficLights = 0;
    for (const auto& road : testNetwork) {
        totalTrafficLights += road.getLanesNo();
    }
    std::cout << "  Roads: " << testNetwork.size() << "\n";
    std::cout << "  Traffic lights: " << totalTrafficLights << "\n";
    std::cout << "\n";

    // Run baseline simulation with default timings
    std::cout << "Running baseline simulation (fixed timings)...\n";
    std::vector<Road> baselineNetwork = createTestIntersection();
    SimulationMetrics baselineMetrics = runBaselineSimulation(baselineNetwork, simulationSteps, dt);
    double baselineFitness = baselineMetrics.getFitness();

    std::cout << "Baseline Results:\n";
    std::cout << "  Average Queue Length: " << baselineMetrics.averageQueueLength << " vehicles\n";
    std::cout << "  Average Speed: " << baselineMetrics.averageSpeed << " m/s\n";
    std::cout << "  Vehicles Exited: " << baselineMetrics.vehiclesExited << "\n";
    std::cout << "  Fitness: " << baselineFitness << "\n";
    std::cout << "\n";

    // Create fitness evaluator
    FitnessEvaluator evaluator(simulationSteps, dt);

    // Create fitness function lambda
    auto fitnessFunc = [&evaluator, &testNetwork](const Chromosome& chromosome) -> double {
        // Create a fresh copy of the network for each evaluation
        std::vector<Road> networkCopy = testNetwork;
        return evaluator.evaluate(chromosome, networkCopy);
    };

    // Create and run GA
    GeneticAlgorithm ga(gaParams, fitnessFunc);
    ga.initializePopulation(totalTrafficLights);

    std::cout << "Starting genetic algorithm optimization...\n";
    auto startTime = std::chrono::high_resolution_clock::now();

    Chromosome bestSolution = ga.evolve();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                   Optimization Complete                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Optimization time: " << duration.count() << " seconds\n";
    std::cout << "\n";

    // Compare baseline vs optimized
    double improvement = ((baselineFitness - bestSolution.fitness) / baselineFitness) * 100.0;

    std::cout << "Results Comparison:\n";
    std::cout << "  Baseline fitness:  " << baselineFitness << "\n";
    std::cout << "  Optimized fitness: " << bestSolution.fitness << "\n";
    std::cout << "  Improvement:       " << improvement << "%\n";
    std::cout << "\n";

    std::cout << "Optimized Traffic Light Timings:\n";
    for (size_t i = 0; i < bestSolution.genes.size(); ++i) {
        std::cout << "  Light " << i << ": Green=" << bestSolution.genes[i].greenTime
                  << "s, Red=" << bestSolution.genes[i].redTime << "s\n";
    }
    std::cout << "\n";

    // Export results
    std::cout << "Exporting results...\n";
    exportEvolutionHistoryCSV(ga.getFitnessHistory(), "evolution_history.csv");
    exportChromosomeCSV(bestSolution, "best_solution.csv");
    std::cout << "  evolution_history.csv - Fitness per generation\n";
    std::cout << "  best_solution.csv - Best traffic light configuration\n";
    std::cout << "\n";

    std::cout << "✓ Optimization complete!\n";
    std::cout << "\n";

    return 0;
}
