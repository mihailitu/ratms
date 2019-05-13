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

    std::vector<Road> roadMap = performanceTest(4000, 100); //threeRoadIntersectionTest(); //singleLaneIntersectionTest(); // performanceTest(500, 500);
    // setDummyMapSize(500, 500, roadMap);

    ::log_level = LogLevels::info;

    simulator.addRoadNetToMap( roadMap );
    log_info("Start running simulation");

    Config::outputSimulationToDisk = false;

    auto start = std::chrono::system_clock::now();
    simulator.runTestSimulator();
    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end-start;
    log_info("Simulation done in: %f seconds", elapsed_seconds.count());
    return 0;
}
