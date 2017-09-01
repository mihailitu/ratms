#include <iostream>

#include "simulator.h"
#include "logger.h"
#include "road.h"

namespace simulator
{

Simulator::Simulator()
{
    initSimulatorTestState();
}

void Simulator::initSimulatorTestState()
{
}

void Simulator::addRoadToMap(const Road &r)
{
    cityMap[r.getId()] = r;
}

void Simulator::addRoadNetToMap(std::vector<Road> &roadNet)
{
    for( Road r : roadNet )
        cityMap[r.getId()] = r;
}

void Simulator::runTestSimulator()
{
    double dt = 0.5;
    int iter = 0;
    while (!terminate && iter < 10) {
        ++iter;
        for( auto &mapEl : cityMap ) {
            mapEl.second.update(dt);
        }
        runTime += dt;

        serialize(runTime);

    }
}

void Simulator::serialize(double time)
{
    /* let other services know this road's layout (version 1, compatible with simple_road.py test:
     * the function will output a line composed of:
     *                |          | vehicle 0  | vehicle 1 | ...... | vehicle n |
     * time | roadID0 | maxSpeed |  x | v | a | x | v | a | .......| x | v | a |
     * time | roadID1 | maxSpeed |  x | v | a | x | v | a | .......| x | v | a |
     */
}

void Simulator::runSimulator()
{
    log_info("Running the simulator");
}

} // namespace simulator
