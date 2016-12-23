#include "road.h"
#include "logger.h"

Road::Road()
{

}

Road::Road( roadID id, int length, roadPos startPos, roadPos endpos ) :
    id (id), length(length), startPos(startPos), endPos(endpos),
    usageProb(0.5)
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
    id (id), length(length), usageProb(0.5), lanes(lanes), maxSpeed(maxSpeed)
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

void Road::addConnection(Road connection)
{
    connections.push_back(connection.getId());
}

void Road::addConnections(std::vector<roadID> rconnections)
{
    for( roadID road : rconnections )
        connections.push_back(road);
}

void Road::addConnections(std::vector<Road> rconnections)
{
    for( Road r : rconnections )
        connections.push_back(r.getId());
}

void Road::printRoad() const
{
    std::string connections_str = "";
    for( unsigned i = 0; i < connections.size(); ++i ) {
        connections_str += std::to_string(connections[i]);
        if ( connections.size() != i+1 )
            connections_str += ", ";
    }

    log_info("\nRoad ID:    %u\n"
             "Length:       %d\n"
             "Lanes:        %d\n"
             "Max speed:    %d\n"
             "Usage:        %.2f\n"
             "Vehicle No.:  %d\n"
             "Start:        (%f, %f)\n"
             "End:          (%f, %f)\n"
             "Connections:  %s\n",
             id, length, lanes, maxSpeed, usageProb, vehicles.size(),
             startPos.first, startPos.second, endPos.first, endPos.second, connections_str.c_str());

    for(auto v : vehicles )
        v.printVehicle();
}
