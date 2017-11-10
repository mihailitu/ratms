#include "road.h"
#include "logger.h"

#include <algorithm>

namespace simulator
{

const Vehicle Road::noVehicle(0.0, 0.0, 0.0);
const double Road::minChangeLaneDist = 0.5;
const double Road::maxChangeLaneDist = 25.0;

Road::Road()
{
}

//Road::Road( roadID id, unsigned length, roadPos startPos, roadPos endpos ) :
//    id (id), length(length), startPos(startPos), endPos(endpos),
//    usageProb(0.5)
//{
//    log_info("New road added: \n"
//             "\t ID: %u \n"
//             "\t length: %d m \n"
//             "\t startPos: %f %f \n"
//             "\t endPos:   %f %f \n"
//             ,
//             id, length, startPos.first, startPos.second, endPos.first, endPos.second);
//}

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
        vehicles.push_back(std::vector<Vehicle>());
    // TODO: revert to linked list
    // vehicles.push_back(std::list<Vehicle>());
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

// TODO: revert to linked lists
// const std::vector<std::list<Vehicle>>& Road::getVehicles() const
const std::vector<std::vector<Vehicle>>& Road::getVehicles() const
{
    return vehicles;
}

/* Vehicles are sorted on descenting order: first vehicle is closest to the end of the road - highest xPos
 * Vehicles on front need to be updated first. */
void Road::indexRoad()
{
    // TODO: revert to linked lists
    // for(std::list<Vehicle> &lane : vehicles)
    //        lane.sort([](const auto &lhs, const auto &rhs)
    //        { return lhs.getPos() > rhs.getPos(); });
    for(std::vector<Vehicle> &lane : vehicles)
        std::sort(lane.begin(), lane.end(), [](const auto &lhs, const auto &rhs)
        { return lhs.getPos() > rhs.getPos(); });
}

/* get the next possible leading vehicle from lane */
std::list<Vehicle>::iterator nextLaneLeaderCandidate(const Vehicle &current, std::list<Vehicle> &nextLane)
{
    std::list<Vehicle>::iterator leadingV = std::upper_bound(nextLane.begin(), nextLane.end(), current,
                                                             [](const auto &lhs, const auto &rhs)
    {return lhs.getPos() < rhs.getPos();});

    return leadingV;
}

int getNextLaneLeaderPos(const Vehicle &current, const std::vector<Vehicle> &nextLane)
{
    if (nextLane.size() == 0)
        return -1;

    auto nextLeader = std::upper_bound(nextLane.rbegin(), nextLane.rend(), current,
                                       [](const auto &lhs, const auto &rhs)
                                       {return lhs.getPos() < rhs.getPos();});

    return std::distance(nextLane.begin(), nextLeader.base() ) - 1;
}

/* Lane change model:
 * http://traffic-simulation.de/MOBIL.html
 *
 * use std::vector<> for vehicles on a lane for debug purposes. Revert to linked list later.
 */
bool Road::changeLane(unsigned laneIndex, const Vehicle &currentVehicle, unsigned vehicleIndex)
{
    if (lanesNo == 1)
        return false;

    currentVehicle.log();
    /* no leading vehicle*/
    if(vehicleIndex == 0) {// TODO: when traffic lights will be on the road, they will have index 0, so so change this condition to 1
        return false;
    }

    const Vehicle &currentLaneLeader = vehicleIndex == 0 ? noVehicle : vehicles[laneIndex][vehicleIndex - 1];

    // quick exit condition - don't change lane before maxChangeLaneDist
    if(currentLaneLeader.getPos() - currentLaneLeader.getPos() > maxChangeLaneDist) {
        return false;
    }

    // prefer overtaking on the left
    int nextLanesIdxs[2] = { laneIndex + 1 < lanesNo  ? (int)laneIndex + 1 : -1,
                             (int)laneIndex - 1  >= 0 ? (int)laneIndex - 1 : -1 };

    for(int nextLaneIdx : nextLanesIdxs ) {
        if ( nextLaneIdx < 0 )
            continue;

        std::vector<Vehicle> &nextLane = vehicles[nextLaneIdx];

        int nextLeaderPos = getNextLaneLeaderPos(currentVehicle, nextLane);
        const Vehicle &nextLaneLeader    = nextLeaderPos == -1 ? noVehicle : nextLane[nextLeaderPos];
        const Vehicle &nextLaneFollower  = ((nextLeaderPos + 1 >= 0) &&
                                            (nextLeaderPos + 1 < nextLane.size())) ?
                    nextLane[nextLeaderPos + 1] : noVehicle;

        if (currentVehicle.canChangeLane(currentLaneLeader, nextLaneLeader, nextLaneFollower)) {
            nextLane.push_back(currentVehicle);
            log_debug("!!!!Vehicle %d change from lane %d to lane %d", currentVehicle.getId(), laneIndex, nextLaneIdx);
            return true;
        }

    }
    return false;
}

void Road::update(double dt)
{
    indexRoad();

    // TODO: first vehicle should always be a traffic light
    unsigned laneIndex = 0;
    for(auto &lane : vehicles) {
        log_debug("Lane %d *********", laneIndex);
        for(unsigned vIndex = 0; vIndex < lane.size(); ++vIndex) {
            Vehicle &current = lane[vIndex];
            if(current.isTrafficLight())
                current.update(dt, noVehicle);
            else {
                if (changeLane(laneIndex, current, vIndex) ) {// for every vehicle, check if a lane change is preferable
                    lane.erase(lane.begin() + vIndex);
                    indexRoad();
                    continue;
                }

                if (vIndex > 0)
                    current.update(dt, lane[vIndex-1]);
                else
                    current.update(dt, noVehicle);
            }
        }
        ++laneIndex;
    }
}

/* TODO: revert to linked list as vehicle lane for optimization */
//bool Road::changeLane(unsigned laneIndex, std::list<Vehicle>::iterator &currentVehicleIterator)
//{
//    Vehicle &currentVehicle = *(currentVehicleIterator);

//    if (lanesNo == 1)
//        return false;

//    // prefer overtaking on the left
//    int nextLanesIdxs[2] = { laneIndex + 1 < lanesNo  ? (int)laneIndex + 1 : -1,
//                             (int)laneIndex - 1  >= 0 ? (int)laneIndex - 1 : -1 };

//    for(int nextLaneIdx : nextLanesIdxs ) {
//        if ( nextLaneIdx < 0 )
//            continue;

//        std::list<Vehicle> &nextLane = vehicles[nextLaneIdx];

//        auto nextLeaderIterator = std::upper_bound(nextLane.begin(), nextLane.end(), currentVehicle,
//                                                   [](const auto &lhs, const auto &rhs)
//                                                    {return lhs.getPos() < rhs.getPos();});


//        auto currentLeaderIterator = std::prev(currentVehicleIterator);
//        auto nextLaneFollowerIterator = std::next(nextLeaderIterator);

//        if (currentVehicle.canChangeLane(currentLeaderIterator == vehicles[laneIndex].end() ? noVehicle : *currentLeaderIterator,
//                                         nextLeaderIterator == nextLane.end() ? noVehicle : *nextLeaderIterator,
//                                         nextLaneFollowerIterator == nextLane.end() ? noVehicle : *nextLaneFollowerIterator)) {
//            // lane change happenning. Remove vehicle from current lane on main loop (Road::update())
//            if ( nextLaneFollowerIterator == nextLane.end())
//                nextLane.push_back(currentVehicle);
//            else
//                nextLane.insert(nextLaneFollowerIterator, *currentVehicleIterator);
//            return true;
//        }
//    }
//    return false;
//}

//void Road::update(double dt)
//{
//    indexRoad();

//    // TODO: first vehicle should always be a traffic light
//    unsigned laneIndex = 0;
//    for(auto &lane : vehicles) {
//        auto it = lane.begin();
//        while(it != lane.end()) {
//            Vehicle &current = (*it);
//            if(current.isTrafficLight())
//                current.update(dt, noVehicle);
//            else {
//                if (changeLane(laneIndex, it) ) {// for every vehicle, check if a lane change is preferable
//                    log_info("Change lane occured");
//                    lane.erase(it++);
//                    continue;
//                }
//                auto currentLeaderIt = std::prev(it);
//                if (currentLeaderIt != lane.end())
//                    current.update(dt, *currentLeaderIt);
//                else
//                    current.update(dt, noVehicle);
//            }
//            ++it;
//        }
//        ++laneIndex;
//    }
//}

void Road::printRoad() const
{

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
