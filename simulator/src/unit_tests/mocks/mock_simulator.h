/**
 * Mock Simulator for unit testing
 * Uses Google Mock to create test doubles
 */

#ifndef MOCK_SIMULATOR_H
#define MOCK_SIMULATOR_H

#include <gmock/gmock.h>
#include "../../core/simulator.h"
#include "../../core/road.h"

namespace ratms {
namespace test {

class MockSimulator {
public:
    simulator::Simulator::CityMap cityMap;

    MOCK_METHOD(void, runSimulator, ());
    MOCK_METHOD(void, runTestSimulator, ());
    MOCK_METHOD(void, addRoadToMap, (simulator::Road& r));
    MOCK_METHOD(void, addRoadNetToMap, (std::vector<simulator::Road>& roadNet));
    MOCK_METHOD(void, serialize, (double time, std::ostream& output), (const));
};

} // namespace test
} // namespace ratms

#endif // MOCK_SIMULATOR_H
