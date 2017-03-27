#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <cstring>
#include <cstdio>

#pragma GCC system_header

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define DEBUG_INFO 1
#define DEBUG_ERROR 1
#define DEBUG_WARNING 1
#define DEBUG_MESSAGE

// with date, time, file:line
#define log_info(fmt, ...) \
    do { \
            if ( DEBUG_INFO) { \
                fprintf( stderr, "INFO: %s %s %s:%d: \n" fmt "\n", __DATE__, __TIME__, __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
            } \
       } while(0)

// with date, time, file:line
#define log_error(fmt, ...) \
    do { \
            if ( DEBUG_ERROR) { \
                fprintf( stderr, "ERROR: %s %s %s:%d: \n" fmt "\n", __DATE__, __TIME__, __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
            } \
       } while(0)

#define log_warning(fmt, ...) \
    do { \
            if ( DEBUG_WARNING) { \
                fprintf( stderr, "WARNING: %s %s %s:%d: \n" fmt "\n", __DATE__, __TIME__, __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
            } \
       } while(0)

//#define log_info(fmt, ...) \
//    do { \
//            if ( DEBUG_INFO) { \
//                fprintf( stderr, fmt "\n", ##__VA_ARGS__ ); \
//            } \
//       } while(0)


class Logger
{
    Logger();
public:

    static void LogMessage( std::string&& message, std::string fName, int line, std::string date, std::string time);
};

#endif // LOGGER_H
