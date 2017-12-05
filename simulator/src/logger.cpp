#include "logger.h"

#include <cstdio>
#include <iostream>

Logger::Logger()
{

}

void Logger::LogMessage(std::string &&message, std::string fName, int line, std::string date, std::string time)
{
    fprintf(stderr, "%s %s %s:%d - %s\n", date.c_str(), time.c_str(), fName.c_str(), line, message.c_str());
}
