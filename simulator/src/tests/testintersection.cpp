#include "testintersection.h"
#include "testmap.h"

namespace simulator {

/*
 *
 */

std::vector<Road> intersectionTest()
{
    unsigned rId = 0;

    // -> x
    Road r0(rId++, 1500, 3, 16.7);
    r0.setCardinalCoordinates({10, 1100}, {1510, 1100});
    r0.setTrafficLightSequence(0, 42, 3, 30);
    r0.setTrafficLightSequence(1, 42, 3, 30);
    r0.setTrafficLightSequence(2, 12, 3, 30);

    // <- x
    Road r00(rId++, 1500, 3, 16.7);
    r00.setCardinalCoordinates({1510, 1050}, {10, 1050});

    // x ->
    Road r1(rId++, 1500, 3, 16.7);
    r1.setCardinalCoordinates({1560, 1100}, {3060, 1100});

    // x <-
    Road r10(rId++, 1500, 3, 16.7);
    r10.setCardinalCoordinates({3060, 1050}, {1560, 1050});
    r10.setTrafficLightSequence(0, 45, 3, 30);
    r10.setTrafficLightSequence(1, 45, 3, 30);
    r10.setTrafficLightSequence(2, 12, 3, 30);

    //  |
    // \/
    // x
    Road r2(rId++, 1000, 2, 16.7);
    r2.setCardinalCoordinates({1525, 10}, {1525, 1010});

    //  ^
    //  |
    //  x
    Road r20(rId++, 1000, 2, 16.7);
    r20.setCardinalCoordinates({1550, 1010}, {1550, 10});

    //  x
    //  |
    // \/
    Road r3(rId++, 1000, 2, 16.7);
    r3.setCardinalCoordinates({1525, 1150}, {1525, 2150});

    //  x
    //  ^
    //  |
    Road r30(rId++, 1000, 2, 16.7);
    r30.setCardinalCoordinates({1550, 2150}, {1550, 1150});

    r0.addLaneConnection(0, {
                             {r3.getId(), 25.0},
                             {r1.getId(), 75.0}
                         });
    r0.addLaneConnection(1, r1.getId(), 40.0);
    r0.addLaneConnection(2, r20.getId(), 10.0);

    r0.addVehicle({0, 0, 0}, 0);

    r10.addLaneConnection(0, {
                              { r20.getId(), 40},
                              {r00.getId(), 60}
                          });
    r10.addLaneConnection(1, r00.getId(), 80);
    r10.addLaneConnection(2, r3.getId(), 20);


    r2.addLaneConnection(0, {
                             {r00.getId(), 50},
                             {r3.getId(), 50}
                         });

    r2.addLaneConnection(1, {
                             {r3.getId(), 80},
                             {r1.getId(), 20}
                         });


    r30.addLaneConnection(0, {
                              {r1.getId(), 30},
                              {r20.getId(), 70}
                          });
    r30.addLaneConnection(1, {
                              {r2.getId(), 70},
                              {r00.getId(), 30}
                          });


    std::vector<Road> cmap = {
        r0, r00, r1, r10, r2, r20, r3, r30,
    };


    setDummyMapSize(3300, 2300, cmap);

    return cmap;
}

std::vector<Road> singleLaneIntersectionTest()
{
    std::vector<Road> cmap;

    //    Vehicle v;
    //    Vehicle v1;
    //    Vehicle v2(20.0, 5.0, 18.0);
    //    Vehicle v3(5.0, 5.0, 17.0);


    Road r0(0, 500, 1, 20);
    r0.setTrafficLightSequence(0, 100, 0, 0);
    r0.setCardinalCoordinates( {0, 100}, {500, 100});
    r0.addVehicle(Vehicle(0.0, 5.0, 20.0), 0);
    r0.addVehicle(Vehicle(30.0, 5.0, 15), 0);
    Road r1(1, 750, 1, 20);
    r1.setCardinalCoordinates({510, 150}, {510 + 750, 150});

    r0.addLaneConnection(0, r1.getId(), 1.0);

    cmap.push_back(r0);
    cmap.push_back(r1);

    setDummyMapSize(2200, 300, cmap);

    return cmap;
}

} // namespace simulator
