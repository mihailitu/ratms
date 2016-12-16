#ifndef ROAD_H
#define ROAD_H

#include <utility>
#include <vector>
#include "vehicle.h"

class Road
{
    /***
     * A road is the one-way section between two semaphores. If a real road (from OMS or else) determines that
     * a road has two ways, we will tread those separately.
     *
     * It's length is expressed in meters and it will behave like x-axis for vechicle position
     *
     * It is characterized by GPS coordinates: start - end coordinates show us the direction of the traffic flow.
     * These coordinates will be also used to represent the road on a map (GUI, etc).
     *
     * A road has connections to other roads: where the traffic can go from this road.
     * Each connection will have a probability that a car should choose it taken from an external and real statistic.
     * Busier roads will have a higher probability that a car will choose that road.
     *
     * Also, based on real statistics, cars might appear/disappear from roads, to temper with real city traffic statistics.
     * The point is: while we only know the number of cars at the sempahore points, the inner roads (residential roads, smaller entrances) will not be monitored.
     * Some cars might be leaving home (enter this road), some might go home/work/shop etc. (exiting this road)
     */

    // road ID - OMS related.
    // TODO - this ID could be duplicated in the case of two way roads. Maybe we should have two id's: OMS id and internal ID
    typedef unsigned long roadID;
    roadID id;

    // length of the road in meters
    int length;
    // x_pos of vechicle is the meter on the road

    typedef std::pair<float, float> roadPos;
    // start position of the road - lat/lon - from OMS or something
    roadPos startPos; // lat/lon

    // end position of the road - lat/lon
    roadPos endPos; // lat/lon
    // the traffic will flow from startPos to endPos

    // this road's connections - id's of other roads.
    // TODO - maybe us some reference to other roads instead of ids so we can access quicker?
    std::vector<float> connections;

    // the preference probability for this road - how much it is used.
    // when a car passes the intersection, it will use this probability to choose the next road.
    float usageProb;

    // the number of lanes
    int lanes;

    // road max speed - if any or city speed limit - this doesn't have to strictly conformed by drivers
    int maxSpeed;

    // vehicles on this road
    std::vector<Vehicle> vehicles;

public:
    Road() = delete;
    Road( roadID id, int length, roadPos startPos, roadPos endpos );
    Road(roadID id, int length, int lanes, int maxSpeed );
};

#endif // ROAD_H
