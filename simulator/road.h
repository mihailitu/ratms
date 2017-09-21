#ifndef ROAD_H
#define ROAD_H

#include "vehicle.h"
#include "defs.h"

#include <utility>
#include <vector>
#include <list>

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
     * The point is: while we only know the number of cars at the sempahore points,
     * inner roads (residential roads, smaller entrances) will not be monitored.
     * Some cars might be heading home (enter this road), some might go home/work/shop etc. (exiting this road)
     *
     * Some multi-lane roads have different semaphores for right, ahead and left.
     * Right and ahead are usually together; also, right turn can be always green, yielding vehicles comming from left.
     */


private:
    /*
     * road ID - OMS related.
     * TODO - this ID could be duplicated in the case of two way roads. Maybe we should have two id's: OMS id and internal ID
     */
    roadID id;

    /*
     * xPos of vechicle is the meter on the road
     * length of the road in meters
     */
    unsigned length;


    /* start position of the road - lat/lon - from OMS or something */
    roadPos startPos; // lat/lon

    /*
     * the traffic will flow from startPos to endPos
     * end position of the road - lat/lon
     */
    roadPos endPos; // lat/lon


    /*
     * TODO - maybe us some reference to other roads instead of ids so we can access quicker?
     * TODO - each lane has a connection to a road
     * this road's connections - id's of other roads.
     */
    std::vector<std::vector<roadID>> connections;

    /*
     * the preference probability for this road - how much it is used.
     * when a car passes the intersection, it will use this probability to choose the next road.
     */
    float usageProb;

    /* TODO: assign vehicles to lanes on the road!!!
     * the number of lanes */
    unsigned lanesNo = { 1 };

    /* road max speed m/s - if any or city speed limit -
     * this doesn't have to be strictly conformed by drivers */
    unsigned maxSpeed;

    /*
     * Right side driving only for now (left side steering wheel)
     *      lane 0 is the most right ("slow lane"), whilst lane n is the most left ("fast lane")
     * TODO - use a linked list instead of vector.
     *      - That way we can keep vehicles sorted and we don't have to sort the lane at each time step
     * Vehicles on this road, assigned to lanes
     */
    std::vector<std::list<Vehicle>> vehicles = {std::list<Vehicle>()};
    // std::vector<std::vector<Vehicle>> vehicles = {std::vector<Vehicle>()};

    static const Vehicle noVehicle; // we use this when no vehicle is on front - free road

private:

//     void changeLane(unsigned laneIndex, unsigned vehicleIndex);

public:
    Road();
    Road( roadID id, unsigned length, roadPos startPos, roadPos endpos );
    Road( roadID id, unsigned length, unsigned lanes, unsigned maxSpeed_mps );

    void addVehicle(Vehicle car, unsigned lane);
    void addConnection(roadID connection);
    void addConnection(Road connection);
    void addConnections(std::vector<roadID> rconnections);
    void addConnections(std::vector<Road> rconnections);

    // We need to sort vehicles on the road based on their position/lane
    // We need to to this every time before updating vehicle position,
    // because some vehicles might change lanes, some might arrive at their destination,
    // some may enter the road.
    void indexRoad();

    roadID getId() const;
    unsigned getMaxSpeed() const;
    unsigned getLength() const;
    unsigned getLanesNo() const;
    const std::vector<std::list<Vehicle>>& getVehicles() const;

    void update(double dt);

    void printRoad() const;

private:

};

} // namespae simulator

#endif // ROAD_H
