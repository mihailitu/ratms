#include "vehicle.h"
#include "logger.h"

Vehicle::Vehicle( int _x_orig, int _length ) :
    length(_length), xOrig(_x_orig),
    velocity(0), xPos(_x_orig), freeRoad(false)
{

}

void Vehicle::advance(int distance)
{
    xPos += distance;
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
