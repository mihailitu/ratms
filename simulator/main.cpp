#include <iostream>

#include <typeinfo>

#include "simulator.h"
#include "testmap.h"


int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = getFollowingVehicleTestMap();// getSigleVehicleTestMap();
    simulator.addRoadNetToMap( roadMap );

    simulator.runTestSimulator();

    return 0;
}
