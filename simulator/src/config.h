#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace simulator
{

class Config
{
public:

    // simple road with few folowing vehicles
    static const std::string simpleRoadTestFName;
    static const std::string simpleMapTestFName;

    static const int simulationTestTime = 60 * 2 * 2; //

    // general values
    static const int simulationTime = simulationTestTime;
    static std::string simulatorOuput;
    static std::string simulatorMap;
    static const double DT;// simulator will update at 0.5 seconds.
};

} // namespace simulator

#endif // CONFIG_H
