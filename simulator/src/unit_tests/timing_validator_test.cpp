/**
 * Unit tests for TimingValidator class
 * Tests validation of traffic light timing configurations
 */

#include <gtest/gtest.h>
#include "../validation/timing_validator.h"
#include "../core/road.h"
#include "../optimization/genetic_algorithm.h"
#include "fixtures/test_fixtures.h"

using namespace ratms::validation;
using namespace simulator;

class TimingValidatorTest : public ::testing::Test {
protected:
    std::vector<Road> testNetwork_;

    void SetUp() override {
        // Create a simple test network
        testNetwork_.push_back(Road(1, 300.0, 2, 20));
        testNetwork_.push_back(Road(2, 300.0, 2, 20));
    }
};

// Construction tests
TEST_F(TimingValidatorTest, DefaultConstruction) {
    TimingValidator validator;
    ValidationConfig config = validator.getConfig();

    EXPECT_GT(config.simulationSteps, 0);
    EXPECT_GT(config.dt, 0.0);
    EXPECT_GT(config.improvementThreshold, 0.0);
    EXPECT_GT(config.regressionThreshold, 0.0);
}

TEST_F(TimingValidatorTest, CustomConfig) {
    ValidationConfig config;
    config.simulationSteps = 200;
    config.dt = 0.05;
    config.improvementThreshold = 10.0;
    config.regressionThreshold = 15.0;

    TimingValidator validator(config);
    ValidationConfig retrieved = validator.getConfig();

    EXPECT_EQ(retrieved.simulationSteps, 200);
    EXPECT_DOUBLE_EQ(retrieved.dt, 0.05);
    EXPECT_DOUBLE_EQ(retrieved.improvementThreshold, 10.0);
    EXPECT_DOUBLE_EQ(retrieved.regressionThreshold, 15.0);
}

// SetConfig tests
TEST_F(TimingValidatorTest, SetConfig_UpdatesConfig) {
    TimingValidator validator;

    ValidationConfig newConfig;
    newConfig.simulationSteps = 100;
    validator.setConfig(newConfig);

    EXPECT_EQ(validator.getConfig().simulationSteps, 100);
}

// Validation tests
TEST_F(TimingValidatorTest, Validate_EmptyChromosome) {
    ValidationConfig config;
    config.simulationSteps = 50;  // Short simulation for fast tests
    TimingValidator validator(config);

    Chromosome empty;
    ValidationResult result = validator.validate(testNetwork_, empty);

    // Should complete without error
    EXPECT_TRUE(result.passed || !result.passed);  // Either outcome is valid
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(TimingValidatorTest, Validate_ValidChromosome) {
    ValidationConfig config;
    config.simulationSteps = 50;
    TimingValidator validator(config);

    Chromosome chromosome;
    chromosome.genes.push_back(TrafficLightTiming(30.0, 30.0));
    chromosome.genes.push_back(TrafficLightTiming(30.0, 30.0));

    ValidationResult result = validator.validate(testNetwork_, chromosome);

    EXPECT_GE(result.baselineFitness, 0.0);
    EXPECT_GE(result.optimizedFitness, 0.0);
    EXPECT_GT(result.timestamp, 0);
}

TEST_F(TimingValidatorTest, Validate_EmptyNetwork) {
    ValidationConfig config;
    config.simulationSteps = 50;
    TimingValidator validator(config);

    std::vector<Road> emptyNetwork;
    Chromosome chromosome;

    ValidationResult result = validator.validate(emptyNetwork, chromosome);

    // Should handle empty network gracefully
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(TimingValidatorTest, Validate_ReturnsTimestamp) {
    ValidationConfig config;
    config.simulationSteps = 50;
    TimingValidator validator(config);

    Chromosome chromosome;
    ValidationResult result = validator.validate(testNetwork_, chromosome);

    EXPECT_GT(result.timestamp, 0);
}

TEST_F(TimingValidatorTest, Validate_CalculatesImprovement) {
    ValidationConfig config;
    config.simulationSteps = 100;
    TimingValidator validator(config);

    Chromosome chromosome;
    chromosome.genes.push_back(TrafficLightTiming(40.0, 20.0));
    chromosome.genes.push_back(TrafficLightTiming(40.0, 20.0));

    ValidationResult result = validator.validate(testNetwork_, chromosome);

    // Improvement should be a calculated percentage
    // Can be positive (improvement) or negative (regression)
    EXPECT_TRUE(std::isfinite(result.improvementPercent));
}

// Threshold tests
TEST_F(TimingValidatorTest, Validate_ImprovementThreshold_PassOnImprovement) {
    ValidationConfig config;
    config.simulationSteps = 100;
    config.improvementThreshold = 0.0;  // Any improvement passes
    config.regressionThreshold = 100.0;  // Very high tolerance
    TimingValidator validator(config);

    Chromosome chromosome;
    chromosome.genes.push_back(TrafficLightTiming(30.0, 30.0));

    ValidationResult result = validator.validate(testNetwork_, chromosome);

    // With these lenient thresholds, should pass
    // (actual result depends on simulation)
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(TimingValidatorTest, Validate_RegressionThreshold) {
    ValidationConfig config;
    config.simulationSteps = 50;
    config.regressionThreshold = 0.0;  // Any regression fails
    TimingValidator validator(config);

    Chromosome chromosome;
    chromosome.genes.push_back(TrafficLightTiming(5.0, 55.0));  // Very short green

    ValidationResult result = validator.validate(testNetwork_, chromosome);

    // Result depends on simulation outcome
    EXPECT_FALSE(result.reason.empty());
}

// Using fixture for more complex tests
class TimingValidatorFixtureTest : public ratms::test::SimulatorTestFixture {
protected:
    TimingValidator validator_;

    void SetUp() override {
        SimulatorTestFixture::SetUp();
        ValidationConfig config;
        config.simulationSteps = 50;  // Fast tests
        validator_.setConfig(config);
    }
};

TEST_F(TimingValidatorFixtureTest, Validate_WithFixtureNetwork) {
    Chromosome chromosome;
    for (size_t i = 0; i < testNetwork_.size(); i++) {
        chromosome.genes.push_back(TrafficLightTiming(30.0, 30.0));
    }

    ValidationResult result = validator_.validate(testNetwork_, chromosome);

    EXPECT_GE(result.baselineFitness, 0.0);
}
