#ifndef TESTMAP_H
#define TESTMAP_H

#include <vector>

#include "road.h"
#include "vehicle.h"

namespace simulator
{

std::vector<Road> getSigleVehicleTestMap();
std::vector<Road> getFollowingVehicleTestMap();
std::vector<Road> getTestMap();

} // namespace simulator
#endif // TESTMAP_H
