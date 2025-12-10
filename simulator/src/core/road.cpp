#include "road.h"
#include "../utils/config.h"
#include "../utils/logger.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <random>

using namespace ratms;

namespace simulator
{

const Vehicle Road::noVehicle(0.0, 0.0, 0.0);
const double Road::minChangeLaneDist = 1.0;
const double Road::maxChangeLaneDist = 25.0;

static std::atomic<long> idSeed{0};

Road::Road()
{
}

Road::Road(roadID /*rId*/, double rLength, unsigned lanes, unsigned maxSpeed_mps ) :
    id (idSeed.fetch_add(1)), length(rLength), lanesNo(lanes), maxSpeed(maxSpeed_mps)
{
    LOG_DEBUG(LogComponent::Simulation, "New road added: id={}, length={:.2f}m, lanes={}, maxSpeed={}",
              id, length, lanesNo, maxSpeed);

    for(unsigned i = 0; i < lanesNo; ++i) {
        vehicles.push_back(std::list<Vehicle>());
        connections.push_back(std::vector<std::pair<roadID, double>>());
        // Use default constructor which has randomized phase offset
        trafficLights.push_back(TrafficLight());
    }

    trafficLightObject = Vehicle(length, 0.0, 0.0, Vehicle::traffic_light);
}

//
bool vehicleComparer(const Vehicle &v1, const Vehicle &v2)
{
    return v1.getPos() < v2.getPos();
}

//TODO: should vehicles be added from outside Road class or
// a road should maintain it's vehicle pool internally based on statistics?
bool Road::addVehicle(Vehicle v, unsigned lane)
{
    if(lane >= lanesNo) {
        LOG_WARN(LogComponent::Simulation, "Assigned vehicle to road {} on lane {}, where road has only {} lanes", id, lane, lanesNo);
        lane = 0; //TODO: throw exception?
    }

    // Add road to itinerary BEFORE inserting (since v is passed by value)
    v.addRoadToItinerary(id);

    auto vehiclePos = std::lower_bound(vehicles[lane].begin(), vehicles[lane].end(), v, &vehicleComparer);
    vehicles[lane].insert(vehiclePos, v);

    return true;
}

void Road::addLaneConnection(unsigned lane, roadID road, double usageProb)
{
    if (lane >= lanesNo) {
        LOG_ERROR(LogComponent::Simulation, "Cannot connect road {} with lane {}. Max lanes: {}", road, lane, lanesNo);
        return;
    }
    connections[lane].push_back({road, usageProb});
}


/* Lane change model:
 * http://traffic-simulation.de/MOBIL.html
 */
bool Road::tryLaneChange(const Vehicle &currentVehicle, const Vehicle &currentLaneLeader, unsigned currentLane) {

    if (lanesNo == 1)
        return false;

    // quick exit condition - don't change lane before maxChangeLaneDist
    if(currentLaneLeader.getPos() - currentVehicle.getPos() > maxChangeLaneDist) {
        return false;
    }

    // prefer overtaking on the left
    int nextLanesIdxs[2] = { currentLane + 1 < lanesNo  ? (int)currentLane + 1 : -1,
                             (int)currentLane - 1  >= 0 ? (int)currentLane - 1 : -1 };


    for(int nextLaneIdx : nextLanesIdxs) {
        if ( nextLaneIdx < 0 )
            continue;

        std::list<Vehicle> &nextLaneVehicles = vehicles[nextLaneIdx];

        auto nextLaneLeaderIterator = std::lower_bound(nextLaneVehicles.begin(), nextLaneVehicles.end(),
                                               currentVehicle, &vehicleComparer);

        const Vehicle &nextLaneLeader = (nextLaneLeaderIterator == nextLaneVehicles.end()) ?
                    noVehicle : *nextLaneLeaderIterator;

        auto nextLaneFollowerIterator = nextLaneVehicles.end();
        if (nextLaneLeaderIterator != nextLaneVehicles.begin() &&
                nextLaneVehicles.size() > 0)
            nextLaneFollowerIterator = std::prev(nextLaneLeaderIterator);

        const Vehicle &nextLaneFollower = (nextLaneFollowerIterator == nextLaneVehicles.end()) ?
                    noVehicle : *nextLaneFollowerIterator;

        if(currentVehicle.canChangeLane(currentLaneLeader, nextLaneLeader, nextLaneFollower)) {
            LOG_TRACE(LogComponent::Simulation, "Vehicle {} lane change {} -> {}",
                      currentVehicle.getId(), currentLane, nextLaneIdx);

            addVehicle(currentVehicle, nextLaneIdx);
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
 * @param pendingTransitions - vector to collect road transitions for two-phase update
 */
void Road::update(double dt, const std::map<roadID, Road> &cityMap,
                 std::vector<RoadTransition> &pendingTransitions)
{
/* - canCrossRoad ?
 *      - crossRoad() if:
 *          - thereIsAConnection
 *          - conndctedRoadNotFull
 *          - semaphoreIsGreen
 *          - vehicleIsFirstInLine
 *          - vehicleIsAtTheEndOfTheRoad
 */

/* TODO:
 *      - start slowing down vehicles if it's yellow light or is about to come
 *          (or maybe just cross on yellow if it's too late to stop (as in reality)
 *      - choose a connection that a vehicle will take earlier on the road so we can
 *          force that vehicle on the correct traffic lane.
 */

    // Vehicle const *nextVehicle = &trafficLightObject; // Using pointer is slightly faster
    std::reference_wrapper<Vehicle const> nextVehicle = trafficLightObject;

    unsigned currentLaneIndex = 0;
    for(auto &lane : vehicles) {

        trafficLights[currentLaneIndex].update(dt);

        if (trafficLights[currentLaneIndex].isGreen())
            nextVehicle = noVehicle;
        else
            nextVehicle = trafficLightObject;

        for(std::list<Vehicle>::reverse_iterator currentVehicle = lane.rbegin(); currentVehicle != lane.rend(); ++currentVehicle) {

            // If a road change is opportune but not possible, update against traffic light
            // If is yellow light and we are too close to stop, cross on yellow

            bool canChangeLane = true;
            bool canChangeRoad = true;

            currentVehicle->update(dt, nextVehicle);

            if (canChangeRoad && currentVehicle == lane.rbegin() && // first vehicle - get into another road
                    currentVehicle->getPos() >= length) { // is at the end of the road

                    bool roadChanged = performRoadChange(*currentVehicle, currentLaneIndex, cityMap, pendingTransitions);
                    if (roadChanged) {
                        lane.erase(--currentVehicle.base());
                        continue;
                    }
            }

            if (canChangeLane && currentVehicle->isSlowingDown() &&
                    !nextVehicle.get().isTrafficLight()) { // take over or pass obstacle

                bool laneChanged = tryLaneChange(*currentVehicle, nextVehicle, currentLaneIndex);

                if( laneChanged ) {
                    lane.erase(--currentVehicle.base());
                    continue;
                }
            }

            nextVehicle = (*currentVehicle);
        }
        ++currentLaneIndex;
    }
}

void Road::setCardinalCoordinates(roadPosCard startPos, roadPosCard endPos)
{
    startPosCard = startPos;
    endPosCard = endPos;

    // Convert Cartesian coordinates (meters) to geographic coordinates (lat/lon)
    // Using Munich as reference point: 48.1351°N, 11.582°E
    // At this latitude:
    //   1 degree latitude ≈ 111,000 meters
    //   1 degree longitude ≈ 71,500 meters (adjusted for latitude)
    const double REF_LAT = 48.1351;
    const double REF_LON = 11.582;
    const double METERS_PER_DEG_LAT = 111000.0;
    const double METERS_PER_DEG_LON = 71500.0;

    // Convert start position
    startPosGeo.first = REF_LON + (startPos.first / METERS_PER_DEG_LON);  // longitude
    startPosGeo.second = REF_LAT + (startPos.second / METERS_PER_DEG_LAT); // latitude

    // Convert end position
    endPosGeo.first = REF_LON + (endPos.first / METERS_PER_DEG_LON);
    endPosGeo.second = REF_LAT + (endPos.second / METERS_PER_DEG_LAT);
}

void Road::setGeoCoordinates(roadPosGeo startPos, roadPosGeo endPos) {
    startPosGeo = startPos;
    endPosGeo = endPos;
}

roadPosCard Road::getStartPosCard()
{
    return startPosCard;
}

roadPosCard Road::getEndPosCard()
{
    return endPosCard;
}

/**
 * @brief selectConnection - given connections and their probabilities,
 *                           choose one weighted connection
 * @param connections   - possible connections (roadID, probability pairs)
 * @return ID of the chosen connection
 *
 * Probabilities are normalized to sum to 1.0, so you can pass {A:0.7, B:0.3}
 * or {A:7, B:3} and both will work correctly.
 */
roadID selectConnection(std::vector<std::pair<roadID, double>> &connections)
{
    if (connections.empty())
        return -1;

    // Normalize probabilities to ensure they sum to 1.0
    double sum = 0.0;
    for(const auto& r : connections) {
        sum += r.second;
    }

    if (sum <= 0.0) {
        LOG_WARN(LogComponent::Simulation, "selectConnection: probabilities sum to zero, choosing first connection");
        return connections[0].first;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double rand = dist(gen);

    // Select road based on cumulative probability
    double cumulative = 0.0;
    for(const auto& r : connections) {
        cumulative += r.second / sum;
        if (rand <= cumulative) {
            return r.first;
        }
    }

    // Fallback to last connection (handles floating point rounding)
    return connections.back().first;
}

/**
 * @brief Road::vehicleCanJoinThisRoad - Check if vehicle can safely join this road
 * @param vehicle - vehicle attempting to join
 * @param lane - lane to check
 * @return true if there's sufficient space at the start of the lane
 */
bool Road::vehicleCanJoinThisRoad(const Vehicle &vehicle, unsigned lane) const
{
    if (lane >= lanesNo) {
        return false;
    }

    if (vehicles[lane].empty()) {
        return true;
    }

    // Check if first vehicle on lane is far enough ahead
    const Vehicle& firstVehicle = vehicles[lane].front();
    double requiredGap = vehicle.getLength() + minChangeLaneDist; // Use minimum safe distance

    return (firstVehicle.getPos() >= requiredGap);
}

/**
 * @brief Road::performRoadChange - Handle vehicle transition to next road
 * @param currentVehicle - vehicle attempting to change roads
 * @param laneIndex - current lane of the vehicle
 * @param cityMap - all roads in the city
 * @param pendingTransitions - vector to collect transitions for two-phase update
 * @return true if vehicle should be removed from current road
 */
bool Road::performRoadChange(const Vehicle &currentVehicle,
                             unsigned laneIndex,
                             const std::map<roadID, Road> &cityMap,
                             std::vector<RoadTransition> &pendingTransitions)
{
    // No connections -> vehicle leaves simulation
    if (connections[laneIndex].size() == 0) {
        LOG_TRACE(LogComponent::Simulation, "Vehicle {} leaving simulation (no connections from road {}, lane {})",
                 currentVehicle.getId(), id, laneIndex);
        return true; // Remove vehicle
    }

    auto laneConnections = connections[laneIndex];

    // Select next road based on probability weights
    roadID nextRoadID = selectConnection(laneConnections);

    if (nextRoadID == (roadID)-1) {
        LOG_ERROR(LogComponent::Simulation, "Failed to select connection for vehicle {} on road {}, lane {}",
                  currentVehicle.getId(), id, laneIndex);
        return true; // Remove vehicle to avoid stuck state
    }

    // Check if next road exists in cityMap
    auto nextRoadIt = cityMap.find(nextRoadID);
    if (nextRoadIt == cityMap.end()) {
        LOG_WARN(LogComponent::Simulation, "Vehicle {} cannot transition - destination road {} not in cityMap",
                 currentVehicle.getId(), nextRoadID);
        return true; // Remove vehicle
    }

    const Road& nextRoad = nextRoadIt->second;

    // For now, choose lane 0 on destination road
    // TODO: Smart lane selection based on downstream connections
    unsigned destLane = 0;
    if (nextRoad.getLanesNo() > 1) {
        // Prefer rightmost lane for now (lane 0)
        destLane = 0;
    }

    // Check if destination road has capacity
    if (!nextRoad.vehicleCanJoinThisRoad(currentVehicle, destLane)) {
        LOG_TRACE(LogComponent::Simulation, "Vehicle {} blocked at intersection - destination road {} lane {} is full",
                 currentVehicle.getId(), nextRoadID, destLane);
        return false; // Keep vehicle on current road (waiting at traffic light)
    }

    LOG_TRACE(LogComponent::Simulation, "Vehicle {} transitioning road {} -> {} (lane {})",
             currentVehicle.getId(), id, nextRoadID, destLane);

    // Add transition to pending list
    pendingTransitions.push_back(std::make_tuple(currentVehicle, nextRoadID, destLane));

    return true; // Remove from current road
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

roadPosGeo Road::getStartPosGeo() const
{
    return startPosGeo;
}

roadPosGeo Road::getEndPosGeo() const
{
    return endPosGeo;
}

void Road::printRoad() const
{
    LOG_DEBUG(LogComponent::Simulation, "Road {}: length={}, lanes={}, maxSpeed={}, vehicles={}",
              id, length, lanesNo, maxSpeed, vehicles.size());

    for(auto lane : vehicles )
        for(auto v : lane)
            v.printVehicle();
}

/**
 * @brief Road::spawnVehicle - Spawn a new vehicle at position 0
 * @param velocity - initial velocity of the vehicle
 * @param length - vehicle length (default 5.0m)
 * @return true if vehicle was spawned, false if no space available
 *
 * Picks the lane with fewest vehicles that has space at position 0.
 */
bool Road::spawnVehicle(double velocity, double length)
{
    // Find lane with fewest vehicles that has space
    int bestLane = -1;
    size_t minVehicles = SIZE_MAX;

    for (unsigned lane = 0; lane < lanesNo; ++lane) {
        // Check if there's space at the start of this lane
        if (vehicles[lane].empty()) {
            bestLane = lane;
            minVehicles = 0;
            break;  // Empty lane is best
        }

        // Check gap from start of road to first vehicle
        const Vehicle& firstVehicle = vehicles[lane].front();
        double requiredGap = length + minChangeLaneDist;

        if (firstVehicle.getPos() >= requiredGap && vehicles[lane].size() < minVehicles) {
            bestLane = lane;
            minVehicles = vehicles[lane].size();
        }
    }

    if (bestLane < 0) {
        LOG_TRACE(LogComponent::Simulation, "Cannot spawn vehicle on road {} - no space available", id);
        return false;
    }

    // Create and add vehicle at position 0
    Vehicle newVehicle(0.0, length, velocity);
    addVehicle(newVehicle, bestLane);

    LOG_TRACE(LogComponent::Simulation, "Spawned vehicle {} on road {} lane {} (v={:.1f} m/s)",
              newVehicle.getId(), id, bestLane, velocity);

    return true;
}

/**
 * @brief Road::spawnVehicle - Spawn vehicle on specific lane
 * @param lane Lane to spawn on
 * @param velocity Initial velocity
 * @param aggressivity Driver aggressivity (0.0-1.0)
 * @return true if vehicle was spawned, false if no space available
 */
bool Road::spawnVehicle(unsigned lane, double velocity, double aggressivity)
{
    if (lane >= lanesNo) {
        LOG_TRACE(LogComponent::Simulation, "Cannot spawn vehicle on road {} - invalid lane {}", id, lane);
        return false;
    }

    double length = 5.0;  // Default vehicle length
    double requiredGap = length + minChangeLaneDist;

    // Check if there's space at the start of this lane
    if (!vehicles[lane].empty()) {
        const Vehicle& firstVehicle = vehicles[lane].front();
        if (firstVehicle.getPos() < requiredGap) {
            LOG_TRACE(LogComponent::Simulation, "Cannot spawn vehicle on road {} lane {} - no space", id, lane);
            return false;
        }
    }

    // Create and add vehicle at position 0 with specified aggressivity
    Vehicle newVehicle(0.0, length, velocity);
    newVehicle.setAggressivity(aggressivity);
    addVehicle(newVehicle, lane);

    LOG_TRACE(LogComponent::Simulation, "Spawned vehicle {} on road {} lane {} (v={:.1f} m/s, aggr={:.2f})",
              newVehicle.getId(), id, lane, velocity, aggressivity);

    return true;
}

/**
 * @brief Road::getVehicleCount - Get total vehicle count across all lanes
 */
int Road::getVehicleCount() const
{
    int count = 0;
    for (const auto& lane : vehicles) {
        count += lane.size();
    }
    return count;
}

} // namespace simulator
