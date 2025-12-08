#include "core/simulator.h"
#include "tests/testintersection.h"
#include "tests/testmap.h"
#include "utils/logger.h"

#include <chrono>
#include <iostream>

using namespace simulator;
using namespace ratms;

int main() {
  Simulator simulator;
  ratms::Logger::init();

  std::vector<Road> roadMap =
      fourWayIntersectionTest(); // Test 4-way intersection with routing
  // singleLaneIntersectionTest(); // Test road transitions
  // testIntersectionTest();// laneChangeTest(); //getTestMap();
  // //getSmallerTestMap(); semaphoreTest();//
  // manyRandomVehicleTestMap(30);//laneChangeTest();

  // setDummyMapSize(500, 500, roadMap);

  simulator.addRoadNetToMap(roadMap);
  LOG_INFO(LogComponent::Simulation, "Starting simulation with {} roads", roadMap.size());

  std::chrono::high_resolution_clock::time_point t1 =
      std::chrono::high_resolution_clock::now();
  simulator.runTestSimulator();
  std::chrono::high_resolution_clock::time_point t2 =
      std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  LOG_INFO(LogComponent::Simulation, "Simulation completed in {:.2f} seconds", static_cast<float>(duration) / 1000.0f);
  ratms::Logger::shutdown();
  return 0;
}
