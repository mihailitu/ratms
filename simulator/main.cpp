#include <iostream>

#include <typeinfo>

#include "simulator.h"
#include "testmap.h"


int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = getSimpleTestMap();
    simulator.addRoadNetToMap( roadMap );

    std::cout << typeid(simulator).name() << std::endl;

    simulator.runSimulator();

    return 0;
}
