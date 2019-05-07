#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <ctime>

#pragma GCC system_header

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

enum class LogLevels {
    debug = 0,
    info = 1,
    warning = 2,
    error = 3,
    none = 4
};

static const LogLevels log_level = LogLevels::debug;

static std::string Time() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::stringstream tt;
    tt << std::put_time(&tm, "%D %T");
    return tt.str();
}

//void log_info(fmt, ...)
//{
//    if ( log_level >= LogLevels::info ) {
//        fprintf( stdout, "INFO: %s %s:%d: " fmt "\n", Time().c_str(), __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
//    }
//}

// with date, time, file:line
#define log_info(fmt, ...) \
    do { \
            if ( log_level <= LogLevels::info ) { \
                fprintf( stdout, "INFO: %s %s:%d: " fmt "\n", Time().c_str(), __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
            } \
       } while(0)

// with date, time, file:line
#define log_error(fmt, ...) \
    do { \
            if ( log_level <= LogLevels::error) { \
                fprintf( stderr, "ERROR: %s %s:%d: " fmt "\n", Time().c_str(), __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
            } \
       } while(0)

#define log_warning(fmt, ...) \
    do { \
            if ( log_level <= LogLevels::warning) { \
                fprintf( stdout, "WARNING: %s %s:%d: " fmt "\n", Time().c_str(), __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
            } \
       } while(0)

// with date, time, file:line
#define log_debug(fmt, ...) \
    do { \
            if ( log_level <= LogLevels::debug) { \
                fprintf( stdout, "DEBUG: %s %s:%d: " fmt "\n", Time().c_str(), __FILENAME__, __LINE__, ##__VA_ARGS__ ); \
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
