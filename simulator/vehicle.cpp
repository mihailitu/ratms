#include "vehicle.h"
#include "logger.h"

#include <cmath>

Vehicle::Vehicle( int _x_orig, int _length ) :
    length(_length), xOrig(_x_orig), xPos(_x_orig)
{

}

void Vehicle::update(double dt, const Vehicle &nextVehicle)
{
    // ODE here
    // s alfa - net distance to vehicle directly on front
    double netDistance = nextVehicle.xPos - xPos - nextVehicle.length;

    // delta v - approaching rate
    double delta_v = velocity - nextVehicle.velocity;

    // S* - equation parameter
    double Sstar = s0 + std::max(0.0, velocity * T + (velocity*delta_v)/(2*std::sqrt(a*b)));

    // calculate acceleration
    double acceleration = a * (1 -
                               std::pow(velocity/v0, delta) -
                               freeRoad ? 0 : std::pow(Sstar/netDistance,2));

    // advance forward
    xPos += velocity * dt + (acceleration * std::pow(dt, 2)) / 2;

    // increase/decrease velocity
    velocity += acceleration * dt;
}

bool Vehicle::onFreeRoad() const
{
    return freeRoad;
}

void Vehicle::freeRoadOn()
{
    freeRoad = true;
}

void Vehicle::freeRoadOff()
{
    freeRoad = false;
}

void Vehicle::printVehicle() const
{
    log_info("Vehicle:\n"
             "Originated: %d\n"
             "Position:   %d m\n"
             "Length:     %d m\n"
             "Velocity:   %d m/s\n"
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
