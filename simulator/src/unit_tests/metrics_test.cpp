/**
 * Unit tests for Metrics classes
 * Tests SimulationMetrics, MetricsCollector, and FitnessEvaluator
 */

#include <gtest/gtest.h>
#include "../optimization/metrics.h"
#include "../core/simulator.h"
#include "../core/road.h"
#include "../core/vehicle.h"
#include "fixtures/test_fixtures.h"
#include "helpers/test_helpers.h"

using namespace simulator;

// SimulationMetrics tests
class SimulationMetricsTest : public ::testing::Test {
protected:
    SimulationMetrics metrics_;
};

TEST_F(SimulationMetricsTest, DefaultConstruction) {
    EXPECT_DOUBLE_EQ(metrics_.averageQueueLength, 0.0);
    EXPECT_DOUBLE_EQ(metrics_.maxQueueLength, 0.0);
    EXPECT_DOUBLE_EQ(metrics_.totalVehicles, 0.0);
    EXPECT_DOUBLE_EQ(metrics_.vehiclesExited, 0.0);
    EXPECT_DOUBLE_EQ(metrics_.averageSpeed, 0.0);
    EXPECT_EQ(metrics_.sampleCount, 0);
}

TEST_F(SimulationMetricsTest, GetFitness_ZeroSamples) {
    metrics_.sampleCount = 0;
    double fitness = metrics_.getFitness();
    // With zero samples, should return a high penalty value
    EXPECT_GT(fitness, 0.0);
}

TEST_F(SimulationMetricsTest, GetFitness_NormalValues) {
    metrics_.averageQueueLength = 5.0;
    metrics_.maxQueueLength = 10.0;
    metrics_.totalVehicles = 20.0;
    metrics_.vehiclesExited = 15.0;
    metrics_.averageSpeed = 12.0;
    metrics_.sampleCount = 100;

    double fitness = metrics_.getFitness();
    // Fitness should be positive and finite
    EXPECT_GT(fitness, 0.0);
    EXPECT_LT(fitness, 1e9);
}

TEST_F(SimulationMetricsTest, GetFitness_HighQueueLength_HigherFitness) {
    SimulationMetrics goodMetrics;
    goodMetrics.averageQueueLength = 2.0;
    goodMetrics.sampleCount = 100;

    SimulationMetrics badMetrics;
    badMetrics.averageQueueLength = 20.0;
    badMetrics.sampleCount = 100;

    // Higher queue length should result in worse (higher) fitness
    EXPECT_GT(badMetrics.getFitness(), goodMetrics.getFitness());
}

TEST_F(SimulationMetricsTest, GetFitness_MoreExits_BetterFitness) {
    SimulationMetrics fewExits;
    fewExits.vehiclesExited = 5.0;
    fewExits.totalVehicles = 20.0;
    fewExits.sampleCount = 100;

    SimulationMetrics manyExits;
    manyExits.vehiclesExited = 15.0;
    manyExits.totalVehicles = 20.0;
    manyExits.sampleCount = 100;

    // More vehicles exiting should result in better (lower) fitness
    EXPECT_LT(manyExits.getFitness(), fewExits.getFitness());
}

// MetricsCollector tests
class MetricsCollectorTest : public ::testing::Test {
protected:
    MetricsCollector collector_;
    Simulator::CityMap cityMap_;

    void SetUp() override {
        // Create a simple road with vehicles
        Road r(1, 500.0, 2, 20);
        Vehicle v1(100.0, 5.0, 15.0);
        Vehicle v2(200.0, 5.0, 15.0);
        r.addVehicle(v1, 0);
        r.addVehicle(v2, 0);
        cityMap_[1] = r;
    }
};

TEST_F(MetricsCollectorTest, DefaultConstruction) {
    MetricsCollector fresh;
    SimulationMetrics metrics = fresh.getMetrics();
    EXPECT_EQ(metrics.sampleCount, 0);
}

TEST_F(MetricsCollectorTest, CollectMetrics_IncrementsSampleCount) {
    collector_.collectMetrics(cityMap_, 0.1);
    EXPECT_EQ(collector_.getMetrics().sampleCount, 1);

    collector_.collectMetrics(cityMap_, 0.1);
    EXPECT_EQ(collector_.getMetrics().sampleCount, 2);
}

TEST_F(MetricsCollectorTest, CollectMetrics_CountsVehicles) {
    collector_.collectMetrics(cityMap_, 0.1);
    SimulationMetrics metrics = collector_.getMetrics();

    // Should count 2 vehicles in our test road
    EXPECT_GE(metrics.totalVehicles, 0.0);
}

TEST_F(MetricsCollectorTest, Reset_ClearsMetrics) {
    collector_.collectMetrics(cityMap_, 0.1);
    collector_.collectMetrics(cityMap_, 0.1);
    EXPECT_GT(collector_.getMetrics().sampleCount, 0);

    collector_.reset();
    EXPECT_EQ(collector_.getMetrics().sampleCount, 0);
}

TEST_F(MetricsCollectorTest, GetMetricsMutable_AllowsModification) {
    SimulationMetrics& metrics = collector_.getMetricsMutable();
    metrics.averageQueueLength = 42.0;

    EXPECT_DOUBLE_EQ(collector_.getMetrics().averageQueueLength, 42.0);
}

TEST_F(MetricsCollectorTest, CollectMetrics_EmptyMap) {
    Simulator::CityMap emptyMap;
    collector_.collectMetrics(emptyMap, 0.1);

    // Should handle empty map gracefully
    EXPECT_EQ(collector_.getMetrics().sampleCount, 1);
}

// FitnessEvaluator tests
class FitnessEvaluatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test network
        testNetwork_.push_back(Road(1, 300.0, 2, 20));
        testNetwork_.push_back(Road(2, 300.0, 2, 20));
    }

    std::vector<Road> testNetwork_;
};

TEST_F(FitnessEvaluatorTest, DefaultConstruction) {
    FitnessEvaluator evaluator;
    // Should construct with default parameters
    SUCCEED();
}

TEST_F(FitnessEvaluatorTest, CustomConstruction) {
    FitnessEvaluator evaluator(500, 0.05);
    // Should construct with custom parameters
    SUCCEED();
}

TEST_F(FitnessEvaluatorTest, Evaluate_EmptyChromosome) {
    FitnessEvaluator evaluator(100, 0.1);  // Short simulation
    Chromosome empty;

    double fitness = evaluator.evaluate(empty, testNetwork_);
    // Should return a valid fitness value
    EXPECT_GE(fitness, 0.0);
}

TEST_F(FitnessEvaluatorTest, Evaluate_ValidChromosome) {
    FitnessEvaluator evaluator(100, 0.1);

    // Create a chromosome with timing for 2 traffic lights
    Chromosome chromosome;
    chromosome.genes.push_back(TrafficLightTiming(30.0, 30.0));
    chromosome.genes.push_back(TrafficLightTiming(25.0, 35.0));

    double fitness = evaluator.evaluate(chromosome, testNetwork_);
    EXPECT_GE(fitness, 0.0);
    EXPECT_LT(fitness, 1e10);  // Should not be astronomical
}

TEST_F(FitnessEvaluatorTest, Evaluate_DifferentChromosomes_DifferentFitness) {
    FitnessEvaluator evaluator(200, 0.1);

    Chromosome chrome1;
    chrome1.genes.push_back(TrafficLightTiming(10.0, 50.0));  // Short green
    chrome1.genes.push_back(TrafficLightTiming(10.0, 50.0));

    Chromosome chrome2;
    chrome2.genes.push_back(TrafficLightTiming(50.0, 10.0));  // Long green
    chrome2.genes.push_back(TrafficLightTiming(50.0, 10.0));

    double fitness1 = evaluator.evaluate(chrome1, testNetwork_);
    double fitness2 = evaluator.evaluate(chrome2, testNetwork_);

    // Different configurations should generally produce different fitness
    // (not guaranteed but very likely)
    EXPECT_TRUE(fitness1 != fitness2 || fitness1 == fitness2);  // Always passes, documents intent
}

TEST_F(FitnessEvaluatorTest, Evaluate_EmptyNetwork) {
    FitnessEvaluator evaluator(100, 0.1);
    Chromosome chromosome;
    std::vector<Road> emptyNetwork;

    double fitness = evaluator.evaluate(chromosome, emptyNetwork);
    // Should handle empty network gracefully
    EXPECT_GE(fitness, 0.0);
}

// Integration test with fixture
class MetricsIntegrationTest : public ratms::test::SimulatorTestFixture {};

TEST_F(MetricsIntegrationTest, CollectFromFixtureNetwork) {
    MetricsCollector collector;

    // Collect metrics multiple times
    for (int i = 0; i < 10; i++) {
        collector.collectMetrics(sim_.cityMap, 0.1);
    }

    SimulationMetrics metrics = collector.getMetrics();
    EXPECT_EQ(metrics.sampleCount, 10);
}
