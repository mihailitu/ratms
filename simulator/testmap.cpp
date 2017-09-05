#include "testmap.h"

namespace simulator
{

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
 * Add a simple road to test the basic equations on free road.
 * Road:
 *      - length: 2000 m
 *      - max speed: 20 meters/second (app. 70 km/h)
 */
std::vector<Road> getSigleVehicleTestMap()
{
    // add one vehicle at the beginning of the road for free road tests
    double vLength = 5.0; // medium sedane
    double vPos = 0.0;

    Vehicle v(vPos, vLength, 20.0);
    v.freeRoadOn();

    Road r(0, 2000, 1, 20);
    r.addVehicle(v, 0);
    std::vector<Road> smap = {
        r
    };
    return smap;
}

/*
 * Add a simple road to test the basic equations on free and busy road.
 */
std::vector<Road> getFollowingVehicleTestMap()
{
    Vehicle v(0.0, 5.0, 20.0);

    Vehicle v1(150.0, 5.0, 15.0);

    Road r(0, 2000, 1, 20);
    r.addVehicle(v1, 0);
    r.addVehicle(v, 0);
    std::vector<Road> smap = {
        r
    };
    return smap;
}

} // namespace simulator
