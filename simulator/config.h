#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace simulator
{

class Config
{
public:

    // single vehicle - v1 serialization
    static const std::string singleVehicleTestFName;
    static const int simulationTestTime = 60 * 3 * 2; //

    // general values
    static const int simulationTime = simulationTestTime;
    static const std::string simulatorOuput;
    static const double DT;// simulator will update at 0.5 seconds.
};

} // namespace simulator

#endif // CONFIG_H
