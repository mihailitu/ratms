#include <iostream>

#include "simulator.h"

/*
 * Construct a simple road network:
 * r1 -> r2
 * r2 -> r3, r7
 *
 *      *--r2-->*--r3-->*
 *      ^       ^       |
 *      |       |       |
 *      r1      r7      r4
 *      |       r8      |
 *      |       \/     \/
 *      *<--r6--*<--r5--*
 *
 *
 */

int main( )
{
    Simulator simulator;

    Road r1(1, 1500, 1, 50);
    Road r2(2,  750, 1, 50);
    Road r3(3,  750, 1, 50);
    Road r4(4, 1500, 1, 50);
    Road r5(5,  750, 1, 50);
    Road r6(6,  750, 1, 50);
    Road r7(7, 1500, 1, 70);
    Road r8(8, 1500, 1, 70);

    r1.addConnection( r2 );
    r2.addConnections( { r3, r8 } );
    r3.addConnection( r4 );
    r4.addConnection( r5 );
    r5.addConnections( { r6, r7 } );
    r6.addConnection( r1 );
    r7.addConnection( r3);
    r8.addConnection(r6 );

    simulator.addRoadToMap(r1);
    simulator.addRoadToMap(r2);
    simulator.addRoadToMap(r3);
    simulator.addRoadToMap(r4);
    simulator.addRoadToMap(r5);
    simulator.addRoadToMap(r6);
    simulator.addRoadToMap(r7);
    simulator.addRoadToMap(r8);

    simulator.runSimulator();

    return 0;
}
