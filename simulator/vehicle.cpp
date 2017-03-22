#include "vehicle.h"
#include "logger.h"

#include <cmath>

Vehicle::Vehicle( int _x_orig, int _length ) :
    length(_length), xOrig(_x_orig), xPos(_x_orig)
{

}

void Vehicle::advance(int distance)
{
    xPos += distance;
}

void Vehicle::update(double dt, Vehicle &nextVehicle)
{
    // ODE here
    // s alfa - net distance to vehicle directly on front
    double netDistance = nextVehicle.xPos - xPos - nextVehicle.length;

    // delta v - approaching rate
    double delta_v = velocity - nextVehicle.velocity;

    // S* - equation parameter
    double Sstar = s0 + velocity * T + (velocity*delta_v)/std::sqrt(a*b);

    // calculate acceleration
    double acceleration = a * (1 -
                               std::pow(velocity/v0, delta -
                               std::pow(Sstar/netDistance,2)));
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
}
