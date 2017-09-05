#include <iostream>
#include <fstream>
#include <iomanip>

#include "simulator.h"
#include "logger.h"
#include "road.h"
#include "config.h"

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
    double dt = Config::DT;
    int iter = 0;

    std::ofstream output(Config::singleVehicleTestFName); // todo: add file name v1 to config file

    while (!terminate && iter < Config::simulationTime) {
        ++iter;
        for( auto &mapEl : cityMap ) {
            mapEl.second.update(dt);
        }
        runTime += dt;

        serialize_v1(runTime, output);
    }
    output.close();
}

void Simulator::serialize(double time, std::ostream &output)
{
    serialize_v1(time, output);
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
void Simulator::serialize_v1(double time, std::ostream &output)
{
    std::showpoint(output);
    for(auto &roadElement : cityMap) {
        Road& road = roadElement.second;
        output << time << " " << road.getId() << " " << road.getLength() << " " << road.getMaxSpeed() << " ";
        for(auto &lane : road.getVehicles())
            for(auto &vehicle : lane)
                vehicle.serialize(output);
    }
    output << "\n";
}

void Simulator::runSimulator()
{
    log_info("Running the simulator");
}

} // namespace simulator
