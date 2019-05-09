#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <ctime>

#pragma GCC system_header

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

enum LogLevels {
    all = 0,
    debug = 1,
    info = 2,
    warning = 3,
    error = 4,
    none = 5
};

volatile static LogLevels log_level = LogLevels::debug;

static std::string Time() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::stringstream tt;
    tt << std::put_time(&tm, "%D %T");
    return tt.str();
}

static const std::vector<std::string> logStrType = {
    "ALL",
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "NONE"
};

template <typename... Args>
void doPrint(std::ostream& out, LogLevels l, Args&&... args)
{
    if (log_level >= l) {
        out << logStrType[l] << ": " << Time() << ' ' << __FILENAME__ << ':' << __LINE__ << ": ";
        ((out << ' ' << std::forward<Args>(args)), ...);
        out << '\n';
    }
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
