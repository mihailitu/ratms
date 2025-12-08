#include <cmath>

#include "vehicle.h"
#include "../utils/logger.h"
#include "../utils/utils.h"

using namespace ratms;

namespace simulator
{

int Vehicle::idGen = 0;

Vehicle::Vehicle( double _x_orig, double _length, double maxV, ElementType vType ) :
    length(_length), xOrig(_x_orig), xPos(_x_orig), v0(maxV), type(vType)
{
    id = idGen++;
    std::string typeStr = "Vehicle";
    if(type == traffic_light)
        typeStr = "Traffic light";
    else if (type == obstacle)
        typeStr = "obstacle";

    // log_debug("New vehicle: ID: %d type: %s Pos: %2.f V: %.2f L: %.2f", id, typeStr.c_str(), xOrig, v0, length);
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
    if (isTrafficLight())
        return;

    roadTime += dt;

    acceleration = getNewAcceleration(nextVehicle);

    // advance
    xPos += velocity * dt + (acceleration * std::pow(dt, 2)) / 2;

    double currentVelocity = velocity;

    // increase/decrease velocity
    velocity += acceleration * dt;

    slowingDown = velocity < currentVelocity;
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
    // gap check
    bool hasGap = true;
    if (newLeader.getLength() > 0)
        hasGap = xPos < newLeader.getPos() - newLeader.getLength() - s0;

    if (newFollower.getLength() > 0)
            hasGap = hasGap && (xPos - length - s0 > newFollower.getPos());

    if (!hasGap)
        return false;

    // MOBIL
    double p = 0.3;         // politeness factor TODO: make this the same as aggresivity
    double b_safe = 4.0;    // maximum safe deceleration
    double a_thr = 0.2;     // acceleration threshold: To avoid lane-change maneoeuvres triggered by marginal advantages

    // safety criterion
    bool isSafe = true;
    if (newFollower.getLength() > 0 ) {
        double nfacc = newFollower.getNewAcceleration(*this);
        isSafe = newFollower.getLength() > 0 ? (nfacc > -b_safe) : true;
    }

    if (!isSafe)
        return false;

    // incentive criterion
    double accNl = newLeader.getLength() > 0 ? getNewAcceleration(newLeader) : a;           // a = max acceleration
    double accCl = currentLeader.getLength() > 0 ? getNewAcceleration(currentLeader) : a;   // a = max acceleration
    unsigned ll = newFollower.getLength();
    double newFollowerNewAcc = ll > 0 ? newFollower.getNewAcceleration(*this) : 0;

    bool changeWanted =
            ((accNl - accCl) >
            ( p * (newFollower.getAcceleration() - newFollowerNewAcc) + a_thr) );

    return changeWanted;
}

void Vehicle::addRoadToItinerary(roadID rId)
{
    itinerary.push_back(rId);
}

roadID Vehicle::getCurrentRoad() const
{
    return itinerary.back();
}

double Vehicle::getVelocity() const
{
    return velocity;
}

double Vehicle::getAcceleration() const
{
    return acceleration;
}

bool Vehicle::isSlowingDown() const
{
    return slowingDown;
}

double Vehicle::getPos() const
{
    return xPos;
}

void Vehicle::setPos(double newPos)
{
    xPos = newPos;
    xOrig = newPos;
}

double Vehicle::getLength() const
{
    return length;
}

bool Vehicle::isTrafficLight() const
{
    return type == traffic_light;
}

bool Vehicle::isVehicle() const
{
    return type == vehicle;
}

bool Vehicle::isObstacle() const
{
    return type == obstacle;
}

void Vehicle::serialize(std::ostream &out) const
{
    serialize_v1(out);
}

// version 1:
void Vehicle::serialize_v1(std::ostream &out) const
{
    out << " " << xPos << " " <<
           velocity << " " <<
           acceleration << " "<<
           id;
}

void Vehicle::printVehicle() const
{
    LOG_DEBUG(LogComponent::Simulation, "Vehicle id={}: pos={:.2f}m, vel={:.2f}m/s, len={:.2f}m",
              id, xPos, velocity, length);
}

void Vehicle::log() const
{
    double mv = mps_to_kmh(velocity);
    double maxv = mps_to_kmh(v0);
    LOG_DEBUG(LogComponent::Simulation, "Vehicle id={}: type={}, pos={:.2f}, vel={:.0f}/{:.0f}km/h, accel={:.1f}",
              id, static_cast<int>(type), xPos, mv, maxv, acceleration);
}

} // namespace simulator
