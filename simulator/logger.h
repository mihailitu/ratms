#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <cstring>
#include <cstdio>

#pragma GCC system_header

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define log_info(fmt, ...) \
    do { \
        if ( 1 ) \
            fprintf(stderr, "%s %s %s:%d: " fmt "\n", __DATE__, __TIME__, __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
       } while(0)

class Logger
{
    Logger();
public:

    static void LogMessage( std::string&& message, std::string fName, int line, std::string date, std::string time);
};

#endif // LOGGER_H
