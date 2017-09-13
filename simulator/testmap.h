#ifndef TESTMAP_H
#define TESTMAP_H

#include "road.h"
#include "vehicle.h"

#include <vector>

namespace simulator
{
std::vector<Road> twoLanesTestMap();
std::vector<Road> sigleVehicleTestMap();
std::vector<Road> followingVehicleTestMap();
std::vector<Road> manyRandomVehicleTestMap();
std::vector<Road> testMap();

} // namespace simulator
#endif // TESTMAP_H
