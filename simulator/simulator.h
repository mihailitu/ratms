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
    float runTime = {0};

public:
    typedef std::map<Road::roadID, Road> CityMap;
    CityMap cityMap;
public:
    Simulator();

    void runSimulator();
    void runTestSimulator();
    void addRoadToMap(const Road &r);
    void addRoadNetToMap(std::vector<Road> &roadNet);
};

} // namespace simulator

#endif // SIMULATOR_H
