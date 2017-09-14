#include <cmath>

#include "vehicle.h"
#include "logger.h"

namespace simulator
{

Vehicle::Vehicle( double _x_orig, double _length, double maxV ) :
    length(_length), xOrig(_x_orig), xPos(_x_orig), v0(maxV)
{
    log_info("New vehicle: Pos: %2.f V: %.2f L: %.2f", xOrig, v0, length);
}

void Vehicle::update(double dt, const Vehicle &nextVehicle)
{
    roadTime += dt;

    // treat traffic lights as standing vehicles for now.
    // We identify traffic lights as zero length vehicles.
    // Zero speed vehicles will affect "real" vehicles.
    if (length <= 0)
        return;

    // ODE here
    // s alfa - net distance to vehicle directly on front
    double netDistance = nextVehicle.xPos - xPos - nextVehicle.length;

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
    acceleration = a * (1.0 - std::pow(velocity/v0, delta) - (freeRoad ? 0 : std::pow(sStar/netDistance,2)));

    // advance forward
    xPos += velocity * dt + (acceleration * std::pow(dt, 2)) / 2;

    // increase/decrease velocity
    velocity += acceleration * dt;
    log_info("pos: %.2f v: %.2f acc: %.2f", xPos, velocity, acceleration);
}

double Vehicle::getAcceleration() const
{
    return acceleration;
}

double Vehicle::getPos() const
{
    return xPos;
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
             "Velocity:   %.2f m/s\n"
             "Free road:  %s\n",
             xOrig, xPos, length, velocity,
             freeRoad ? "yes" : "no" );

    log_info("Vehicle model params: \n"
             "Desired velocity:      %.2f m/s\n"
             "Safe time headway:     %.2f s\n"
             "Maximum acceleration:  %.2f m/s^2\n"
             "Desired deleration:    %.2f m/s^2\n"
             "Acceleration exponent: %.2f\n"
             "Minimum distance:      %.2f m\n", v0, T, a, b, delta, s0);
}

} // namespace simulator
