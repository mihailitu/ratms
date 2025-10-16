#include "testintersection.h"
#include "testmap.h"

namespace simulator {

/*
 *
 *
 */

std::vector<Road> testIntersectionTest()
{
    unsigned rId = 0;
    std::vector<Road> cmap;
    Road r0(rId++, 1500, 3, 16.7);
    r0.setCardinalCoordinates({10, 1100}, {1510, 1100});
    cmap.push_back(r0);

    Road r00(rId++, 1500, 3, 16.7);
    r00.setCardinalCoordinates({1510, 1000}, {10, 1000});
    cmap.push_back(r00);

    Road r1(rId++, 1500, 3, 16.7);
    r1.setCardinalCoordinates({1610, 1100}, {1610 + 1500, 1100});
    cmap.push_back(r1);

    Road r11(rId++, 1500, 3, 16.7);
    r11.setCardinalCoordinates({1610 + 1500, 1000}, {1610, 1000});
    cmap.push_back(r11);

    Road r2(rId++, 1000, 2, 16.7);
    r2.setCardinalCoordinates({1500, 0}, {1500,1000});
    cmap.push_back(r2);

//    Road r3(2, 1000, 2, 16.7);
//    r3.setCardinalCoordinates({1500, 1100}, {1500,2100});
//    cmap.push_back(r3);



    return cmap;
}

std::vector<Road> singleLaneIntersectionTest()
{
    std::vector<Road> cmap;

    Road r0(0, 500, 1, 20);  // Shorter road for faster transition
    r0.setCardinalCoordinates({0, 100}, {500, 100});
    // Start vehicles closer to end so they transition while light is still green
    r0.addVehicle(Vehicle(350.0, 5.0, 20.0), 0);  // Will reach end in ~7.5 seconds
    r0.addVehicle(Vehicle(380.0, 5.0, 15.0), 0);  // Will reach end in ~8 seconds

    Road r1(1, 1500, 1, 20);
    r1.setCardinalCoordinates({500, 100}, {2000, 100});

    // Connect road 0 lane 0 to road 1 with 100% probability
    r0.addLaneConnection(0, r1.getId(), 1.0);

    cmap.push_back(r0);
    cmap.push_back(r1);

    setDummyMapSize(2500, 500, cmap);

    return cmap;
}

/*
 * Four-way intersection test with probabilistic routing
 *
 * Layout:
 *                  Road 1
 *                    ↓
 *      Road 0  →  [CENTER]  →  Road 2
 *                    ↓
 *                  Road 3
 *
 * - Road 0 (West approach): vehicles can go straight (Road 2) or turn right (Road 3)
 * - Road 1 (North approach): vehicles go straight (Road 3)
 */
std::vector<Road> fourWayIntersectionTest()
{
    std::vector<Road> cmap;

    // Road 0: West approach (0, 1000) → (900, 1000)
    Road r0(0, 400, 1, 20);
    r0.setCardinalCoordinates({0, 1000}, {400, 1000});
    // Add vehicles starting close to end so they reach intersection quickly
    r0.addVehicle(Vehicle(250.0, 5.0, 20.0), 0);
    r0.addVehicle(Vehicle(280.0, 5.0, 18.0), 0);
    r0.addVehicle(Vehicle(310.0, 5.0, 15.0), 0);

    // Road 1: North approach (1000, 0) → (1000, 900)
    Road r1(1, 400, 1, 20);
    r1.setCardinalCoordinates({1000, 0}, {1000, 400});
    r1.addVehicle(Vehicle(250.0, 5.0, 20.0), 0);
    r1.addVehicle(Vehicle(280.0, 5.0, 16.0), 0);

    // Road 2: East exit (1100, 1000) → (2000, 1000)
    Road r2(2, 1000, 1, 20);
    r2.setCardinalCoordinates({1100, 1000}, {2100, 1000});

    // Road 3: South exit (1000, 1100) → (1000, 2000)
    Road r3(3, 1000, 1, 20);
    r3.setCardinalCoordinates({1000, 1100}, {1000, 2100});

    // Set up connections with probabilities
    // Road 0 can go to Road 2 (70% - straight east) or Road 3 (30% - right turn south)
    r0.addLaneConnection(0, r2.getId(), 0.7);
    r0.addLaneConnection(0, r3.getId(), 0.3);

    // Road 1 goes to Road 3 (100% - straight south)
    r1.addLaneConnection(0, r3.getId(), 1.0);

    cmap.push_back(r0);
    cmap.push_back(r1);
    cmap.push_back(r2);
    cmap.push_back(r3);

    setDummyMapSize(2500, 2500, cmap);

    return cmap;
}

} // namespace simulator
