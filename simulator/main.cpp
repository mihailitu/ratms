#include <iostream>

#include "simulator.h"
#include "testmap.h"


int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = getTestMap();
    simulator.addRoadNetToMap( roadMap );

    simulator.runSimulator();

    return 0;
}
