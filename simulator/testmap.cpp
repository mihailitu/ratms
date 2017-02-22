#include "testmap.h"

/*
 * Construct a simple road network for dev and testing purposes:
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

std::vector<Road> getTestMap()
{
    std::vector<Road> cmap = {
    Road(0, 1500, 1, 50),
    Road(1,  750, 1, 50),
    Road(2,  750, 1, 50),
    Road(3, 1500, 1, 50),
    Road(4,  750, 1, 50),
    Road(5,  750, 1, 50),
    Road(6, 1500, 1, 70),
    Road(7, 1500, 1, 70)
    };

    cmap[0].addConnection( cmap[1] );
    cmap[1].addConnections({ cmap[2], cmap[7] });
    cmap[2].addConnection( cmap[3] );
    cmap[3].addConnection( cmap[4] );
    cmap[4].addConnections({ cmap[5], cmap[6] });
    cmap[5].addConnection( cmap[0] );
    cmap[6].addConnection( cmap[2]);
    cmap[7].addConnection( cmap[5] );

    /* Add some hardcoded cars on the road, so we test equations */



    return cmap;
}

/*
 * Add a simple road to test the basic equations on free and busy road.
 */
std::vector<Road> getSimpleTestMap()
{
    std::vector<Road> smap = {
        Road(0, 2000, 1, 70)
    };

    // add one vehicle at the beginning of the road for free road tests
    int vLength = 4; // medium
    int vPos = 10;

    Vehicle v(vPos, vLength);
    v.freeRoadOn();
    smap[0].addVehicle(v);

    return smap;
}
