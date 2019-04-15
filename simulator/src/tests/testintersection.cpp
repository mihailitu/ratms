#include "testintersection.h"

namespace simulator {

std::vector<Road> testIntersectionTest()
{
    std::vector<Road> cmap;
    Road r0(0, 1500, 3, 16.7);
    cmap.push_back(r0);
//    Road r1();
//    Road r2();
//    Road r3();
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
