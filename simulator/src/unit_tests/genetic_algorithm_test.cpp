/**
 * Unit tests for Genetic Algorithm classes
 * Tests Chromosome operations and GA evolution
 */

#include <gtest/gtest.h>
#include "../optimization/genetic_algorithm.h"
#include <cmath>
#include <numeric>

using namespace simulator;

// =============================================================================
// Chromosome Tests
// =============================================================================

class ChromosomeTest : public ::testing::Test {
protected:
    std::mt19937 rng{42};
};

TEST_F(ChromosomeTest, DefaultConstruction) {
    Chromosome c;
    EXPECT_EQ(c.size(), 0);
    EXPECT_DOUBLE_EQ(c.fitness, 0.0);
}

TEST_F(ChromosomeTest, SizedConstruction) {
    Chromosome c(10);
    EXPECT_EQ(c.size(), 10);
    EXPECT_DOUBLE_EQ(c.fitness, 0.0);
}

TEST_F(ChromosomeTest, Randomization) {
    Chromosome c(10);
    c.randomize(10.0, 60.0, 10.0, 60.0, rng);

    EXPECT_EQ(c.size(), 10);

    for (const auto& gene : c.genes) {
        EXPECT_GE(gene.greenTime, 10.0);
        EXPECT_LE(gene.greenTime, 60.0);
        EXPECT_GE(gene.redTime, 10.0);
        EXPECT_LE(gene.redTime, 60.0);
    }
}

TEST_F(ChromosomeTest, Clamping) {
    Chromosome c(3);
    c.genes[0] = TrafficLightTiming(5.0, 5.0);    // Below min
    c.genes[1] = TrafficLightTiming(100.0, 100.0); // Above max
    c.genes[2] = TrafficLightTiming(30.0, 30.0);  // Within bounds

    c.clamp(10.0, 60.0, 10.0, 60.0);

    EXPECT_DOUBLE_EQ(c.genes[0].greenTime, 10.0);  // Clamped to min
    EXPECT_DOUBLE_EQ(c.genes[0].redTime, 10.0);
    EXPECT_DOUBLE_EQ(c.genes[1].greenTime, 60.0);  // Clamped to max
    EXPECT_DOUBLE_EQ(c.genes[1].redTime, 60.0);
    EXPECT_DOUBLE_EQ(c.genes[2].greenTime, 30.0);  // Unchanged
    EXPECT_DOUBLE_EQ(c.genes[2].redTime, 30.0);
}

TEST_F(ChromosomeTest, MutationChangesValues) {
    Chromosome c(20);
    c.randomize(10.0, 60.0, 10.0, 60.0, rng);

    // Store original values
    std::vector<double> originalGreen;
    for (const auto& gene : c.genes) {
        originalGreen.push_back(gene.greenTime);
    }

    // Apply mutation with high rate
    c.mutate(1.0, 5.0, 10.0, 60.0, 10.0, 60.0, rng);

    // At least some values should have changed
    int changedCount = 0;
    for (size_t i = 0; i < c.size(); i++) {
        if (std::abs(c.genes[i].greenTime - originalGreen[i]) > 0.01) {
            changedCount++;
        }
    }

    EXPECT_GT(changedCount, 0);
}

TEST_F(ChromosomeTest, MutationStaysWithinBounds) {
    Chromosome c(50);
    c.randomize(10.0, 60.0, 10.0, 60.0, rng);

    // Mutate many times
    for (int i = 0; i < 100; i++) {
        c.mutate(0.5, 10.0, 10.0, 60.0, 10.0, 60.0, rng);
    }

    // All values should still be within bounds
    for (const auto& gene : c.genes) {
        EXPECT_GE(gene.greenTime, 10.0);
        EXPECT_LE(gene.greenTime, 60.0);
        EXPECT_GE(gene.redTime, 10.0);
        EXPECT_LE(gene.redTime, 60.0);
    }
}

// =============================================================================
// Genetic Algorithm Tests
// =============================================================================

class GeneticAlgorithmTest : public ::testing::Test {
protected:
    GeneticAlgorithm::Parameters params;

    void SetUp() override {
        params.populationSize = 20;
        params.generations = 10;
        params.mutationRate = 0.1;
        params.crossoverRate = 0.8;
        params.tournamentSize = 3;
        params.elitismRate = 0.1;
        params.minGreenTime = 10.0;
        params.maxGreenTime = 60.0;
        params.minRedTime = 10.0;
        params.maxRedTime = 60.0;
        params.seed = 42;
    }

    // Simple fitness function for testing
    static double simpleFitness(const Chromosome& c) {
        // Fitness = sum of squared deviations from 30
        double fitness = 0.0;
        for (const auto& gene : c.genes) {
            fitness += std::pow(gene.greenTime - 30.0, 2);
            fitness += std::pow(gene.redTime - 30.0, 2);
        }
        return fitness;
    }
};

TEST_F(GeneticAlgorithmTest, Construction) {
    GeneticAlgorithm ga(params, simpleFitness);
    // Just verify construction doesn't crash
}

TEST_F(GeneticAlgorithmTest, PopulationInitialization) {
    GeneticAlgorithm ga(params, simpleFitness);
    ga.initializePopulation(10);  // 10 traffic lights
    // Verify initialization doesn't crash
}

TEST_F(GeneticAlgorithmTest, EvolutionImprovesFitness) {
    params.generations = 50;
    params.populationSize = 30;

    GeneticAlgorithm ga(params, simpleFitness);
    ga.initializePopulation(5);  // 5 traffic lights

    Chromosome best = ga.evolve();

    const auto& history = ga.getFitnessHistory();

    EXPECT_FALSE(history.empty());

    // Final fitness should be better (lower) than or equal to initial
    double initialFitness = history.front();
    double finalFitness = history.back();

    EXPECT_LE(finalFitness, initialFitness);
}

TEST_F(GeneticAlgorithmTest, BestChromosomeAccessible) {
    GeneticAlgorithm ga(params, simpleFitness);
    ga.initializePopulation(5);

    Chromosome evolved = ga.evolve();
    const Chromosome& best = ga.getBestChromosome();

    EXPECT_EQ(evolved.size(), best.size());
    EXPECT_DOUBLE_EQ(evolved.fitness, best.fitness);
}

TEST_F(GeneticAlgorithmTest, FitnessHistoryRecorded) {
    params.generations = 20;

    GeneticAlgorithm ga(params, simpleFitness);
    ga.initializePopulation(5);
    ga.evolve();

    const auto& history = ga.getFitnessHistory();

    EXPECT_EQ(history.size(), params.generations);

    // History should be monotonically non-increasing (best fitness tracked)
    for (size_t i = 1; i < history.size(); i++) {
        EXPECT_LE(history[i], history[i-1] + 0.001);  // Allow small tolerance
    }
}

TEST_F(GeneticAlgorithmTest, DifferentSeedsProduceDifferentResults) {
    params.generations = 20;

    params.seed = 42;
    GeneticAlgorithm ga1(params, simpleFitness);
    ga1.initializePopulation(5);
    Chromosome best1 = ga1.evolve();

    params.seed = 123;
    GeneticAlgorithm ga2(params, simpleFitness);
    ga2.initializePopulation(5);
    Chromosome best2 = ga2.evolve();

    // Results should differ (unless extremely unlikely)
    bool different = false;
    for (size_t i = 0; i < best1.size(); i++) {
        if (std::abs(best1.genes[i].greenTime - best2.genes[i].greenTime) > 0.1) {
            different = true;
            break;
        }
    }

    EXPECT_TRUE(different);
}

TEST_F(GeneticAlgorithmTest, SameSeedProducesSameResults) {
    params.generations = 10;

    params.seed = 42;
    GeneticAlgorithm ga1(params, simpleFitness);
    ga1.initializePopulation(5);
    Chromosome best1 = ga1.evolve();

    params.seed = 42;
    GeneticAlgorithm ga2(params, simpleFitness);
    ga2.initializePopulation(5);
    Chromosome best2 = ga2.evolve();

    // Results should be identical with same seed
    EXPECT_DOUBLE_EQ(best1.fitness, best2.fitness);
    for (size_t i = 0; i < best1.size(); i++) {
        EXPECT_DOUBLE_EQ(best1.genes[i].greenTime, best2.genes[i].greenTime);
        EXPECT_DOUBLE_EQ(best1.genes[i].redTime, best2.genes[i].redTime);
    }
}

TEST_F(GeneticAlgorithmTest, ElitismPreservesBest) {
    params.generations = 5;
    params.elitismRate = 0.5;  // 50% elitism
    params.populationSize = 10;

    GeneticAlgorithm ga(params, simpleFitness);
    ga.initializePopulation(3);
    ga.evolve();

    // Best fitness should never get worse between generations
    const auto& history = ga.getFitnessHistory();
    for (size_t i = 1; i < history.size(); i++) {
        EXPECT_LE(history[i], history[i-1] + 0.0001);
    }
}

TEST_F(GeneticAlgorithmTest, SmallPopulationHandled) {
    params.populationSize = 5;
    params.tournamentSize = 2;  // Must be <= population size
    params.generations = 5;

    GeneticAlgorithm ga(params, simpleFitness);
    ga.initializePopulation(3);

    // Should not crash with small population
    Chromosome best = ga.evolve();
    EXPECT_EQ(best.size(), 3);
}

TEST_F(GeneticAlgorithmTest, LargeChromosomeHandled) {
    params.generations = 5;

    GeneticAlgorithm ga(params, simpleFitness);
    ga.initializePopulation(100);  // 100 traffic lights

    Chromosome best = ga.evolve();
    EXPECT_EQ(best.size(), 100);
}

// =============================================================================
// TrafficLightTiming Tests
// =============================================================================

TEST(TrafficLightTimingTest, DefaultConstruction) {
    TrafficLightTiming t;
    EXPECT_DOUBLE_EQ(t.greenTime, 30.0);
    EXPECT_DOUBLE_EQ(t.redTime, 30.0);
}

TEST(TrafficLightTimingTest, ParameterizedConstruction) {
    TrafficLightTiming t(15.0, 45.0);
    EXPECT_DOUBLE_EQ(t.greenTime, 15.0);
    EXPECT_DOUBLE_EQ(t.redTime, 45.0);
}
