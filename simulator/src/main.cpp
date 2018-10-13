#include "simulator.h"
#include "tests/testmap.h"
#include "tests/testintersection.h"

#include <iostream>

using namespace simulator;

int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = singleLaneIntersectionTest(); // semaphoreTest();// manyRandomVehicleTestMap(30);//laneChangeTest();

    simulator.addRoadNetToMap( roadMap );

    simulator.runTestSimulator();

    return 0;
}
