#include "testmap.h"
#include "config.h"

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

//    cmap[0].addConnection( cmap[1] );
//    cmap[1].addConnections({ cmap[2], cmap[7] });
//    cmap[2].addConnection( cmap[3] );
//    cmap[3].addConnection( cmap[4] );
//    cmap[4].addConnections({ cmap[5], cmap[6] });
//    cmap[5].addConnection( cmap[0] );
//    cmap[6].addConnection( cmap[2]);
//    cmap[7].addConnection( cmap[5] );

    /* Add some hardcoded cars on the road, so we test equations */



    return cmap;
}

/*
 * Lane change test
 * Road:
 *      - length: 2000 m
 *      - max speed: 20 meters/second (app. 70 km/h)
 */
std::vector<Road> laneChangeTest()
{
    Config::simulatorOuput = Config::simpleRoadTestFName;

    Road r(0, 2000, 2, 20);

    Vehicle v(0.0, 5.0, 20.0);
    Vehicle v1(20.0, 5.0, 15);
    Vehicle v2(20.0, 5.0, 18.0);
    Vehicle v3(5.0, 5.0, 17.0);

    r.addVehicle(v, 0);
    r.addVehicle(v1, 0);
    r.addVehicle(v2, 1);
    r.addVehicle(v3, 1);
    std::vector<Road> smap = {
        r
    };
    return smap;
}

/*
 * Create a two lane test road.
 * Road:
 *      - length: 2000 m
 *      - max speed: 20 meters/second (app. 70 km/h)
 */
std::vector<Road> twoLanesTestMap()
{
    Config::simulatorOuput = Config::simpleRoadTestFName;
    Vehicle v(0.0, 5.0, 20.0);
    Vehicle v1(100.0, 5.0, 18);
    Vehicle v2(0.0, 5.0, 22);

    Road r(0, 2000, 2, 20); // two lanes
    r.addVehicle(v, 0);
    r.addVehicle(v1, 1);
    r.addVehicle(v2, 1);
    std::vector<Road> smap = {
        r
    };
    return smap;
}


/* TODO:
 * Add random number of vehicles with random positions and random speeds
 */
std::vector<Road> manyRandomVehicleTestMap()
{
    Config::simulatorOuput = Config::simpleRoadTestFName;
    Vehicle v(0.0, 5.0, 20.0);
    Vehicle v1(100.0, 5.0, 18);
    Vehicle v2(150.0, 5.0, 15.0);
    Vehicle v3(400, 5.0, 10);

    Road r(0, 2000, 1, 20);
    r.addVehicle(v3, 0);
    r.addVehicle(v2, 0);
    r.addVehicle(v1, 0);
    r.addVehicle(v, 0);
    std::vector<Road> smap = {
        r
    };
    return smap;
}

/*
 * Add a simple road to test the basic equations on free road.
 * Road:
 *      - length: 2000 m
 *      - max speed: 20 meters/second (app. 70 km/h)
 */
std::vector<Road> sigleVehicleTestMap()
{
    Config::simulatorOuput = Config::simpleRoadTestFName;
    // add one vehicle at the beginning of the road for free road tests
    double vLength = 5.0; // medium sedane
    double vPos = 0.0;

    Vehicle v(vPos, vLength, 20.0);

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
std::vector<Road> followingVehicleTestMap()
{
    Config::simulatorOuput = Config::simpleRoadTestFName;
    Vehicle v(0.0, 5.0, 20.0);
    Vehicle v1(100.0, 5.0, 18);
    Vehicle v2(150.0, 5.0, 15.0);
    Vehicle v3(400, 5.0, 10.0);
    Vehicle v4(1750, -1.0, 0.0);

    Road r(0, 2000, 1, 20);
    r.addVehicle(v4, 0);
    r.addVehicle(v3, 0);
    r.addVehicle(v2, 0);
    r.addVehicle(v1, 0);
    r.addVehicle(v, 0);
    std::vector<Road> smap = {
        r
    };
    return smap;
}

} // namespace simulator
