#include "vehicle.h"
#include "logger.h"

Vehicle::Vehicle( int _x_orig, int _length ) :
    length(_length), x_orig(_x_orig),
    velocity(0), freeRoad(false)
{

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

void freeRoadOn(); // toggle free road on
void freeRoadOff(); // toggle free road off


void Vehicle::printVehicle() const
{
    log_info("\nVehicle:\n"
             "Originated: %d\n"
             "Position:   %d m\n"
             "Length:     %d m\n"
             "Velocity:   %d m/s\n"
             "Free road:  %s\n",
             x_orig, x_pos, length, velocity,
             freeRoad ? "yes" : "no" );
}
