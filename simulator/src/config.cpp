#include "config.h"

namespace simulator
{

const double Config::DT = 0.5; // simulator will update at 0.5 seconds.

const std::string Config::simpleRoadTestFName = "simple_road.dat";

std::string Config::simulatorOuput = "output.dat";
std::string Config::simulatorMap = "roads.dat";

bool Config::outputSimulationToDisk = false;

}
