#include "testintersection.h"

namespace simulator {


std::vector<Road> singleLaneIntersectionTest()
{
    std::vector<Road> cmap;

    Road r0(0, 1500, 1, 20);
    Road r1(1,  750, 1, 15);

    r0.addLaneConnection(1, r1.getId());

    cmap.push_back(r0);
    cmap.push_back(r1);

    return cmap;
}

} // namespace simulator
