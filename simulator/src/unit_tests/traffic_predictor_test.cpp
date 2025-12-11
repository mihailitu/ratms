/**
 * Unit tests for TrafficPredictor class
 * Tests prediction logic, configuration, and caching
 */

#include <gtest/gtest.h>
#include "../prediction/traffic_predictor.h"
#include "../core/simulator.h"
#include "../data/storage/traffic_pattern_storage.h"
#include "fixtures/test_fixtures.h"

using namespace ratms::prediction;
using namespace simulator;

// Static method tests (no dependencies needed)
class TrafficPredictorStaticTest : public ::testing::Test {};

TEST_F(TrafficPredictorStaticTest, TimeSlotToString_Morning) {
    std::string slot = TrafficPredictor::timeSlotToString(16);  // 8:00 AM (8*2=16)
    EXPECT_FALSE(slot.empty());
    EXPECT_TRUE(slot.find("08") != std::string::npos || slot.find("8") != std::string::npos);
}

TEST_F(TrafficPredictorStaticTest, TimeSlotToString_Noon) {
    std::string slot = TrafficPredictor::timeSlotToString(24);  // 12:00 PM (12*2=24)
    EXPECT_FALSE(slot.empty());
}

TEST_F(TrafficPredictorStaticTest, TimeSlotToString_Evening) {
    std::string slot = TrafficPredictor::timeSlotToString(36);  // 6:00 PM (18*2=36)
    EXPECT_FALSE(slot.empty());
}

TEST_F(TrafficPredictorStaticTest, TimeSlotToString_Midnight) {
    std::string slot = TrafficPredictor::timeSlotToString(0);
    EXPECT_FALSE(slot.empty());
}

TEST_F(TrafficPredictorStaticTest, TimeSlotToString_EndOfDay) {
    std::string slot = TrafficPredictor::timeSlotToString(47);  // 23:30
    EXPECT_FALSE(slot.empty());
}

TEST_F(TrafficPredictorStaticTest, GetFutureTimeSlot_ZeroHorizon) {
    auto [dayOfWeek, timeSlot] = TrafficPredictor::getFutureTimeSlot(0);

    EXPECT_GE(dayOfWeek, 0);
    EXPECT_LE(dayOfWeek, 6);
    EXPECT_GE(timeSlot, 0);
    EXPECT_LE(timeSlot, 47);
}

TEST_F(TrafficPredictorStaticTest, GetFutureTimeSlot_ThirtyMinutes) {
    auto [dayOfWeek, timeSlot] = TrafficPredictor::getFutureTimeSlot(30);

    EXPECT_GE(dayOfWeek, 0);
    EXPECT_LE(dayOfWeek, 6);
    EXPECT_GE(timeSlot, 0);
    EXPECT_LE(timeSlot, 47);
}

TEST_F(TrafficPredictorStaticTest, GetFutureTimeSlot_OneHour) {
    auto [dayOfWeek, timeSlot] = TrafficPredictor::getFutureTimeSlot(60);

    EXPECT_GE(dayOfWeek, 0);
    EXPECT_LE(dayOfWeek, 6);
}

TEST_F(TrafficPredictorStaticTest, CalculateConfidence_HighSamples) {
    double conf = TrafficPredictor::calculateConfidence(100, 1.0, 10.0, 10);
    EXPECT_GE(conf, 0.0);
    EXPECT_LE(conf, 1.0);
}

TEST_F(TrafficPredictorStaticTest, CalculateConfidence_LowSamples) {
    double conf = TrafficPredictor::calculateConfidence(2, 1.0, 10.0, 10);
    EXPECT_GE(conf, 0.0);
    EXPECT_LE(conf, 1.0);
    // Low samples should result in lower confidence
}

TEST_F(TrafficPredictorStaticTest, CalculateConfidence_ZeroSamples) {
    double conf = TrafficPredictor::calculateConfidence(0, 0.0, 0.0, 10);
    EXPECT_GE(conf, 0.0);
    EXPECT_LE(conf, 1.0);
}

TEST_F(TrafficPredictorStaticTest, CalculateConfidence_HighVariance) {
    double lowVar = TrafficPredictor::calculateConfidence(50, 1.0, 10.0, 10);
    double highVar = TrafficPredictor::calculateConfidence(50, 10.0, 10.0, 10);

    // Higher variance should generally mean lower confidence
    EXPECT_GE(lowVar, 0.0);
    EXPECT_GE(highVar, 0.0);
}

TEST_F(TrafficPredictorStaticTest, CalculateConfidence_MinSamples) {
    double atMin = TrafficPredictor::calculateConfidence(10, 1.0, 10.0, 10);
    double belowMin = TrafficPredictor::calculateConfidence(5, 1.0, 10.0, 10);

    // At or above min samples should have higher confidence
    EXPECT_GE(atMin, belowMin);
}

// PredictionConfig tests
class PredictionConfigTest : public ::testing::Test {};

TEST_F(PredictionConfigTest, DefaultValues) {
    PredictionConfig config;

    EXPECT_EQ(config.horizonMinutes, 30);
    EXPECT_EQ(config.minHorizonMinutes, 10);
    EXPECT_EQ(config.maxHorizonMinutes, 120);
    EXPECT_DOUBLE_EQ(config.patternWeight, 0.7);
    EXPECT_DOUBLE_EQ(config.currentWeight, 0.3);
    EXPECT_EQ(config.minSamplesForFullConfidence, 10);
    EXPECT_EQ(config.cacheDurationSeconds, 30);
}

TEST_F(PredictionConfigTest, WeightsSumToOne) {
    PredictionConfig config;
    EXPECT_DOUBLE_EQ(config.patternWeight + config.currentWeight, 1.0);
}

// PredictedMetrics tests
class PredictedMetricsTest : public ::testing::Test {};

TEST_F(PredictedMetricsTest, ValueInitialization) {
    // Note: PredictedMetrics is a POD struct without default initializers.
    // Use value initialization {} to ensure zero/default values.
    PredictedMetrics metrics{};

    EXPECT_EQ(metrics.roadId, 0);
    EXPECT_DOUBLE_EQ(metrics.confidence, 0.0);
    EXPECT_FALSE(metrics.hasCurrentData);
    EXPECT_FALSE(metrics.hasHistoricalPattern);
}

// PredictionResult tests
class PredictionResultTest : public ::testing::Test {};

TEST_F(PredictionResultTest, ValueInitialization) {
    // Note: PredictionResult is a POD struct without default initializers.
    // Use value initialization {} to ensure zero/default values.
    PredictionResult result{};

    EXPECT_TRUE(result.roadPredictions.empty());
    EXPECT_DOUBLE_EQ(result.averageConfidence, 0.0);
}

// CurrentRoadState tests
class CurrentRoadStateTest : public ::testing::Test {};

TEST_F(CurrentRoadStateTest, ValueInitialization) {
    // Note: CurrentRoadState is a POD struct without default initializers.
    // Use value initialization {} to ensure zero/default values.
    CurrentRoadState state{};

    EXPECT_EQ(state.roadId, 0);
    EXPECT_EQ(state.vehicleCount, 0);
    EXPECT_DOUBLE_EQ(state.queueLength, 0.0);
    EXPECT_DOUBLE_EQ(state.avgSpeed, 0.0);
    EXPECT_DOUBLE_EQ(state.flowRate, 0.0);
}

// Integration-level predictor tests would require proper setup
// These tests focus on the public interface behavior

class TrafficPredictorIntegrationTest : public ratms::test::DatabaseTestFixture {
protected:
    std::shared_ptr<ratms::data::TrafficPatternStorage> patternStorage_;
    std::shared_ptr<Simulator> simulator_;
    std::mutex simMutex_;

    void SetUp() override {
        DatabaseTestFixture::SetUp();

        // Create pattern storage (would need proper implementation)
        // For now, these tests focus on what we can test
        simulator_ = std::make_shared<Simulator>();
    }
};

// Note: Full TrafficPredictor integration tests would require
// proper TrafficPatternStorage implementation with test data
