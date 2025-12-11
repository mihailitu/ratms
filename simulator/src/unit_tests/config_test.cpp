/**
 * Unit tests for Config class
 * Tests configuration constants and default values
 */

#include <gtest/gtest.h>
#include "../utils/config.h"

using namespace simulator;

class ConfigTest : public ::testing::Test {};

TEST_F(ConfigTest, DT_HasExpectedValue) {
    // DT is 0.5 seconds (simulation time step)
    // Note: The comment in config.h says "0.5 seconds"
    EXPECT_DOUBLE_EQ(Config::DT, 0.5);
}

TEST_F(ConfigTest, DT_IsPositive) {
    EXPECT_GT(Config::DT, 0.0);
}

TEST_F(ConfigTest, DT_IsReasonable) {
    // DT should be small enough for accurate physics
    EXPECT_LE(Config::DT, 1.0);
    // But not too small (performance)
    EXPECT_GE(Config::DT, 0.01);
}

TEST_F(ConfigTest, SimulationTime_IsPositive) {
    EXPECT_GT(Config::simulationTime, 0);
}

TEST_F(ConfigTest, OutputPaths_AreNotEmpty) {
    EXPECT_FALSE(Config::simulatorOuput.empty());
    EXPECT_FALSE(Config::simulatorMap.empty());
}
