#include "road.h"
#include "logger.h"

#include <algorithm>

namespace simulator
{

const Vehicle Road::noVehicle(0.0, 0.0, 0.0);

Road::Road()
{
}

Road::Road( roadID id, unsigned length, roadPos startPos, roadPos endpos ) :
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

Road::Road( roadID id, unsigned length, unsigned lanes, unsigned maxSpeed_mps ) :
    id (id), length(length), usageProb(0.5), lanesNo(lanes), maxSpeed(maxSpeed_mps)
{
    log_info("New road added: \n"
             "\t ID: %u \n"
             "\t length: %d m\n"
             "\t lanes: %d \n"
             "\t max_speed: %d km/h \n",
             id, length, lanesNo, maxSpeed);
    for(unsigned i = 1; i < lanesNo; ++i)
        vehicles.push_back(std::vector<Vehicle>());
}

//TODO: should vehicles be added from outside Road class or
// a road should maintain it's vehicle pool internally based on statistics?
void Road::addVehicle(Vehicle car, unsigned lane)
{
    if(lane >= lanesNo) {
        log_warning("Assigned vehicle to road %u on lane %d, where the road has only %d lanes.", id, lane, lanesNo);
        lane = 0; //TODO: throw exception?
    }
    vehicles[lane].push_back(car);
}

//void Road::addConnection(roadID connection)
//{
//    connections.push_back(connection);
//}

//void Road::addConnection(Road connection)
//{
//    connections.push_back(connection.getId());
//}

//void Road::addConnections(std::vector<roadID> rconnections)
//{
//    for( roadID road : rconnections )
//        connections.push_back(road);
//}

//void Road::addConnections(std::vector<Road> rconnections)
//{
//    for( Road r : rconnections )
//        connections.push_back(r.getId());
//}

roadID Road::getId() const
{
        return id;
}

unsigned Road::getMaxSpeed() const
{
    return maxSpeed;
}

unsigned Road::getLanesNo() const
{
    return lanesNo;
}

unsigned Road::getLength() const
{
    return length;
}

const std::vector<std::vector<Vehicle>>& Road::getVehicles() const
{
    return vehicles;
}

// Vehicles are sorted on descenting order: first vehicle is closest to the end of the road - highest xPos
// Vehicles on front need to be updated first.
void Road::indexRoad()
{
    for(auto &lane : vehicles)
        std::sort(lane.begin(), lane.end(),
                  [](const auto& lhs, const auto& rhs)
                        {return lhs.getPos() > rhs.getPos();});
}

void Road::update(double dt)
{
    indexRoad();

    for(auto &lane : vehicles)
        for(unsigned i = 0; i < lane.size(); ++i) // see indexRoad() comments for clarification
            if (i == 0 ) // first vehicle (highest xPos) has no leading vehicle
                lane[i].update(dt, noVehicle);
            else
                lane[i].update(dt, lane[i-1]);
}

void Road::printRoad() const
{
//    std::string connections_str = "";
//    for( unsigned i = 0; i < connections.size(); ++i ) {
//        connections_str += std::to_string(connections[i]);
//        if ( connections.size() != i+1 )
//            connections_str += ", ";
//    }

    log_info("Road ID:    %u\n"
             "Length:       %d\n"
             "Lanes:        %d\n"
             "Max speed:    %d\n"
             "Usage:        %.2f\n"
             "Vehicle No.:  %d\n"
             "Start:        (%f, %f)\n"
             "End:          (%f, %f)\n"
             "Connections:  %s\n",
             id, length, lanesNo, maxSpeed, usageProb, vehicles.size(),
             startPos.first, startPos.second, endPos.first, endPos.second);//, connections_str.c_str());

    for(auto lane : vehicles )
        for(auto v : lane)
            v.printVehicle();
}

} // namespace simulator
