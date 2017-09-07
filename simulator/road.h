#ifndef ROAD_H
#define ROAD_H

#include "vehicle.h"
#include "defs.h"

#include <utility>
#include <vector>

namespace simulator
{

class Road
{
    /***
     * A road is the one-way section between two semaphores. If a real road (from OMS or else) determines that
     * a road has two ways, we will tread those separately.
     *
     * It's length is expressed in meters and it will behave like x-axis for vechicle position. Vehicles only moves forward.
     *
     * It is characterized by GPS coordinates: start - end coordinates show us the direction of the traffic flow.
     * These coordinates will be also used to represent the road on a map (GUI, etc).
     *
     * A road has connections to other roads: where the traffic can go from this road.
     * Each road will have a probability that a car should choose it taken from an external and real statistic.
     * Busier roads will have a higher probability that a car will choose that road.
     *
     * Also, based on real statistics, cars might appear/disappear from roads, to temper with real city traffic statistics.
     * The point is: while we only know the number of cars at the sempahore points, the inner roads (residential roads, smaller entrances) will not be monitored.
     * Some cars might be heading home (enter this road), some might go home/work/shop etc. (exiting this road)
     */


private:
    // road ID - OMS related.
    // TODO - this ID could be duplicated in the case of two way roads. Maybe we should have two id's: OMS id and internal ID
    roadID id;

    // length of the road in meters
    unsigned length;
    // xPos of vechicle is the meter on the road

    // start position of the road - lat/lon - from OMS or something
    roadPos startPos; // lat/lon

    // end position of the road - lat/lon
    roadPos endPos; // lat/lon
    // the traffic will flow from startPos to endPos

    // this road's connections - id's of other roads.
    // TODO - maybe us some reference to other roads instead of ids so we can access quicker?
    std::vector<roadID> connections;

    // the preference probability for this road - how much it is used.
    // when a car passes the intersection, it will use this probability to choose the next road.
    float usageProb;

    // the number of lanes
    // TODO: assign vehicles to lanes on the road!!!
    unsigned lanesNo = { 1 };

    // road max speed m/s - if any or city speed limit - this doesn't have to be strictly conformed by drivers
    unsigned maxSpeed;

    // vehicles on this road, assigned to lanes
    std::vector<std::vector<Vehicle>> vehicles = {std::vector<Vehicle>()};

    static const Vehicle noVehicle; // we use this when no vehicle is on front - free road

public:
    Road();
    Road( roadID id, unsigned length, roadPos startPos, roadPos endpos );
    Road( roadID id, unsigned length, unsigned lanes, unsigned maxSpeed_mps );

    void addVehicle(Vehicle car, unsigned lane);
    void addConnection(roadID connection);
    void addConnection(Road connection);
    void addConnections(std::vector<roadID> rconnections);
    void addConnections(std::vector<Road> rconnections);

    roadID getId() const;
    unsigned getMaxSpeed() const;
    unsigned getLength() const;
    const std::vector<std::vector<Vehicle>>& getVehicles() const;

    // We need to sort vehicles on the road based on their position/lane
    // We need to to this every time before updating vehicle position,
    // because some vehicles might change lanes, some might arrive at their destination,
    // some may enter the road.
    void indexRoad();
    void update(double dt);

    void printRoad() const;

private:

};

} // namespae simulator

#endif // ROAD_H
