#include "simulator.h"
#include "testmap.h"

#include <iostream>

using namespace simulator;

int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = laneChangeTest();

    simulator.addRoadNetToMap( roadMap );

    simulator.runTestSimulator();

    return 0;
}
