#ifndef ROAD_H
#define ROAD_H

#include "vehicle.h"
#include "defs.h"
#include "trafficlight.h"

#include <utility>
#include <vector>
#include <list>
#include <map>

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
    double length;


    /* Geodetic coortdinates */
    /* start position of the road - lat/lon - from OMS or something */
    roadPosGeo startPosGeo; // lat/lon

    /*
     * the traffic will flow from startPos to endPos
     * end position of the road - lat/lon
     */
    roadPosGeo endPosGeo; // lat/lon


    /* Cartesian coordinates - for easier visual 2D representation */
    /* The Cartesian coordinates are set in meters for now */
    roadPosCard startPosCard;
    roadPosCard endPosCard;

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
     * TODO - maybe use some reference to other roads instead of ids so we can access quicker?
     * TODO - each lane has a connection to another road
     * this road's connections - id's of other roads.
     */
    std::vector<std::vector<roadID>> connections;

    /*
     * Right side driving only for now (left side steering wheel)
     *      lane 0 is the most right ("slow lane"), whilst lane n is the most left ("fast lane")
     * TODO - use a linked list instead of vector.
     *      - That way we can keep vehicles sorted and we don't have to sort the lane at each time step
     * Vehicles on this road, assigned to lanes
     */
    std::vector<std::list<Vehicle>> vehicles;

    /* Every lane from a road has a TrafficLight object associated with it.
     * Each traffic light is updated independently.
     * With first vehicle, if the light on it's lane is Red, then that vehicle
     * will have it's next vehicle (the leader) the static TrafficLight object (zero speed, zero length, etc).
     * If the light is green, then the first vehicle's leader will be none
     */
    std::vector<TrafficLight> trafficLights;

    static Vehicle trafficLightObject;

    /* Don't consider lane change when leader is more than minChangeLaneDist ahead.
     * This is a minor optimization - we don't do all the math for lane change if it's no needed */
    static const double maxChangeLaneDist; // 25 meters
    static const double minChangeLaneDist; // 1 meters

    static const Vehicle noVehicle; // we use this when no vehicle is on front - free road

private:

    /**
     * @brief changeLane     - perform a lane change of currentVehicle, if it's phisically possible and if can gain some acceleration
     * @param laneIndex      - the index of the lane that the current vehicle is driving on
     * @param currentVehicle - a reference to currentVehicle object
     * @param vehicleIndex   - currentVehicle's index on the current lane
     * @return true if vehicle has changed lane or false if not
     */
    bool performLaneChange(unsigned laneIndex, const Vehicle &currentVehicle, unsigned vehicleIndex);

    /**
     * @brief performRoadChange - change the road that currentVehicle is driving on, if necessary
     * @param currentVehicle - current updated vehicle
     * @param laneIndex      - lane being processed
     * @param cityMap        - all roads from this city
     * @return true if currentVehicle moved to another road, false otherwise
     */
    bool performRoadChange(const Vehicle &currentVehicle, unsigned laneIndex, const std::map<roadID, Road> &cityMap);

public:
    Road();
    Road(roadID rId, double length, unsigned lanes, unsigned maxSpeed_mps);

    void addVehicle(Vehicle v, unsigned lane);

    /**
     * Each lane from a road has it's own connection to a road.
     * For example: for a road with 3 lanes, the most left lane might be forced to
     * left only turns or might be connected with left and ahead directions.
     * Also, right lane vehicles might be able to turn right on red light, but move ahead and right on green light
     * @brief addLaneConnection - makes the connection between a lane the road that vehicles might take
     * @param lane - lane number
     * @param road - road ID the the vehicles on this current lane might use
     */
    void addLaneConnection(unsigned lane, roadID road);

    roadID getId() const;
    unsigned getMaxSpeed() const;
    unsigned getLength() const;
    unsigned getLanesNo() const;
    const std::vector<std::list<Vehicle>>& getVehicles() const;

    void update(double dt, const std::map<roadID, Road> &cityMap );

    void setCardinalCoordinates(roadPosCard startPos, roadPosCard endPosCard);
    roadPosCard getStartPosCard();
    roadPosCard getEndPosCard();

    void serialize(std::ostream &out) const;

    void printRoad() const;

private:
    void serialize_v2(std::ostream &out) const;

};

} // namespae simulator

#endif // ROAD_H
