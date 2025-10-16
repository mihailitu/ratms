#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

#include <vector>
#include <functional>
#include <random>
#include <string>

namespace simulator {

/**
 * @brief TrafficLightTiming - Represents timing parameters for a single traffic light
 */
struct TrafficLightTiming {
    double greenTime;  // seconds
    double redTime;    // seconds
    // yellowTime is fixed at 3.0 seconds for safety

    TrafficLightTiming() : greenTime(30.0), redTime(30.0) {}
    TrafficLightTiming(double g, double r) : greenTime(g), redTime(r) {}
};

/**
 * @brief Chromosome - Represents a complete traffic light configuration for the network
 * Contains timing parameters for all traffic lights in all roads
 */
class Chromosome {
public:
    std::vector<TrafficLightTiming> genes;  // One per traffic light in network
    double fitness;

    Chromosome() : fitness(0.0) {}
    explicit Chromosome(size_t numLights) : genes(numLights), fitness(0.0) {}

    // Initialize with random values within bounds
    void randomize(double minGreen, double maxGreen, double minRed, double maxRed, std::mt19937& rng);

    // Mutation operators
    void mutate(double mutationRate, double mutationStdDev,
                double minGreen, double maxGreen, double minRed, double maxRed,
                std::mt19937& rng);

    // Clamp values to valid ranges
    void clamp(double minGreen, double maxGreen, double minRed, double maxRed);

    size_t size() const { return genes.size(); }
};

/**
 * @brief GeneticAlgorithm - GA engine for optimizing traffic light timings
 */
class GeneticAlgorithm {
public:
    struct Parameters {
        size_t populationSize = 50;
        size_t generations = 100;
        double mutationRate = 0.1;
        double mutationStdDev = 5.0;   // seconds
        double crossoverRate = 0.8;
        size_t tournamentSize = 3;
        double elitismRate = 0.1;      // Keep top 10% unchanged

        // Traffic light parameter bounds
        double minGreenTime = 10.0;    // seconds
        double maxGreenTime = 90.0;    // seconds
        double minRedTime = 10.0;      // seconds
        double maxRedTime = 90.0;      // seconds

        unsigned seed = 42;
    };

    // Fitness function type: takes a chromosome and returns fitness value
    // Lower fitness is better (minimization problem)
    using FitnessFunction = std::function<double(const Chromosome&)>;

private:
    Parameters params_;
    std::vector<Chromosome> population_;
    std::mt19937 rng_;
    FitnessFunction fitnessFunc_;

    Chromosome bestChromosome_;
    std::vector<double> fitnessHistory_;  // Best fitness per generation

public:
    GeneticAlgorithm(const Parameters& params, FitnessFunction fitnessFunc);

    // Initialize population with random chromosomes
    void initializePopulation(size_t chromosomeSize);

    // Run the genetic algorithm
    Chromosome evolve();

    // Get evolution history
    const std::vector<double>& getFitnessHistory() const { return fitnessHistory_; }
    const Chromosome& getBestChromosome() const { return bestChromosome_; }

private:
    // Selection operators
    Chromosome& tournamentSelection();

    // Crossover operators
    Chromosome uniformCrossover(const Chromosome& parent1, const Chromosome& parent2);

    // Evaluate fitness for entire population
    void evaluatePopulation();

    // Sort population by fitness (ascending - lower is better)
    void sortPopulation();

    // Create next generation
    void reproduce();
};

/**
 * @brief Utility function to export evolution history to CSV
 */
void exportEvolutionHistoryCSV(const std::vector<double>& history, const std::string& filename);

/**
 * @brief Utility function to export chromosome to CSV
 */
void exportChromosomeCSV(const Chromosome& chromosome, const std::string& filename);

} // namespace simulator

#endif // GENETIC_ALGORITHM_H
