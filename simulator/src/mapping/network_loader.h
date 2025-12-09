#ifndef NETWORK_LOADER_H
#define NETWORK_LOADER_H

#include "core/road.h"

#include <string>
#include <vector>
#include <map>

namespace mapping {

/**
 * NetworkLoader - Loads road networks from JSON files
 *
 * The JSON format matches the output of osm_import tool.
 */
class NetworkLoader {
public:
    /**
     * Load road network from JSON file
     * @param jsonFile Path to JSON network file
     * @return Vector of Road objects ready for simulation
     */
    static std::vector<simulator::Road> loadFromJson(const std::string& jsonFile);

    /**
     * Load road network directly into a cityMap
     * @param jsonFile Path to JSON network file
     * @param cityMap Map to populate with roads
     */
    static void loadIntoCityMap(const std::string& jsonFile,
                                 std::map<simulator::roadID, simulator::Road>& cityMap);

    /**
     * Network metadata from JSON file
     */
    struct NetworkInfo {
        std::string name;
        std::string version;
        double bboxMinLon = 0, bboxMinLat = 0;
        double bboxMaxLon = 0, bboxMaxLat = 0;
        size_t totalRoads = 0;
        size_t totalIntersections = 0;
        size_t totalConnections = 0;
    };

    /**
     * Get metadata about a network file without fully loading it
     */
    static NetworkInfo getNetworkInfo(const std::string& jsonFile);
};

} // namespace mapping

#endif // NETWORK_LOADER_H
