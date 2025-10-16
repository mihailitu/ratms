#ifndef TESTINTERSECTION_H
#define TESTINTERSECTION_H

#include "../core/road.h"
#include "../core/vehicle.h"

#include <vector>

namespace simulator
{

std::vector<Road> singleLaneIntersectionTest();

std::vector<Road> testIntersectionTest();

std::vector<Road> fourWayIntersectionTest();

} //namespace simulator


#endif // TESTINTERSECTION_H
