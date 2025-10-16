#include "genetic_algorithm.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>

namespace simulator {

// ============================================================================
// Chromosome Implementation
// ============================================================================

void Chromosome::randomize(double minGreen, double maxGreen, double minRed, double maxRed, std::mt19937& rng) {
    std::uniform_real_distribution<double> greenDist(minGreen, maxGreen);
    std::uniform_real_distribution<double> redDist(minRed, maxRed);

    for (auto& gene : genes) {
        gene.greenTime = greenDist(rng);
        gene.redTime = redDist(rng);
    }
}

void Chromosome::mutate(double mutationRate, double mutationStdDev,
                        double minGreen, double maxGreen, double minRed, double maxRed,
                        std::mt19937& rng) {
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    std::normal_distribution<double> mutationDist(0.0, mutationStdDev);

    for (auto& gene : genes) {
        // Mutate green time
        if (probDist(rng) < mutationRate) {
            gene.greenTime += mutationDist(rng);
        }

        // Mutate red time
        if (probDist(rng) < mutationRate) {
            gene.redTime += mutationDist(rng);
        }
    }

    // Clamp to valid ranges
    clamp(minGreen, maxGreen, minRed, maxRed);
}

void Chromosome::clamp(double minGreen, double maxGreen, double minRed, double maxRed) {
    for (auto& gene : genes) {
        gene.greenTime = std::clamp(gene.greenTime, minGreen, maxGreen);
        gene.redTime = std::clamp(gene.redTime, minRed, maxRed);
    }
}

// ============================================================================
// GeneticAlgorithm Implementation
// ============================================================================

GeneticAlgorithm::GeneticAlgorithm(const Parameters& params, FitnessFunction fitnessFunc)
    : params_(params), rng_(params.seed), fitnessFunc_(fitnessFunc) {
}

void GeneticAlgorithm::initializePopulation(size_t chromosomeSize) {
    population_.clear();
    population_.reserve(params_.populationSize);

    for (size_t i = 0; i < params_.populationSize; ++i) {
        Chromosome chromosome(chromosomeSize);
        chromosome.randomize(params_.minGreenTime, params_.maxGreenTime,
                           params_.minRedTime, params_.maxRedTime, rng_);
        population_.push_back(chromosome);
    }

    std::cout << "Initialized population with " << params_.populationSize
              << " individuals, each with " << chromosomeSize << " traffic lights\n";
}

Chromosome GeneticAlgorithm::evolve() {
    std::cout << "\n=== Starting Genetic Algorithm Evolution ===\n";
    std::cout << "Population size: " << params_.populationSize << "\n";
    std::cout << "Generations: " << params_.generations << "\n";
    std::cout << "Mutation rate: " << params_.mutationRate << "\n";
    std::cout << "Crossover rate: " << params_.crossoverRate << "\n\n";

    // Evaluate initial population
    evaluatePopulation();
    sortPopulation();

    for (size_t gen = 0; gen < params_.generations; ++gen) {
        // Store best fitness
        fitnessHistory_.push_back(population_[0].fitness);

        // Print progress
        if (gen % 10 == 0 || gen == params_.generations - 1) {
            std::cout << "Generation " << gen << ": Best fitness = "
                      << population_[0].fitness << " (lower is better)\n";
        }

        // Create next generation
        reproduce();

        // Evaluate new generation
        evaluatePopulation();
        sortPopulation();
    }

    // Store best solution
    bestChromosome_ = population_[0];

    std::cout << "\n=== Evolution Complete ===\n";
    std::cout << "Final best fitness: " << bestChromosome_.fitness << "\n";

    return bestChromosome_;
}

Chromosome& GeneticAlgorithm::tournamentSelection() {
    std::uniform_int_distribution<size_t> dist(0, population_.size() - 1);

    size_t bestIdx = dist(rng_);
    double bestFitness = population_[bestIdx].fitness;

    for (size_t i = 1; i < params_.tournamentSize; ++i) {
        size_t idx = dist(rng_);
        if (population_[idx].fitness < bestFitness) {
            bestIdx = idx;
            bestFitness = population_[idx].fitness;
        }
    }

    return population_[bestIdx];
}

Chromosome GeneticAlgorithm::uniformCrossover(const Chromosome& parent1, const Chromosome& parent2) {
    Chromosome offspring(parent1.size());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (size_t i = 0; i < parent1.size(); ++i) {
        if (dist(rng_) < 0.5) {
            offspring.genes[i] = parent1.genes[i];
        } else {
            offspring.genes[i] = parent2.genes[i];
        }
    }

    return offspring;
}

void GeneticAlgorithm::evaluatePopulation() {
    for (auto& chromosome : population_) {
        chromosome.fitness = fitnessFunc_(chromosome);
    }
}

void GeneticAlgorithm::sortPopulation() {
    std::sort(population_.begin(), population_.end(),
              [](const Chromosome& a, const Chromosome& b) {
                  return a.fitness < b.fitness;  // Lower fitness is better
              });
}

void GeneticAlgorithm::reproduce() {
    std::vector<Chromosome> newPopulation;
    newPopulation.reserve(params_.populationSize);

    // Elitism: keep best individuals
    size_t eliteCount = static_cast<size_t>(params_.populationSize * params_.elitismRate);
    for (size_t i = 0; i < eliteCount; ++i) {
        newPopulation.push_back(population_[i]);
    }

    // Generate offspring
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    while (newPopulation.size() < params_.populationSize) {
        // Select parents
        Chromosome& parent1 = tournamentSelection();
        Chromosome& parent2 = tournamentSelection();

        // Crossover
        Chromosome offspring;
        if (dist(rng_) < params_.crossoverRate) {
            offspring = uniformCrossover(parent1, parent2);
        } else {
            // No crossover, just copy parent1
            offspring = parent1;
        }

        // Mutation
        offspring.mutate(params_.mutationRate, params_.mutationStdDev,
                        params_.minGreenTime, params_.maxGreenTime,
                        params_.minRedTime, params_.maxRedTime, rng_);

        newPopulation.push_back(offspring);
    }

    population_ = std::move(newPopulation);
}

// ============================================================================
// Utility Functions
// ============================================================================

void exportEvolutionHistoryCSV(const std::vector<double>& history, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    file << "generation,fitness\n";
    for (size_t i = 0; i < history.size(); ++i) {
        file << i << "," << history[i] << "\n";
    }

    file.close();
    std::cout << "Exported evolution history to " << filename << "\n";
}

void exportChromosomeCSV(const Chromosome& chromosome, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    file << "light_index,green_time,red_time\n";
    for (size_t i = 0; i < chromosome.genes.size(); ++i) {
        file << i << ","
             << chromosome.genes[i].greenTime << ","
             << chromosome.genes[i].redTime << "\n";
    }

    file.close();
    std::cout << "Exported chromosome to " << filename << "\n";
}

} // namespace simulator
