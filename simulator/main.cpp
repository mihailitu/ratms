#include <iostream>

#include "simulator.h"
#include "testmap.h"

using namespace simulator;

int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = getFollowingVehicleTestMap(); // getSigleVehicleTestMap();
    simulator.addRoadNetToMap( roadMap );

    simulator.runTestSimulator();

    return 0;
}
