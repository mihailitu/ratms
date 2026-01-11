#ifndef OSM_TYPES_H
#define OSM_TYPES_H

#include <map>
#include <string>
#include <set>
#include <vector>
#include <cstdint>

namespace osm {

// Speed defaults by OSM highway type (in m/s)
// Based on typical German urban speed limits
const std::map<std::string, double> DEFAULT_SPEEDS = {
    {"motorway", 33.3},       // 120 km/h
    {"motorway_link", 22.2},  // 80 km/h
    {"trunk", 27.8},          // 100 km/h
    {"trunk_link", 16.7},     // 60 km/h
    {"primary", 13.9},        // 50 km/h
    {"primary_link", 11.1},   // 40 km/h
    {"secondary", 13.9},      // 50 km/h
    {"secondary_link", 11.1}, // 40 km/h
    {"tertiary", 13.9},       // 50 km/h
    {"tertiary_link", 11.1},  // 40 km/h
    {"residential", 8.3},     // 30 km/h
    {"living_street", 5.6},   // 20 km/h
    {"unclassified", 13.9},   // 50 km/h
    {"service", 5.6},         // 20 km/h
};

// Lane defaults by OSM highway type
const std::map<std::string, unsigned> DEFAULT_LANES = {
    {"motorway", 3},
    {"motorway_link", 1},
    {"trunk", 2},
    {"trunk_link", 1},
    {"primary", 2},
    {"primary_link", 1},
    {"secondary", 2},
    {"secondary_link", 1},
    {"tertiary", 1},
    {"tertiary_link", 1},
    {"residential", 1},
    {"living_street", 1},
    {"unclassified", 1},
    {"service", 1},
};

// Highway types to import (skip footways, cycleways, etc.)
const std::set<std::string> ROAD_HIGHWAY_TYPES = {
    "motorway", "motorway_link",
    "trunk", "trunk_link",
    "primary", "primary_link",
    "secondary", "secondary_link",
    "tertiary", "tertiary_link",
    "residential",
    "living_street",
    "unclassified",
    "service"
};

// Structure to hold parsed OSM way data before conversion to Road
struct OSMWay {
    int64_t osmId;
    std::string highwayType;
    std::string name;
    std::vector<int64_t> nodeIds;  // Ordered list of node IDs

    // Tags
    bool oneway = false;
    int lanes = -1;           // -1 means not specified
    double maxspeed = -1.0;   // -1 means not specified (in m/s)
    bool hasTrafficSignals = false;
};

// Structure to hold parsed OSM node data
struct OSMNode {
    int64_t osmId;
    double lat;
    double lon;
    bool isTrafficSignal = false;
};

// Structure for a road segment (between two intersection nodes)
struct RoadSegment {
    int64_t startNodeId;
    int64_t endNodeId;
    double startLat, startLon;
    double endLat, endLon;
    double length;           // in meters
    unsigned lanes;
    double maxSpeed;         // in m/s
    bool oneway;
    std::string name;
    int64_t osmWayId;        // Original OSM way ID for debugging
    bool hasTrafficLight = false;
};

// Parse maxspeed string to m/s
// Handles formats: "50", "50 km/h", "30 mph"
inline double parseMaxspeed(const std::string& maxspeedStr) {
    if (maxspeedStr.empty()) {
        return -1.0;
    }

    try {
        // Check for mph
        if (maxspeedStr.find("mph") != std::string::npos) {
            double mph = std::stod(maxspeedStr);
            return mph * 0.44704;  // mph to m/s
        }

        // Assume km/h (default in Germany)
        double kmh = std::stod(maxspeedStr);
        return kmh / 3.6;  // km/h to m/s
    } catch (...) {
        return -1.0;
    }
}

} // namespace osm

#endif // OSM_TYPES_H
