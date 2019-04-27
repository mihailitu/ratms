#include "simulator.h"
#include "tests/testmap.h"
#include "tests/testintersection.h"
#include "logger.h"
#include "config.h"

#include <iostream>
#include <chrono>

using namespace simulator;

int main( )
{
    Simulator simulator;

    std::vector<Road> roadMap = performanceTest(500, 500); //singleLaneIntersectionTest();
    // setDummyMapSize(500, 500, roadMap);

    simulator.addRoadNetToMap( roadMap );
    log_info("Start running simulation");

    Config::outputSimulationToDisk = true;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    simulator.runTestSimulator();
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
    log_info("Simulation done in: %f seconds", (float)duration / 1000);
    return 0;
}
