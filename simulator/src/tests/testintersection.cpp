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

//    Vehicle v;
//    Vehicle v1;
//    Vehicle v2(20.0, 5.0, 18.0);
//    Vehicle v3(5.0, 5.0, 17.0);


    Road r0(0, 1500, 1, 20);
    r0.addVehicle(Vehicle(0.0, 5.0, 20.0), 0);
    r0.addVehicle(Vehicle(30.0, 5.0, 15), 0);
    Road r1(1, 1500, 1, 20);

    r0.addLaneConnection(1, r1.getId(), 1.0);

    cmap.push_back(r0);
    cmap.push_back(r1);

    return cmap;
}

} // namespace simulator
