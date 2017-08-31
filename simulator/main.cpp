#include <iostream>

#include "simulator.h"
#include "testmap.h"

using namespace simulator;

int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = getSigleVehicleTestMap(); // getFollowingVehicleTestMap();
    simulator.addRoadNetToMap( roadMap );

    simulator.runTestSimulator();

    return 0;
}
