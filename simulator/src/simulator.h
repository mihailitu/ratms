#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "road.h"

#include <map>

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

    bool outputData = {false};

    // simulator run time
    double runTime = {0};

public:
    typedef std::map<roadID, Road> CityMap;
    CityMap cityMap;
public:
    Simulator();

    void runSimulator();
    void runTestSimulator();
    void addRoadToMap(Road &r);
    void addRoadNetToMap(std::vector<Road> &roadNet);

    // output the current layout of this map
    void serialize(double time, std::ostream &output) const;

private:
    /* there will probably several serialization versions, as the project develops
     * Keep all versions so we can run older python tests at later times
     * !! Final serialization version should be implemented using sockets
     */
    void serialize_v1(double time, std::ostream &output) const;
    void serialize_v2(double time, std::ostream &output) const;
    void serialize_roads_v2(std::ostream &road_output) const;
};

} // namespace simulator

#endif // SIMULATOR_H
