#include "simulator.h"
#include "tests/testmap.h"
#include "tests/testintersection.h"

#include <iostream>

using namespace simulator;

int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = getTestMap();
    // semaphoreTest();// manyRandomVehicleTestMap(30);//laneChangeTest();

    // setDummyMapSize(500, 500, roadMap);

    simulator.addRoadNetToMap( roadMap );

    simulator.runTestSimulator();

    return 0;
}
