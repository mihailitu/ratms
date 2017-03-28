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
    void initSimulatorTestState();

    bool terminate = {false};

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

#endif // SIMULATOR_H
