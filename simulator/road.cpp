#include "road.h"
#include "logger.h"

Road::Road()
{

}

Road::Road( roadID id, int length, roadPos startPos, roadPos endpos ) :
    id (id), length(length), startPos(startPos), endPos(endpos)
{
    log_info("New road added: \n"
             "\t ID: %u \n"
             "\t length: %d m \n"
             "\t startPos: %f %f \n"
             "\t endPos:   %f %f \n"
             ,
             id, length, startPos.first, startPos.second, endPos.first, endPos.second);
}

Road::Road( roadID id, int length, int lanes, int maxSpeed ) :
    id (id), length(length), lanes(lanes), maxSpeed(maxSpeed)
{
    log_info("New road added: \n"
             "\t ID: %u \n"
             "\t length: %d m\n"
             "\t lanes: %d \n"
             "\t max_speed: %d km/h \n",
             id, length, lanes, maxSpeed);
}

//TODO: should vehicles be added from outside Road class or
// a road should maintain it's vehicle pool internally based on statistics?
void Road::addVehicle(Vehicle car)
{
    vehicles.push_back(car);
}

void Road::addConnection(roadID connection)
{
    connections.push_back(connection);
}
