#include "testweb.h"

namespace simulator
{

///
/// \brief buildWebTestMap - builds a map with hardcoded Cartesian coordonitates for roads
///                        to construct the web visual interface
/// \return a map with hardcoded cartesian coordinates
///
std::vector<Road> buildWebTestMap()
{
    // Baba Novac: Mihai Bravu - Campia Libertatii
    Road r1(0, 993, 2, 17.0);
    r1.setCardinalCoordinates({0, 0}, {0, 0});
    // Baba Novac: Campia Libertatii - Mihai Bravu
    Road r2(1, 993, 2, 17.0);
    r2.setCardinalCoordinates({0, 0}, {0, 0});

    std::vector<Road> cmap = { r1, r2 };

//    cmap[0].addConnection( cmap[1] );
//    cmap[1].addConnections({ cmap[2], cmap[7] });
//    cmap[2].addConnection( cmap[3] );
//    cmap[3].addConnection( cmap[4] );
//    cmap[4].addConnections({ cmap[5], cmap[6] });
//    cmap[5].addConnection( cmap[0] );
//    cmap[6].addConnection( cmap[2]);
//    cmap[7].addConnection( cmap[5] );
    return cmap;
}

} // namespace simulator
