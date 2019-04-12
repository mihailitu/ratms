#include "testmap.h"
#include "../config.h"
#include "../logger.h"

#include <random>
namespace simulator
{

void setDummyMapSize(unsigned x, unsigned y, std::vector<Road> &map) {
    Road r(0xffff, x, 0, 0);
    r.setCardinalCoordinates({0, y}, {x, y});
    map.push_back(r);
}

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
    Road r(0, 500, 2, 20);
    r.setCardinalCoordinates({0, 100}, {500, 100});

    r.addVehicle({1.0, 5.0, 20.0}, 0);
    r.addVehicle({10.0, 5.0, 17.0}, 0);
    r.addVehicle({20.0, 5.0, 15.0}, 0);
    r.addVehicle({30.0, 5.0, 13.0}, 0);
//    r.addVehicle({1.0, 5.0, 20.0}, 0);
//    r.addVehicle({10.0, 5.0, 10.0}, 0);
    std::vector<Road> cmap = {
        r
    };

    setDummyMapSize(1000, 500, cmap);
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

std::vector<Road> getSmallerTestMap()
{
    std::vector<Road> cmap;
    for(unsigned i = 0; i < 100; ++i) {
        Road r(0, 200, 2, 50);
        r.setCardinalCoordinates({0, 100}, {200, 100});

        cmap.push_back(r);

    }
    setDummyMapSize(250, 500, cmap);
    return cmap;
}

std::vector<Road> getTimeTestMap()
{
    std::vector<Road> cmap;
    for(unsigned i = 0; i < 100; ++i){
        Road r(0, 3000, 1, 50);
        r.setCardinalCoordinates({0, i}, {3000, i});
        int pos = 0;
        for(unsigned v = 0; v < 200; ++v ) {
            r.addVehicle({static_cast<float>(pos + 5), 1.0, 20.0}, 0);
            pos+=5;
        }
        cmap.push_back(r);
    }

    setDummyMapSize(5000, 500, cmap);
    return cmap;
}

std::vector<Road> semaphoreTest() {
    Road r(0, 500, 2, 20);
    r.setCardinalCoordinates({0, 0}, {0, 500});

    Vehicle v(0.0, 5.0, 20.0);
    Vehicle v1(30.0, 5.0, 15);
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
 * Lane change test
 * Road:
 *      - length: 1500 m
 *      - max speed: 20 meters/second (app. 70 km/h)
 */
std::vector<Road> laneChangeTest()
{
    Road r1(0, 1500, 2, 20);
    r1.setCardinalCoordinates({0, 100}, {1500, 100});
    {
        Vehicle v(0.0, 5.0, 20.0);
        Vehicle v1(20.0, 5.0, 15);
        Vehicle v2(20.0, 5.0, 18.0);
        Vehicle v3(5.0, 5.0, 17.0);

        r1.addVehicle(v, 0);
        r1.addVehicle(v1, 0);
        r1.addVehicle(v2, 1);
        r1.addVehicle(v3, 1);
    }

    std::vector<Road> smap = {
        r1
    };

//    Road r2(1, 1400, 3, 20);
//    r2.setCardinalCoordinates({1500, 90}, {100, 90});
//    {
//        Vehicle v(0.0, 5.0, 20.0);
//        Vehicle v1(20.0, 5.0, 15);
//        Vehicle v2(20.0, 5.0, 18.0);
//        Vehicle v3(5.0, 5.0, 17.0);

//        r2.addVehicle(v, 0);
//        r2.addVehicle(v1, 0);
//        r2.addVehicle(v2, 1);
//        r2.addVehicle(v3, 1);
//    }

//    smap.push_back(r2);

    setDummyMapSize(2200, 500, smap);
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
std::vector<Road> manyRandomVehicleTestMap(int numVehicles)
{
    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_int_distribution<int> posRnd(1, 500);
    std::uniform_int_distribution<int> speedRnd(10, 15);
    std::uniform_int_distribution<int> laneRnd(0, 2);

    Road r(0, 2000, 3, 20);
    r.setCardinalCoordinates({10, 100}, {2010, 100});
    log_info("Random test - vehicles: %d", numVehicles);
    for(int i = 0; i < numVehicles; ++i) {

        int pos = posRnd(rng);
        int speed = speedRnd(rng);
        int lane = laneRnd(rng);
        Vehicle v(pos, 5.0, speed);
        v.log();
        r.addVehicle(v, lane);
    }

    std::vector<Road> smap = {
        r
    };

    setDummyMapSize(2000, 500, smap);

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
