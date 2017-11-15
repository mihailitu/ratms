#include "trafficlight.h"

namespace simulator {

TrafficLight::TrafficLight()
{
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
    case TrafficLight::red_light:
        return TrafficLight::green_light;
    default:
        return TrafficLight::green_light; // to get rid of the "warning: control reaches end of non-void function" warning
    }
}

void TrafficLight::update(double dt)
{
    if(counter >= lightsTime[currentLightColor]) {
        counter = 0;
        currentLightColor = nextColor(currentLightColor);
    }
    counter += dt;
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
