#include "simulator.h"
#include "logger.h"
#include "road.h"
#include "config.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>

namespace simulator
{

Simulator::Simulator()
{
    initSimulatorTestState();
}

void Simulator::initSimulatorTestState()
{
    Config::outputSimulationToDisk = true;
}

void Simulator::addRoadToMap(Road &r)
{
    cityMap[r.getId()] = r;
}

void Simulator::addRoadNetToMap(std::vector<Road> &roadNet)
{
    for( Road r : roadNet ) {
        cityMap[r.getId()] = r;
    }
}


void log_map(const Simulator::CityMap &cityMap, double dt)
{
    for(auto &roadElement : cityMap) {
        const Road& road = roadElement.second;
        fprintf(stdout, "Delta: %4.2f RoadID: %3lu\n", dt, road.getId());

        unsigned vLane = 0;
        std::vector<Vehicle> vectorRoad;
        auto trafficLights = road.getCurrentLightConfig();
        for(auto &lane : road.getVehicles()) {
            std::string vehiclesStr = "";
            vectorRoad.insert(vectorRoad.end(), lane.begin(), lane.end());
            fprintf(stdout, "\tLane: %d\t { ", vLane);
            for(auto &vehicle : lane) {
                fprintf(stdout, "%3d, ", vehicle.getId());
            }
            fprintf(stdout, "} { %c }\n", trafficLights[vLane]);
            ++vLane;
        }
        std::sort(vectorRoad.begin(), vectorRoad.end(),[](const Vehicle &v1, const Vehicle &v2){return v1.getId() < v2.getId();});
        for(auto &v : vectorRoad)
            fprintf(stdout, "\t\t\t{id: %3d d: %6.2f a: %2.2f v: %2.2f}\n", v.getId(), v.getPos(), v.getAcceleration(), v.getVelocity());
        fprintf(stdout, "\n\n");
    }
}

void Simulator::runTestSimulator()
{
    double dt = Config::DT;
    int iter = 0;

    std::ofstream roadMap;
    std::ofstream output;
    if (Config::outputSimulationToDisk) {
        roadMap.open(Config::simulatorMap);
        output.open(Config::simulatorOuput);
        serialize_roads_v2(roadMap);
    }

    while (!terminate && iter < Config::simulationTime) {

        // log_map(cityMap, runTime);

        ++iter;
        for( auto &mapEl : cityMap )
            mapEl.second.update(dt, cityMap);

        if (Config::outputSimulationToDisk)
            serialize(runTime, output);

        runTime += dt;
    }
    if (Config::outputSimulationToDisk)
        output.close();
}

void Simulator::serialize(double time, std::ostream &output) const
{
    serialize_v2(time, output);
}
/* Serialization V2:
 * - after the map is constructed, we will output the roads in a separate file
 * roads_v2.dat
 * roadID0 | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes_no |
 * roadID1 | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes_no |
 */

void Simulator::serialize_roads_v2(std::ostream &road_output) const
{
    std::showpoint(road_output);
    for(auto &roadElement : cityMap) {
        const Road& road = roadElement.second;
        road.serialize(road_output);
        road_output << "\n";
    }
}

/*
 * At each iteration, we will output the status of the traffic
 *                 | lanes / light ...|   vehicle 0    |   vehicle 1   | ...... |   vehicle n   |
 * time0 | roadID0 |   l   |  RYG  |  x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time0 | roadID1 |   l   |  RYG  |  x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time1 | roadID0 |   l   |  RYG  |  x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time1 | roadID1 |   l   |  RYG  |  x | v | a | l | x | v | a | l | .......| x | v | a | l |
 */
void Simulator::serialize_v2(double time, std::ostream &output) const
{
    std::showpoint(output);
    for(auto &roadElement : cityMap) {
        const Road& road = roadElement.second;
        output << time << " " << road.getId();

        output << " " << road.getLanesNo();
        std::vector<char> lights = road.getCurrentLightConfig();
        for(unsigned i = 0; i < lights.size(); ++i)
            output << " " << lights[i];

        unsigned vLane = 0;
        for(auto &lane : road.getVehicles()) {
            for(auto &vehicle : lane) {
                vehicle.serialize(output);
                output << " " << vLane; // until decided how to let a vehicle know on which lane is, simply output it.
            }
            ++vLane;
        }
        output << "\n";
    }
}

/* let other services know this road's layout (version 1, compatible with simple_road.py test:
 * the function will output a line composed of:
 *                                                | vehicle 0     |     vehicle 1 | ...... | vehicle n     |
 * time0 | roadID0 | length | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time0 | roadID1 | length | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time1 | roadID0 | length | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
 * time1 | roadID1 | length | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
 *
 * !!! Roads have different number of vehicles.
 *     Roads can also have different number of vehicles at different times
 */

void Simulator::serialize_v1(double time, std::ostream &output) const
{
    std::showpoint(output);
    for(auto &roadElement : cityMap) {
        const Road& road = roadElement.second;
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
