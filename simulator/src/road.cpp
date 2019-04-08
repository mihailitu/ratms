#include "road.h"
#include "config.h"
#include "logger.h"

#include <algorithm>
#include <functional>

namespace simulator
{

const Vehicle Road::noVehicle(0.0, 0.0, 0.0);
const double Road::minChangeLaneDist = 0.5;
const double Road::maxChangeLaneDist = 25.0;

static long idSeed = 0;

Road::Road()
{
}

Road::Road(roadID /*rId*/, double rLength, unsigned lanes, unsigned maxSpeed_mps ) :
    id (idSeed++), length(rLength), usageProb(0.5), lanesNo(lanes), maxSpeed(maxSpeed_mps)
{
    log_info("New road added: \n"
             "\t ID: %u \n"
             "\t length: %.2f m\n"
             "\t max_speed: %u \n"
             "\t lanes: %u \n",
             id, length, maxSpeed, lanesNo);

    for(unsigned i = 0; i < lanesNo; ++i) {
        vehicles.push_back(std::list<Vehicle>());
        connections.push_back(std::vector<roadID>());
        trafficLights.push_back(TrafficLight(10, 1, 30, TrafficLight::red_light));
    }

    trafficLightObject = Vehicle(length, 0.0, 0.0, Vehicle::traffic_light);
}

//TODO: should vehicles be added from outside Road class or
// a road should maintain it's vehicle pool internally based on statistics?
void Road::addVehicle(Vehicle v, unsigned lane)
{
    if(lane >= lanesNo) {
        log_warning("Assigned vehicle to road %u on lane %d, where the road has only %d lanes.", id, lane, lanesNo);
        lane = 0; //TODO: throw exception?
    }

    auto comp = [](const Vehicle &v1, const Vehicle &v2)
    {
        return v1.getPos() < v2.getPos();
    };

    v.addRoadToItinerary(id);

    vehicles[lane].insert(std::lower_bound(vehicles[lane].begin(), vehicles[lane].end(), v, comp), v);
}

void Road::addLaneConnection(unsigned lane, roadID road)
{
    if (lane >= lanesNo) {
        log_error("Cannot connect road %u with lane %d. Max lanes: %d", road, lane, lanesNo);
        return;
    }
    connections[lane].push_back(road);
}


/* Lane change model:
 * http://traffic-simulation.de/MOBIL.html
 */
bool Road::performLaneChange(unsigned /*laneIndex*/,
                             const Vehicle &/*currentVehicle*/,
                             unsigned /*vehicleIndex*/)
{
    //    if (lanesNo == 1)
    //        return false;

    //    const Vehicle &currentLaneLeader = vehicleIndex == 0 ? noVehicle : vehicles[laneIndex][vehicleIndex - 1];

    //    // quick exit condition - don't change lane before maxChangeLaneDist
    //    if(currentLaneLeader.getPos() - currentLaneLeader.getPos() > maxChangeLaneDist) {
    //        return false;
    //    }

    //    // prefer overtaking on the left
    //    int nextLanesIdxs[2] = { laneIndex + 1 < lanesNo  ? (int)laneIndex + 1 : -1,
    //                             (int)laneIndex - 1  >= 0 ? (int)laneIndex - 1 : -1 };

    //    for(int nextLaneIdx : nextLanesIdxs ) {
    //        if ( nextLaneIdx < 0 )
    //            continue;

    //        std::vector<Vehicle> &nextLane = vehicles[nextLaneIdx];

    //        int nextLeaderPos = getNextLaneLeaderPos(currentVehicle, nextLane);
    //        const Vehicle &nextLaneLeader    = nextLeaderPos == -1 ? noVehicle : nextLane[nextLeaderPos];
    //        const Vehicle &nextLaneFollower  = ((nextLeaderPos + 1 >= 0) &&
    //                                            ((unsigned)nextLeaderPos + 1 < nextLane.size())) ?
    //                    nextLane[nextLeaderPos + 1] : noVehicle;

    //        if (currentVehicle.canChangeLane(currentLaneLeader, nextLaneLeader, nextLaneFollower)) {
    //            nextLane.insert(nextLane.begin() + nextLeaderPos + 1, currentVehicle);
    //            log_debug("Road %u: vehicle %d change from lane %d to lane %d", id, currentVehicle.getId(), laneIndex, nextLaneIdx);
    //            return true;
    //        }

    //    }
    return false;
}

/**
 * @brief Road::update - apply IDM equations to vehicles on this road.
 *                     - perfome lane changes according to MOBIL lane change equations
 * @param dt - update time
 * @param cityMap - all the roads from the city
 */
void Road::update(double dt,
                  const std::map<roadID, Road> &/*cityMap*/)
{

    /***
 * - isSlowingDown ?
 *      - takeOver() if:
 *          - isEnoughSpace on the next lane
 *          - changing lane will improve speed for this vehicle
 * - canCrossRoad ?
 *      - crossRoad() if:
 *          - thereIsAConnection
 *          - conndctedRoadNotFull
 *          - semaphoreIsGreen
 *          - vehicleIsFirstInLine
 *          - vehicleIsAtTheEndOfTheRoad
 */

    // Vehicle const *nextVehicle = &trafficLightObject; // Using pointer is slightly faster
    std::reference_wrapper<Vehicle const> nextVehicle = trafficLightObject;

    unsigned laneIndex = 0;
    for(auto &lane : vehicles) {

        trafficLights[laneIndex].update(dt);

        if (trafficLights[laneIndex].isGreen())
            nextVehicle = noVehicle;
        else
            nextVehicle = trafficLightObject;

        for(std::list<Vehicle>::reverse_iterator currentVehicle = lane.rbegin(); currentVehicle != lane.rend(); ++currentVehicle) {

            currentVehicle->update(dt, nextVehicle);

            if (currentVehicle == lane.rbegin() && // first vehicle - get into another road
                    currentVehicle->getPos() >= length) { // is at the end of the road

                if (connections[laneIndex].size() == 0) { // no more connections.
                    lane.erase(--currentVehicle.base());
                    continue;
                } else { // change roads

                }
            }

            if (currentVehicle->isSlowingDown() &&
                    !nextVehicle.get().isTrafficLight()) { // take over or pass obstacle
            }

            nextVehicle = (*currentVehicle);
        }
        ++laneIndex;
    }
}

void Road::setCardinalCoordinates(roadPosCard startPos, roadPosCard endPos)
{
    startPosCard = startPos;
    endPosCard = endPos;
}

roadPosCard Road::getStartPosCard()
{
    return startPosCard;
}

roadPosCard Road::getEndPosCard()
{
    return endPosCard;
}

bool Road::performRoadChange(const Vehicle &currentVehicle,
                             unsigned /*laneIndex*/,
                             const std::map<roadID, Road> &/*cityMap*/)
{
    if(currentVehicle.getPos() >= length) {
    }
    return false;
}

void Road::serialize(std::ostream &out) const
{
    serialize_v2(out);
}

const std::vector<std::list<Vehicle>>& Road::getVehicles() const
{
    return vehicles;
}

/*
 *  version 2:
 * roadID0 | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes_no |
 */
void Road::serialize_v2(std::ostream &out) const
{
    out << id << " " <<
           startPosGeo.first << " " <<
           startPosGeo.second << " " <<
           endPosGeo.first << " " <<
           endPosGeo.second << " " <<
           startPosCard.first << " " <<
           startPosCard.second << " " <<
           endPosCard.first << " " <<
           endPosCard.second << " " <<
           length << " " <<
           maxSpeed << " " <<
           lanesNo;
}

std::vector<char> Road::getCurrentLightConfig() const
{
    std::vector<char> lights(trafficLights.size());
    for(unsigned i = 0; i < trafficLights.size(); ++i)
        if(trafficLights[i].isGreen())
            lights[i] = 'G';
        else if (trafficLights[i].isYellow())
            lights[i] = 'Y';
        else lights[i] = 'R';

    return lights;
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
