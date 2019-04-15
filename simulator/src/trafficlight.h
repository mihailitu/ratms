#ifndef TRAFFICLIGHT_H
#define TRAFFICLIGHT_H

#include <vector>

namespace simulator
{

/* TrafficLight class
 * Holds information about lights.
 * Light sequence: green - yellow - red - green - yellow - red
 * TODO: some lights have light sequence as green - yellow - red - yellow - green - etc.
 *      see if this second possible sequence influence the traffic or the yellow between red and green can be simply added to red light.
 *
 */
class TrafficLight
{
public:
    enum LightColor{
        green_light, // green light
        yellow_light, // yellow from green to red (this is usually unchangeable
        red_light, // red light
    };

private:
    // light time counter. Resets to zero
    double counter;

    // current color
    LightColor currentLightColor;

    std::vector<LightColor> lightCycle = {green_light, yellow_light, red_light};
    std::vector<double> lightsTime = {0.0, 0.0, 0.0};
public:
    TrafficLight();
    TrafficLight(double g, double y, double r, LightColor initialColor = green_light, double startTime = 0);

    void setSequence(double g, double y, double r, LightColor initialColor = green_light, double startTime = 0);

    bool isYellow() const;
    bool isRed() const;
    bool isGreen() const;
    void update(double dt);
    double getRenainingTimeForCurrentColor();
};

} // namespace simulator
#endif // TRAFFICLIGHT_H
