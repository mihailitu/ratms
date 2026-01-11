#ifndef OSM_IMPORTER_H
#define OSM_IMPORTER_H

#include "osm_types.h"
#include "core/road.h"

#include <string>
#include <vector>
#include <map>
#include <set>

namespace osm {

/**
 * OSMImporter - Imports road networks from OpenStreetMap files
 *
 * The import process:
 * 1. Parse OSM file to extract nodes and ways
 * 2. Identify intersection nodes (nodes used by multiple ways)
 * 3. Split ways at intersections into road segments
 * 4. Build connections between segments at intersections
 * 5. Convert to simulator::Road objects
 * 6. Optionally save to JSON for later loading
 */
class OSMImporter {
public:
    OSMImporter() = default;

    /**
     * Import road network from an OSM file (.osm or .osm.pbf)
     * @param osmFile Path to OSM file
     * @return Vector of Road objects ready for simulation
     */
    std::vector<simulator::Road> importFromFile(const std::string& osmFile);

    /**
     * Save imported road network to JSON file
     * @param roads Vector of Road objects
     * @param jsonFile Output JSON file path
     * @param networkName Optional name for the network
     */
    void saveToJson(const std::vector<simulator::Road>& roads,
                    const std::string& jsonFile,
                    const std::string& networkName = "Imported Network");

    /**
     * Get statistics about the last import
     */
    struct ImportStats {
        size_t nodesRead = 0;
        size_t waysRead = 0;
        size_t intersectionsFound = 0;
        size_t roadSegmentsCreated = 0;
        size_t connectionsCreated = 0;
        size_t trafficLightsFound = 0;
    };

    ImportStats getStats() const { return stats_; }

private:
    // Parsed OSM data
    std::map<int64_t, OSMNode> nodes_;
    std::vector<OSMWay> ways_;

    // Intersection detection
    std::map<int64_t, int> nodeUsageCount_;  // How many ways use each node
    std::set<int64_t> intersectionNodes_;     // Nodes used by 2+ ways

    // Road segments (ways split at intersections)
    std::vector<RoadSegment> segments_;

    // Final roads with assigned IDs
    std::map<simulator::roadID, RoadSegment> roadSegmentMap_;

    // Connection mapping: nodeId -> list of (roadId, isStart)
    // isStart=true means the road starts at this node
    std::map<int64_t, std::vector<std::pair<simulator::roadID, bool>>> nodeToRoads_;

    ImportStats stats_;

    // Parsing methods
    void parseOSMFile(const std::string& osmFile);
    void parseOSMXML(const std::string& osmFile);

    // Processing methods
    void identifyIntersections();
    void splitWaysAtIntersections();
    void buildConnections(std::vector<simulator::Road>& roads);

    // Utility methods
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    unsigned inferLanes(const OSMWay& way);
    double inferMaxSpeed(const OSMWay& way);
    simulator::roadID generateRoadId(int64_t osmWayId, int segmentIndex);
};

} // namespace osm

#endif // OSM_IMPORTER_H
