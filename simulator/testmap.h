#ifndef TESTMAP_H
#define TESTMAP_H

#include "road.h"
#include "vehicle.h"

#include "vector"

namespace simulator
{

std::vector<Road> getSigleVehicleTestMap();
std::vector<Road> getFollowingVehicleTestMap();
std::vector<Road> getTestMap();

} // namespace simulator
#endif // TESTMAP_H
