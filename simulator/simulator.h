#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <map>

#include "road.h"

namespace simulator
{

class Simulator
{
    /***
     * IDM - Intelligent driver model
     * https://en.wikipedia.org/wiki/Intelligent_driver_model
     */
    void initSimulatorTestState();

    bool terminate = {false};

    // simulator run time
    double runTime = {0};

public:
    typedef std::map<roadID, Road> CityMap;
    CityMap cityMap;
public:
    Simulator();

    void runSimulator();
    void runTestSimulator();
    void addRoadToMap(const Road &r);
    void addRoadNetToMap(std::vector<Road> &roadNet);

    // output the current layout of this road
    void serialize(double time);
};

} // namespace simulator

#endif // SIMULATOR_H
