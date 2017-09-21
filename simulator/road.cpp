#include "road.h"
#include "logger.h"

#include <algorithm>

namespace simulator
{

const Vehicle Road::noVehicle(0.0, 0.0, 0.0);

Road::Road()
{
}

Road::Road( roadID id, unsigned length, roadPos startPos, roadPos endpos ) :
    id (id), length(length), startPos(startPos), endPos(endpos),
    usageProb(0.5)
{
    log_info("New road added: \n"
             "\t ID: %u \n"
             "\t length: %d m \n"
             "\t startPos: %f %f \n"
             "\t endPos:   %f %f \n"
             ,
             id, length, startPos.first, startPos.second, endPos.first, endPos.second);
}

Road::Road( roadID id, unsigned length, unsigned lanes, unsigned maxSpeed_mps ) :
    id (id), length(length), usageProb(0.5), lanesNo(lanes), maxSpeed(maxSpeed_mps)
{
    log_info("New road added: \n"
             "\t ID: %u \n"
             "\t length: %d m\n"
             "\t lanes: %d \n"
             "\t max_speed: %d km/h \n",
             id, length, lanesNo, maxSpeed);
    for(unsigned i = 1; i < lanesNo; ++i)
        // vehicles.push_back(std::vector<Vehicle>());
        vehicles.push_back(std::list<Vehicle>());
}

//TODO: should vehicles be added from outside Road class or
// a road should maintain it's vehicle pool internally based on statistics?
void Road::addVehicle(Vehicle car, unsigned lane)
{
    if(lane >= lanesNo) {
        log_warning("Assigned vehicle to road %u on lane %d, where the road has only %d lanes.", id, lane, lanesNo);
        lane = 0; //TODO: throw exception?
    }
    vehicles[lane].push_back(car);
}

//void Road::addConnection(roadID connection)
//{
//    connections.push_back(connection);
//}

//void Road::addConnection(Road connection)
//{
//    connections.push_back(connection.getId());
//}

//void Road::addConnections(std::vector<roadID> rconnections)
//{
//    for( roadID road : rconnections )
//        connections.push_back(road);
//}

//void Road::addConnections(std::vector<Road> rconnections)
//{
//    for( Road r : rconnections )
//        connections.push_back(r.getId());
//}

roadID Road::getId() const
{
    return id;
}

unsigned Road::getMaxSpeed() const
{
    return maxSpeed;
}

unsigned Road::getLanesNo() const
{
    return lanesNo;
}

unsigned Road::getLength() const
{
    return length;
}

const std::vector<std::list<Vehicle>>& Road::getVehicles() const
{
    return vehicles;
}

/* Vehicles are sorted on descenting order: first vehicle is closest to the end of the road - highest xPos
 * Vehicles on front need to be updated first. */
void Road::indexRoad()
{
    for(std::list<Vehicle> &lane : vehicles)
        lane.sort([](const auto &lhs, const auto &rhs)
        { return lhs.getPos() > rhs.getPos(); });

    //    for(auto &lane : vehicles)
    //        std::sort(lane.begin(), lane.end(),
    //                  [](const auto &lhs, const auto &rhs)
    //        {return lhs.getPos() > rhs.getPos();});
}

/* get the next possible leading vehicle from lane */
std::list<Vehicle>::iterator nextLaneLeaderCandidate(const Vehicle &current, std::list<Vehicle> &nextLane)
{
    std::list<Vehicle>::iterator leadingV = std::upper_bound(nextLane.begin(), nextLane.end(), current,
                                     [](const auto &lhs, const auto &rhs)
    {return lhs.getPos() < rhs.getPos();});

    return leadingV;
//    return ((leadingV != lane.end()) && (!(*leadingV).isTrafficLight()) ?
//                std::distance(lane.begin(), leadingV) : -1);
}

/*
 *
 */
void Road::changeLane(unsigned laneIndex, std::list<Vehicle>::iterator &it)
{
    Vehicle &current = *(it);

    if (lanesNo == 1)
        return;

    // consider lane changing only when decelerating
    if ((current.getAcceleration() >= 0 )
            || !current.laneChangeScheduled())
        return;

    if (current.isTrafficLight()) // traffic light
        return;

    // prefer overtaking on the left
    int nextLanesIdxs[2] = { laneIndex + 1 < lanesNo ? (int)laneIndex + 1 : -1,
                             (int)laneIndex - 1  >= 0     ? (int)laneIndex - 1 : -1 };

        for(int nextLaneIdx : nextLanesIdxs ) {
            if ( nextLaneIdx < 0 )
                continue;

            std::list<Vehicle> nextLane = vehicles[nextLaneIdx];

            auto nextLeaderIt = std::upper_bound(nextLane.begin(), nextLane.end(), current,
                                                 [](const auto &lhs, const auto &rhs)
                                                 {return lhs.getPos() < rhs.getPos();});

            Vehicle &nextLeader = (nextLeaderIt == nextLane.end() ? noVehicle : *nextLeaderIt);

            if (nextLeaderIt == nextLane.end())
                return;

//            int nextFollower = nextLeader + 1;
//            if (nextFollower == 0 || nextFollower >= vehicles[nextLane].size())
//                nextFollower = -1;

//            if (nextLeader < 0 && nextFollower < 0) {
//                log_info("Change lane: lane: %d vIndex: %d nextLane: %d\n"
//                         "             cv speed: %f cf acc: %f cv pos: %f\n"
//                         "             cl speed: %f cl acc: %f cl pos: %f", laneIndex, vehicleIndex, nextLane,
//                         processedVehicle.getVelocity(), processedVehicle.getAcceleration(), processedVehicle.getPos(),
//                         vehicles[laneIndex][vehicleIndex - 1].getVelocity(),
//                        vehicles[laneIndex][vehicleIndex - 1].getAcceleration(),
//                        vehicles[laneIndex][vehicleIndex - 1].getPos());

//                vehicles[nextLane].push_back(vehicles[laneIndex][vehicleIndex]);
//                vehicles[laneIndex].erase(vehicles[laneIndex].begin() + vehicleIndex);
//                indexRoad();
//                return;
//            }

//            if(nextLeader != -1) { // there's a possible leader on the next lane
//                // check for gap between
//            } else { // no leader on the next lane

//            }
//            // check for speeds - speed of prospected leading vehicle is larger than the one of the current leading vehicle?
//            //                  - can current leading vehicle be overtaken?
//            // will the next behind vehicle be incomodated by overtaking vehicle? will current vehicle provoque an anccident?
//            //
        }

}

//    Vehicle &processedVehicle = vehicles[laneIndex][vehicleIndex];

//    if (lanesNo == 1) // don't evaluate lane change for one lane road
//        return;

//    if (processedVehicle.isTrafficLight()) // traffic light
//        return;

//    // consider lane changing only when decelerating
//    if (!(processedVehicle.getAcceleration() < 0) || processedVehicle.laneChangeScheduled())
//        return;

//    // prefer overtaking on the left
//    int nextLanes[2] = {      laneIndex + 1 < lanesNo ? (int)laneIndex + 1 : -1,
//                              (int)laneIndex - 1  >= 0     ? (int)laneIndex - 1 : -1 };

//    for(int nextLane : nextLanes ) {
//        if ( nextLane < 0 )
//            continue;

//        int nextLeader = nextLeadingVehicleIndex(processedVehicle, vehicles[nextLane]);

//        int nextFollower = nextLeader + 1;
//        if (nextFollower == 0 || nextFollower >= vehicles[nextLane].size())
//            nextFollower = -1;

//        if (nextLeader < 0 && nextFollower < 0) {
//            log_info("Change lane: lane: %d vIndex: %d nextLane: %d\n"
//                     "             cv speed: %f cf acc: %f cv pos: %f\n"
//                     "             cl speed: %f cl acc: %f cl pos: %f", laneIndex, vehicleIndex, nextLane,
//                     processedVehicle.getVelocity(), processedVehicle.getAcceleration(), processedVehicle.getPos(),
//                     vehicles[laneIndex][vehicleIndex - 1].getVelocity(),
//                    vehicles[laneIndex][vehicleIndex - 1].getAcceleration(),
//                    vehicles[laneIndex][vehicleIndex - 1].getPos());

//            vehicles[nextLane].push_back(vehicles[laneIndex][vehicleIndex]);
//            vehicles[laneIndex].erase(vehicles[laneIndex].begin() + vehicleIndex);
//            indexRoad();
//            return;
//        }

//        if(nextLeader != -1) { // there's a possible leader on the next lane
//            // check for gap between
//        } else { // no leader on the next lane

//        }
//        // check for speeds - speed of prospected leading vehicle is larger than the one of the current leading vehicle?
//        //                  - can current leading vehicle be overtaken?
//        // will the next behind vehicle be incomodated by overtaking vehicle? will current vehicle provoque an anccident?
//        //
//    }

//}

void Road::update(double dt)
{
    indexRoad();

    // TODO: first vehicle should always be a traffic light
    unsigned laneIndex = 0;
    for(auto &lane : vehicles) {
        for(auto it = lane.begin(); it != lane.end(); ++it) {
            Vehicle &current = (*it);
            if(current.isTrafficLight())
                current.update(dt, noVehicle);
            else {
                changeLane(laneIndex, it);
                Vehicle &currentLeader = *(std::prev(it));
                current.update(dt, currentLeader);
            }
        }
        ++laneIndex;
    }

    //        for(unsigned i = 0; i < lane.size(); ++i) {// see indexRoad() comments for clarification
    //            if (i == 0 ) // first vehicle (highest xPos) has no leading vehicle
    //                lane[i].update(dt, noVehicle);
    //            else {
    //                changeLane(laneIndex, i);
    //                lane[i].update(dt, lane[i-1]);
    //            }
    //            ++laneIndex;
    //        }
    //    }
}

void Road::printRoad() const
{
    //    std::string connections_str = "";
    //    for( unsigned i = 0; i < connections.size(); ++i ) {
    //        connections_str += std::to_string(connections[i]);
    //        if ( connections.size() != i+1 )
    //            connections_str += ", ";
    //    }

    log_info("Road ID:    %u\n"
             "Length:       %d\n"
             "Lanes:        %d\n"
             "Max speed:    %d\n"
             "Usage:        %.2f\n"
             "Vehicle No.:  %d\n"
             "Start:        (%f, %f)\n"
             "End:          (%f, %f)\n"
             "Connections:  %s\n",
             id, length, lanesNo, maxSpeed, usageProb, vehicles.size(),
             startPos.first, startPos.second, endPos.first, endPos.second);//, connections_str.c_str());

    for(auto lane : vehicles )
        for(auto v : lane)
            v.printVehicle();
}

} // namespace simulator
