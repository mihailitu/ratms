#include <iostream>

#include "simulator.h"
int main( )
{
    Simulator simulator;

    Road r1(1, 1000, 2, 50 );
    Road r2(2, 750, 1, 50 );
    Road r3(3, 1450, 3, 50 );

    r1.addConnection(r2.getId());
    r1.addConnection(r3.getId());

    simulator.addRoadToMap(r1);
    simulator.addRoadToMap(r2);
    simulator.addRoadToMap(r3);

    simulator.runSimulator();

    return 0;
}
