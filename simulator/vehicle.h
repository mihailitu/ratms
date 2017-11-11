#ifndef VEHICLE_H
#define VEHICLE_H

#include "defs.h"

#include <ostream>
#include <vector>

namespace simulator
{

class Vehicle
{
    static int idGen;
    int id;
    /* the length of the car.
     * We can have:
     *      - compact car - usual sedan 3.5 - 5 meters long
     *      - van - 7-8 meters long
     *      - bus - 12 - 18 meters long
     *      - truck - 20 -25 meters long
     * The type of the vechicle should be determined trough statistics:
     *      there should be some statistics with vehicle type distribution to a city - from the Cityhall
     *      in a regular city, sedans (or shorter should be more probable than trucks, etc
     *
     * NOTE: Use Vehicle class to represent traffic lights for now.
     *       We identify them as zero or negavice length vehicles with zero speed.
     *
     */
    double  length = { 5.0 };   // vechile length - see above
    double  xOrig = { 0.0 };    // when a vechicle is created, it has to start(appear) somewhere
    double  velocity = { 0.0 }; // current velocity. It will be updated through IDM equations
    double  xPos = { 0.0 };     // current position on the road. It will be updated through IDM equations

    double  s = { -1.0 };       // net distance to vehicle in front of this one (0 = accident, -1 = no vehicle in front
                                // for large values of net distance, we should enter in free road mode
    double acceleration = { 0 };// vehicle acceleration (meters per second square)

    /* Model parameters are here, as we make most of it dependent on this driver's aggressivity */
    double aggressivity = { 0.5 };  // aggressivity factor of this driver.
                                    // 0.5 - normal driver
                                    // < 0.5 altruist/prudent driver
                                    // > 0.5 aggressive/selfish driver

    double v0 = { 20.0 };   // Desired velocity - initialize to road's max speed
                            // Adjust depending on aggressivity - some drivers would want to go above speed limit,
                            //                                    while others will want to go lower than speed limit, determined by statistics
    double T = { 1.0 };     // Safe time headway - aggressivity dependent
    double a = { 1.5 };     // Maximum acceleration - linked to agressivity
    double b = { 3.0 };     // Desired deceleration - linked to agressivity
    double s0 = { 1.0 };    // Minimum distance - Some drivers are more agressive, while others are less agressive
                            //
    double delta = { 4.0 }; // Acceleration exponent

    double freeRoadDistance = { 100.0 }; // if net distance to vehicle ahead is larger, turn free road on

    /* Keep some stats about this vehicle.
     * We can compare itineraries and travel time between vehicles for performance measures */
    std::vector<roadID> itinerary; // itinerary of this vehicle.
    double roadTime; // time spent in traffic by this car

private:
    /* compute new acceleration considering next vehicle */
    double getNewAcceleration(const Vehicle &nextVehicle) const;
public:
    Vehicle( double _x_orig, double _length, double maxV );

    void update(double dt, const Vehicle &nextVehicle); // update position, acceleration and velocity

    bool canChangeLane(const Vehicle &currentLeader, const Vehicle &newLeader, const Vehicle &newFollower) const;

    void addRoadToItinerary(roadID rid);

    double getPos() const;
    double getAcceleration() const;
    double getLength() const;
    double getVelocity() const;

    bool isTrafficLight() const;

    void serialize(std::ostream &out) const;

    void serialize_v1(std::ostream &out) const;

    void printVehicle() const;
    void log() const;
    int getId() const { return id; }
};

} // simulator

#endif // VEHICLE_H
