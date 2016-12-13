#include "road.h"
#include "logger.h"
Road::Road( roadID id, int length, roadPos startPos, roadPos endpos ) :
    id (id), length(length), startPos(startPos), endPos(endpos)
{
    log_info("New road added: \n"
             "\t ID: %ul \n"
             "\t length: %d \n"
             "\t startPos: %f %f \n"
             "\t endPos:   %f %f \n"
             ,
             id, length, startPos.first, startPos.second, endPos.first, endPos.second);
}
