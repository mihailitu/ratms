#include "road.h"
#include "config.h"
#include "logger.h"

#include <algorithm>

namespace simulator
{

const Vehicle Road::noVehicle(0.0, 0.0, 0.0);
Vehicle Road::trafficLightObject(0.0, 1.0, 0.0);
const double Road::minChangeLaneDist = 0.5;
const double Road::maxChangeLaneDist = 25.0;

Road::Road()
{
}

Road::Road( roadID id, double rLength, unsigned lanes, double maxSpeed_mps ) :
    id (id), length(rLength), usageProb(0.5), lanesNo(lanes), maxSpeed(maxSpeed_mps)
{
    log_info("New road added: \n"
             "\t ID: %u \n"
             "\t length: %.2f m\n"
             "\t lanes: %d \n"
             "\t max_speed: %.2f km/h \n",
             id, length, lanesNo, maxSpeed);

    for(unsigned i = 0; i < lanesNo; ++i) {
        vehicles.push_back(std::vector<Vehicle>());
        trafficLights.push_back(TrafficLight(10, 1, 30, TrafficLight::red_light));

    }

    trafficLightObject = Vehicle(length - Config::trafficLightDistToRoadEnd, 0.0, 0.0, Vehicle::traffic_light);
}

//TODO: should vehicles be added from outside Road class or
// a road should maintain it's vehicle pool internally based on statistics?
void Road::addVehicle(Vehicle v, unsigned lane)
{
    if(lane >= lanesNo) {
        log_warning("Assigned vehicle to road %u on lane %d, where the road has only %d lanes.", id, lane, lanesNo);
        lane = 0; //TODO: throw exception?
    }
    vehicles[lane].push_back(v);
    v.addRoadToItinerary(id);
}

void Road::addLaneConnection(unsigned lane, roadID road)
{
    if (lane >= lanesNo) {
        log_error("Cannot connect road %u with lane %d. Max lanes: %d", road, lane, lanesNo);
        return;
    }
    connections[lane].push_back(road);
}

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

const std::vector<std::vector<Vehicle>>& Road::getVehicles() const
{
    return vehicles;
}

/* Vehicles are sorted on descenting order: first vehicle is closest to the end of the road - highest xPos
 * Vehicles on front need to be updated first. */
void Road::indexRoad()
{
    for(std::vector<Vehicle> &lane : vehicles)
        std::sort(lane.begin(), lane.end(), [](const auto &lhs, const auto &rhs)
        { return lhs.getPos() > rhs.getPos(); });
}

/**
 * @brief getNextLaneLeaderPos
 * @param current - vehicle being updated
 * @param nextLane - target lane
 * @return the position of the vehicle from next lane which will be on front of @current if a lane change occurs
 */
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
 */
bool Road::performLaneChange(unsigned laneIndex, const Vehicle &currentVehicle, unsigned vehicleIndex)
{
    if (lanesNo == 1)
        return false;

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
                                            ((unsigned)nextLeaderPos + 1 < nextLane.size())) ?
                    nextLane[nextLeaderPos + 1] : noVehicle;

        if (currentVehicle.canChangeLane(currentLaneLeader, nextLaneLeader, nextLaneFollower)) {
            nextLane.insert(nextLane.begin() + nextLeaderPos + 1, currentVehicle);
            log_debug("Road %u: vehicle %d change from lane %d to lane %d", id, currentVehicle.getId(), laneIndex, nextLaneIdx);
            return true;
        }

    }
    return false;
}

/**
 * @brief Road::update - apply IDM equations to vehicles on this road.
 *                     - perfome lane changes according to MOBIL lane change equations
 * @param dt - update time
 * @param cityMap - all the roads from the city
 */
void Road::update(double dt, const std::map<roadID, Road> &cityMap)
{
    indexRoad();

    unsigned laneIndex = 0;
    for(auto &lane : vehicles) {

        trafficLights[laneIndex].update(dt);

        unsigned vIndex = 0;
        for(Vehicle &current : lane) {
            if(vIndex == 0) {
                if(trafficLights[laneIndex].isRed() || trafficLights[laneIndex].isYellow()) {
                    current.update(dt, trafficLightObject);
                } else {
                    // TODO: determine which road this vehicle will choose
                    if(performRoadChange(current, laneIndex, cityMap)) {
                        lane.erase(lane.begin() + vIndex);
                        continue;
                    }

                    // TODO: Even if we have a green light, check if next road is full.
                    current.update(dt, noVehicle);
                }
            } else {
                if (performLaneChange(laneIndex, current, vIndex) ) {
                    lane.erase(lane.begin() + vIndex);
                    continue;
                }
                current.update(dt, lane[vIndex - 1]);
            }
            ++vIndex;
        }
        ++laneIndex;
    }
}

bool Road::performRoadChange(const Vehicle &currentVehicle, unsigned laneIndex, const std::map<roadID, Road> &cityMap)
{
    if(currentVehicle.getPos() >= length) {
    }
    return false;
}

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
             startPosGeo.first, startPosGeo.second, endPosGeo.first, endPosGeo.second);//, connections_str.c_str());

    for(auto lane : vehicles )
        for(auto v : lane)
            v.printVehicle();
}

} // namespace simulator
