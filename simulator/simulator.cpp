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
    for( auto mapEl : cityMap ) {
        mapEl.second.printRoad();
        auto vehicles = mapEl.second.getVehicles();
        for( auto vehicle : vehicles ) {
            vehicle.printVehicle();
        }
    }
}

void Simulator::runSimulator()
{
    log_info("Running the simulator");
}
