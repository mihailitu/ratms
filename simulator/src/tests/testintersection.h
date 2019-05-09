#ifndef TESTINTERSECTION_H
#define TESTINTERSECTION_H

#include "../road.h"
#include "../vehicle.h"

#include <vector>

namespace simulator
{

std::vector<Road> threeRoadIntersectionTest();
std::vector<Road> singleLaneIntersectionTest();

std::vector<Road> intersectionTest();

} //namespace simulator


#endif // TESTINTERSECTION_H
