#include <cmath>

#include "vehicle.h"
#include "logger.h"
#include "utils.h"

namespace simulator
{

int Vehicle::idGen = 0;

Vehicle::Vehicle( double _x_orig, double _length, double maxV ) :
    length(_length), xOrig(_x_orig), xPos(_x_orig), v0(maxV)
{
    id = idGen++;
    // log_info("New vehicle: ID: %d Pos: %2.f V: %.2f L: %.2f", id, xOrig, v0, length);
}

/*
 * Split IDM ODE with separate acceleration function, so we can use it also
 * for lane changing.
 */

double Vehicle::getNewAcceleration(const Vehicle &nextVehicle) const
{
    // ODE here
    // s alfa - net distance to vehicle directly on front
    double netDistance = nextVehicle.xPos - xPos - nextVehicle.length;

    bool freeRoad = false;

    // toggle free road
    if (netDistance <= 0)
        freeRoad = true;
    else if (netDistance >= freeRoadDistance)
        freeRoad = true;
    else
        freeRoad = false;

    // delta v - approaching rate
    double deltaV = velocity - nextVehicle.velocity;

    // S* - equation parameter
    double sStar = s0 + std::max(0.0, velocity * T + (velocity*deltaV)/(2*std::sqrt(a*b)));

    // calculate acceleration
    double newAcceleration = (a * (1.0 - std::pow(velocity/v0, delta) -
                          (freeRoad ? 0 : std::pow(sStar/netDistance,2))));

    return newAcceleration;
}

void Vehicle::update(double dt, const Vehicle &nextVehicle)
{
    roadTime += dt;

    // treat traffic lights as standing vehicles for now.
    // We identify traffic lights as zero length vehicles.
    // Zero speed vehicles will affect "real" vehicles.
    if (length <= 0)
        return;

    acceleration = getNewAcceleration(nextVehicle);

    // advance
    xPos += velocity * dt + (acceleration * std::pow(dt, 2)) / 2;

    // increase/decrease velocity
    velocity += acceleration * dt;
}

/* Lane change model:
 * http://traffic-simulation.de/MOBIL.html
 * Parameters:
 *      currentLeader - current vehicle in front of this vehicle
 *      newLeader - next leader candidate, on next lane
 *      newFollower - next follower candidate, on next lane
 */
bool Vehicle::canChangeLane(const Vehicle &currentLeader, const Vehicle &newLeader, const Vehicle &newFollower) const
{
//    if (currentLeader.getLength() <= 0 )
//        return false;

    // gap check
    bool hasGap = true;
    if (newLeader.getLength() > 0)
        hasGap = xPos < newLeader.getPos() - newLeader.getLength() - s0;

    if (newFollower.getLength() > 0)
            hasGap = hasGap && (xPos - length - s0 > newFollower.getPos());

    if (!hasGap)
        return false;

    // MOBIL
    double p = 0.3; // politeness factor TODO: make this the same as aggresivity
    double b_safe = 4.0; // maximum safe deceleration
    double a_thr = 0.2; // acceleration threshold: To avoid lane-change maneoeuvres triggered by marginal advantages

    // safety criterion
    bool isSafe = true;
    if (newFollower.getLength() > 0 ) {
        double nfacc = newFollower.getNewAcceleration(*this);
        isSafe = newFollower.getLength() > 0 ? (nfacc > -b_safe) : true;
    }

    if (!isSafe)
        return false;

    // incentive criterion
    double accNl = newLeader.getLength() > 0 ? getNewAcceleration(newLeader) : a; // a = max acceleration
    double accCl = currentLeader.getLength() > 0 ? getNewAcceleration(currentLeader) : a; // a = max acceleration
    unsigned ll = newFollower.getLength();
    double newFollowerNewAcc = ll > 0 ? newFollower.getNewAcceleration(*this) : 0;

    bool changeWanted =
            ((accNl - accCl) >
            ( p * (newFollower.getAcceleration() - newFollowerNewAcc) + a_thr) );

    return changeWanted;
}

void Vehicle::addRoadToItinerary(roadID rid)
{
    itinerary.push_back(rid);
}

double Vehicle::getVelocity() const
{
    return velocity;
}

double Vehicle::getAcceleration() const
{
    return acceleration;
}

double Vehicle::getPos() const
{
    return xPos;
}

double Vehicle::getLength() const
{
    return length;
}

bool Vehicle::isTrafficLight() const
{
    return (length <= 0) && (velocity == 0);
}

void Vehicle::serialize(std::ostream &out) const
{
    serialize_v1(out);
}

// version 1:
void Vehicle::serialize_v1(std::ostream &out) const
{
    out << xPos << " " <<
           velocity << " " <<
           acceleration << " ";

}

void Vehicle::printVehicle() const
{
    log_info("Vehicle:\n"
             "Originated: %.2f\n"
             "Position:   %.2f m\n"
             "Length:     %.2f m\n"
             "Velocity:   %.2f m/s\n",
             xOrig, xPos, length, velocity);
}

void Vehicle::log() const
{
    double mv = mps_to_kmh(velocity);
    double maxv = mps_to_kmh(v0);
    log_debug("id: %2d orig: %5.2f x: %5.2f v: %2.f max: %2.f a: %1.1f ", id, xOrig, xPos, mv, maxv, acceleration);
}

} // namespace simulator
