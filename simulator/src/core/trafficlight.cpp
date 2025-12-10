#include "trafficlight.h"
#include "../utils/logger.h"
#include <cstdlib>

using namespace ratms;

namespace simulator {

TrafficLight::TrafficLight()
    : counter(0.0)
    , currentLightColor(green_light)
{
    // Set sensible default timings: green=30s, yellow=3s, red=27s (60s total cycle)
    lightsTime[green_light] = 30.0;
    lightsTime[yellow_light] = 3.0;
    lightsTime[red_light] = 27.0;

    // Randomize starting phase to prevent all lights being synchronized
    // This creates more realistic traffic patterns
    double totalCycle = lightsTime[green_light] + lightsTime[yellow_light] + lightsTime[red_light];
    double randomOffset = static_cast<double>(rand() % static_cast<int>(totalCycle));

    // Set the current phase based on the random offset
    if (randomOffset < lightsTime[green_light]) {
        currentLightColor = green_light;
        counter = randomOffset;
    } else if (randomOffset < lightsTime[green_light] + lightsTime[yellow_light]) {
        currentLightColor = yellow_light;
        counter = randomOffset - lightsTime[green_light];
    } else {
        currentLightColor = red_light;
        counter = randomOffset - lightsTime[green_light] - lightsTime[yellow_light];
    }
}

TrafficLight::TrafficLight(double g, double y, double r, LightColor initialColor, double startTime):
    counter(startTime), currentLightColor(initialColor)
{
    lightsTime[green_light] = g;
    lightsTime[yellow_light] = y;
    lightsTime[red_light] = r;
}

TrafficLight::LightColor nextColor(TrafficLight::LightColor current)
{
    switch (current) {
    case TrafficLight::green_light:
        return TrafficLight::yellow_light;
    case TrafficLight::yellow_light:
        return TrafficLight::red_light;
    default: // case TrafficLight::red_light:
        return TrafficLight::green_light;
    }
}

static char lightColorToChar(TrafficLight::LightColor color) {
    switch (color) {
    case TrafficLight::green_light: return 'G';
    case TrafficLight::yellow_light: return 'Y';
    case TrafficLight::red_light: return 'R';
    default: return '?';
    }
}

void TrafficLight::update(double dt)
{
    if(counter >= lightsTime[currentLightColor]) {
        LightColor oldColor = currentLightColor;
        counter = 0;
        currentLightColor = nextColor(currentLightColor);
        LOG_TRACE(LogComponent::Core, "Traffic light state change: {} -> {}",
                  lightColorToChar(oldColor), lightColorToChar(currentLightColor));
    }
    counter += dt;
}

double TrafficLight::getRenainingTimeForCurrentColor()
{
    return lightsTime[currentLightColor] - counter;
}

bool TrafficLight::isYellow() const
{
    return currentLightColor == yellow_light;
}

bool TrafficLight::isRed() const
{
    return currentLightColor == red_light;
}

bool TrafficLight::isGreen() const
{
    return currentLightColor == green_light;
}

} // namespace simulator
