#include "simulator.h"
#include "logger.h"
#include "road.h"
#include "config.h"

#include <iostream>
#include <fstream>
#include <iomanip>

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

    std::ofstream output(Config::simulatorOuput);

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
 *                 |                     | vehicle 0         | vehicle 1 | ...... | vehicle n     | vehicle n+1 |
 * time0 | roadID0 | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time0 | roadID1 | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time1 | roadID0 | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time1 | roadID1 | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
 *
 * !!! Roads have different number of vehicles.
 * Same road can also have different number of vehicles at different times
 */

void Simulator::serialize_v1(double time, std::ostream &output)
{
    std::showpoint(output);
    for(auto &roadElement : cityMap) {
        Road& road = roadElement.second;
        output << time << " " << road.getId() << " " << road.getLength() << " " << road.getMaxSpeed() << " " << road.getLanesNo() << " ";
        unsigned vLane = 0;
        for(auto &lane : road.getVehicles()) {
            for(auto &vehicle : lane) {
                vehicle.serialize(output);
                output << vLane << " "; // until decided how to let a vehicle know on whic lane is, simply output it.
            }
            ++vLane;
        }
    }
    output << "\n";
}

void Simulator::runSimulator()
{
    log_info("Running the simulator");
}

} // namespace simulator
