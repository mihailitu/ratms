#include <iostream>
#include <fstream>

#include "simulator.h"
#include "logger.h"
#include "road.h"

namespace simulator
{

Simulator::Simulator()
{
    initSimulatorTestState();
}

void Simulator::initSimulatorTestState()
{
}

void Simulator::addRoadToMap(const Road &r)
{
    cityMap[r.getId()] = r;
}

void Simulator::addRoadNetToMap(std::vector<Road> &roadNet)
{
    for( Road r : roadNet )
        cityMap[r.getId()] = r;
}

void Simulator::runTestSimulator()
{
    double dt = 0.5;
    int iter = 0;
    while (!terminate && iter < 10) {
        ++iter;
        for( auto &mapEl : cityMap ) {
            mapEl.second.update(dt);
        }
        runTime += dt;

        serialize_v1(runTime);
    }
}

void Simulator::serialize(double time)
{
    serialize_v1(time);
}

/* let other services know this road's layout (version 1, compatible with simple_road.py test:
 * the function will output a line composed of:
 *                 |          | vehicle 0  | vehicle 1 | ...... | vehicle n | vehicle n+1 |
 * time0 | roadID0 | maxSpeed |  x | v | a | x | v | a | .......|
 * time0 | roadID1 | maxSpeed |  x | v | a | x | v | a | .......| x | v | a |
 * time1 | roadID0 | maxSpeed |  x | v | a | x | v | a | .......| x | v | a |
 * time1 | roadID1 | maxSpeed |  x | v | a | x | v | a | .......| x | v | a |
 *
 * !!! Roads have different number of vehicles.
 * Same road can also have different number of vehicles at different times
 */
void Simulator::serialize_v1(double time)
{
    std::ofstream output("xx.dat"); // todo: add file name v1 to config file
    for(auto &roadElement : cityMap) {
        Road& road = roadElement.second;
        output << time << " " << road.getId() << " " << road.maxSpeed;
        for(auto &lane : road.getVehicles())
            for(Vehicle &vehicle : lane)
                vehicle.serialize(output);
    }
    output.close();
}

void Simulator::runSimulator()
{
    log_info("Running the simulator");
}

} // namespace simulator
