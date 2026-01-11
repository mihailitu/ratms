/**
 * Unit tests for TrafficLight class
 * Tests state machine transitions and timing
 */

#include <gtest/gtest.h>
#include "../core/trafficlight.h"

using namespace simulator;

class TrafficLightTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default light: 30s green, 3s yellow, 30s red
    }
};

// Construction tests
TEST_F(TrafficLightTest, DefaultConstruction) {
    TrafficLight tl;
    // Default constructed light state is undefined
    // Just verify it doesn't crash
    EXPECT_TRUE(tl.isGreen() || tl.isYellow() || tl.isRed());
}

TEST_F(TrafficLightTest, ParameterizedConstruction) {
    TrafficLight tl(30.0, 3.0, 30.0);  // 30s green, 3s yellow, 30s red
    EXPECT_TRUE(tl.isGreen());  // Starts green by default
    EXPECT_FALSE(tl.isYellow());
    EXPECT_FALSE(tl.isRed());
}

TEST_F(TrafficLightTest, ConstructWithInitialColor) {
    TrafficLight green(30.0, 3.0, 30.0, TrafficLight::green_light);
    EXPECT_TRUE(green.isGreen());

    TrafficLight yellow(30.0, 3.0, 30.0, TrafficLight::yellow_light);
    EXPECT_TRUE(yellow.isYellow());

    TrafficLight red(30.0, 3.0, 30.0, TrafficLight::red_light);
    EXPECT_TRUE(red.isRed());
}

// State transition tests
TEST_F(TrafficLightTest, GreenToYellowTransition) {
    TrafficLight tl(10.0, 3.0, 10.0, TrafficLight::green_light);
    EXPECT_TRUE(tl.isGreen());

    // Advance past green time
    for (int i = 0; i < 110; i++) {  // 11 seconds at 0.1s steps
        tl.update(0.1);
    }

    EXPECT_TRUE(tl.isYellow());
    EXPECT_FALSE(tl.isGreen());
    EXPECT_FALSE(tl.isRed());
}

TEST_F(TrafficLightTest, YellowToRedTransition) {
    TrafficLight tl(10.0, 3.0, 10.0, TrafficLight::yellow_light);
    EXPECT_TRUE(tl.isYellow());

    // Advance past yellow time (3 seconds)
    for (int i = 0; i < 35; i++) {  // 3.5 seconds at 0.1s steps
        tl.update(0.1);
    }

    EXPECT_TRUE(tl.isRed());
    EXPECT_FALSE(tl.isGreen());
    EXPECT_FALSE(tl.isYellow());
}

TEST_F(TrafficLightTest, RedToGreenTransition) {
    TrafficLight tl(10.0, 3.0, 10.0, TrafficLight::red_light);
    EXPECT_TRUE(tl.isRed());

    // Advance past red time (10 seconds)
    for (int i = 0; i < 110; i++) {  // 11 seconds at 0.1s steps
        tl.update(0.1);
    }

    EXPECT_TRUE(tl.isGreen());
    EXPECT_FALSE(tl.isRed());
    EXPECT_FALSE(tl.isYellow());
}

// Full cycle test
TEST_F(TrafficLightTest, FullCycleTransition) {
    TrafficLight tl(10.0, 3.0, 10.0, TrafficLight::green_light);

    // Start at green
    EXPECT_TRUE(tl.isGreen());

    // Run updates until we transition to yellow
    int iterations = 0;
    while (tl.isGreen() && iterations < 200) {
        tl.update(0.1);
        iterations++;
    }
    EXPECT_TRUE(tl.isYellow()) << "Failed to reach yellow after " << iterations << " iterations";
    EXPECT_GT(iterations, 0);  // Should have taken some iterations

    // Run updates until we transition to red
    iterations = 0;
    while (tl.isYellow() && iterations < 100) {
        tl.update(0.1);
        iterations++;
    }
    EXPECT_TRUE(tl.isRed()) << "Failed to reach red after " << iterations << " iterations";

    // Run updates until we transition back to green
    iterations = 0;
    while (tl.isRed() && iterations < 200) {
        tl.update(0.1);
        iterations++;
    }
    EXPECT_TRUE(tl.isGreen()) << "Failed to reach green after " << iterations << " iterations";
}

// Remaining time test
TEST_F(TrafficLightTest, RemainingTimeCalculation) {
    TrafficLight tl(30.0, 3.0, 30.0, TrafficLight::green_light);

    double remaining = tl.getRenainingTimeForCurrentColor();
    EXPECT_DOUBLE_EQ(remaining, 30.0);

    // Advance 10 seconds
    for (int i = 0; i < 100; i++) {
        tl.update(0.1);
    }

    remaining = tl.getRenainingTimeForCurrentColor();
    EXPECT_NEAR(remaining, 20.0, 0.5);  // Allow small floating point error
}

// State consistency tests
TEST_F(TrafficLightTest, MutuallyExclusiveStates) {
    TrafficLight tl(10.0, 3.0, 10.0);

    // At any point, exactly one state should be true
    for (int i = 0; i < 500; i++) {
        int stateCount = 0;
        if (tl.isGreen()) stateCount++;
        if (tl.isYellow()) stateCount++;
        if (tl.isRed()) stateCount++;

        EXPECT_EQ(stateCount, 1) << "Multiple states active at iteration " << i;

        tl.update(0.1);
    }
}

// Edge cases
TEST_F(TrafficLightTest, ZeroGreenTime) {
    // Edge case: 0 green time should immediately transition
    TrafficLight tl(0.0, 3.0, 10.0, TrafficLight::green_light);

    tl.update(0.1);
    EXPECT_FALSE(tl.isGreen());
}

TEST_F(TrafficLightTest, SmallTimeStep) {
    TrafficLight tl(10.0, 3.0, 10.0, TrafficLight::green_light);

    // Use very small time steps - need > 10s to transition
    for (int i = 0; i < 10100; i++) {  // 10.1 seconds at 0.001s steps
        tl.update(0.001);
    }

    // Should have transitioned to yellow (counter >= 10.0)
    EXPECT_TRUE(tl.isYellow());
}

TEST_F(TrafficLightTest, LargeTimeStep) {
    TrafficLight tl(10.0, 3.0, 10.0, TrafficLight::green_light);

    // The check happens BEFORE adding dt to counter:
    // - First update: check(0 >= 10) = false, counter = 0 + 15 = 15
    // - Second update: check(15 >= 10) = true, transition, counter = 0 + 15 = 15

    tl.update(15.0);  // counter=15, still green (check was before adding dt)
    EXPECT_TRUE(tl.isGreen());  // Still green after first update!

    tl.update(0.1);  // Now check(15 >= 10) triggers transition
    EXPECT_FALSE(tl.isGreen());  // Should be yellow now
}
