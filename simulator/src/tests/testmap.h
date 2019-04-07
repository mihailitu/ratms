#ifndef TESTMAP_H
#define TESTMAP_H

#include "../road.h"
#include "../vehicle.h"

#include <vector>

namespace simulator
{
void setDummyMapSize(unsigned x, unsigned y, std::vector<Road> &map);
std::vector<Road> getTestMap();

std::vector<Road> getSmallerTestMap();

std::vector<Road> semaphoreTest();
std::vector<Road> manyRandomVehicleTestMap(int numVehicles);

std::vector<Road> laneChangeTest();

std::vector<Road> twoLanesTestMap();

std::vector<Road> sigleVehicleTestMap();
std::vector<Road> followingVehicleTestMap();
std::vector<Road> testMap();

} // namespace simulator
#endif // TESTMAP_H
