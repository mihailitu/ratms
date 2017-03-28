#include "simulator.h"

#include <iostream>

#include "logger.h"
#include "road.h"

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
    double dt = 1.0;
    int iter = 0;
    while (!terminate && iter < 10) {
        ++iter;
        for( auto &mapEl : cityMap ) {
            mapEl.second.update(dt);
        }
    }
}

void Simulator::runSimulator()
{
    log_info("Running the simulator");
}
