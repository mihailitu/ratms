#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <map>

#include "road.h"
class Simulator
{
    /***
     * IDM - Intelligent driver model
     * https://en.wikipedia.org/wiki/Intelligent_driver_model
     */

    // TODO: maybe convert all parameters to int by multiplying with 10 or 100?
    /* model parameters */

    double v0;      // Desired velocity - TODO: move this into Vehicle class, as some drivers would want to go above speed limit,
                    //                    while others will want to go lower than speed limit, determined by statistics
    double T;       // Safe time headway
    double a;       // Maximum acceleration
    double b;       // Desired deceleration
    double delta;   // Acceleration exponent
    double s0;      // Minimum distance - TODO: move this into Vehicle class, as some drivers are more agressive, while others are less agressive

    void initSimulatorTestState();

public:
    typedef std::map<Road::roadID, Road> CityMap;
    CityMap cityMap;
public:
    Simulator();
    Simulator( double _v0, double _T, double _a, double _b, double _delta, double s0 );

    void runSimulator();
    void runTestSimulator();
    void addRoadToMap(const Road &r);
    void addRoadNetToMap(std::vector<Road> &roadNet);
};

#endif // SIMULATOR_H
