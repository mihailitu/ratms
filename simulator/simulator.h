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

    double v0;      // Desired velocity
    double T;       // Safe time headway
    double a;       // Maximum acceleration
    double b;       // Desired deceleration
    double delta;   // Acceleration exponent
    double s0;      // Minimum distance
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
