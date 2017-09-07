#include "config.h"

namespace simulator
{

const double Config::DT = 0.5; // simulator will update at 0.5 seconds.

const std::string Config::simpleRoadTestFName = "simple_road.dat";
const std::string Config::twoLaneRoadTestFName = "two_lanes.dat";
std::string Config::simulatorOuput = "output.dat";

}
