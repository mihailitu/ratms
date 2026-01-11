#include "osm_importer.h"
#include "nlohmann/json.hpp"

#include <expat.h>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <algorithm>

namespace osm {

using json = nlohmann::json;

// XML Parser context
struct XMLParserContext {
    OSMImporter* importer;
    OSMWay currentWay;
    bool inWay = false;
    std::map<int64_t, OSMNode>* nodes;
    std::vector<OSMWay>* ways;
    std::map<int64_t, int>* nodeUsageCount;
};

// XML element start handler
static void XMLCALL startElement(void* userData, const XML_Char* name, const XML_Char** atts) {
    XMLParserContext* ctx = static_cast<XMLParserContext*>(userData);

    auto getAttr = [&](const char* attrName) -> std::string {
        for (int i = 0; atts[i]; i += 2) {
            if (std::string(atts[i]) == attrName) {
                return std::string(atts[i + 1]);
            }
        }
        return "";
    };

    if (std::string(name) == "node") {
        OSMNode node;
        node.osmId = std::stoll(getAttr("id"));
        node.lat = std::stod(getAttr("lat"));
        node.lon = std::stod(getAttr("lon"));
        (*ctx->nodes)[node.osmId] = node;
    }
    else if (std::string(name) == "way") {
        ctx->inWay = true;
        ctx->currentWay = OSMWay();
        ctx->currentWay.osmId = std::stoll(getAttr("id"));
    }
    else if (ctx->inWay && std::string(name) == "nd") {
        int64_t ref = std::stoll(getAttr("ref"));
        ctx->currentWay.nodeIds.push_back(ref);
    }
    else if (ctx->inWay && std::string(name) == "tag") {
        std::string k = getAttr("k");
        std::string v = getAttr("v");

        if (k == "highway") {
            ctx->currentWay.highwayType = v;
        }
        else if (k == "name") {
            ctx->currentWay.name = v;
        }
        else if (k == "oneway") {
            ctx->currentWay.oneway = (v == "yes" || v == "true" || v == "1");
        }
        else if (k == "lanes") {
            try {
                ctx->currentWay.lanes = std::stoi(v);
            } catch (...) {}
        }
        else if (k == "maxspeed") {
            ctx->currentWay.maxspeed = parseMaxspeed(v);
        }
    }
    // Check for traffic signals on nodes
    else if (std::string(name) == "tag" && !ctx->inWay) {
        std::string k = getAttr("k");
        std::string v = getAttr("v");
        // Note: This won't work perfectly since we're in streaming mode
        // Traffic signals would need a second pass or different handling
    }
}

// XML element end handler
static void XMLCALL endElement(void* userData, const XML_Char* name) {
    XMLParserContext* ctx = static_cast<XMLParserContext*>(userData);

    if (std::string(name) == "way" && ctx->inWay) {
        ctx->inWay = false;

        // Only keep ways that are roads we care about
        if (ROAD_HIGHWAY_TYPES.count(ctx->currentWay.highwayType) > 0 &&
            ctx->currentWay.nodeIds.size() >= 2) {

            ctx->ways->push_back(ctx->currentWay);

            // Count node usage for intersection detection
            for (int64_t nodeId : ctx->currentWay.nodeIds) {
                (*ctx->nodeUsageCount)[nodeId]++;
            }
        }
    }
}

void OSMImporter::parseOSMXML(const std::string& osmFile) {
    std::ifstream file(osmFile);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open OSM file: " + osmFile);
    }

    XML_Parser parser = XML_ParserCreate(nullptr);
    if (!parser) {
        throw std::runtime_error("Failed to create XML parser");
    }

    XMLParserContext ctx;
    ctx.importer = this;
    ctx.nodes = &nodes_;
    ctx.ways = &ways_;
    ctx.nodeUsageCount = &nodeUsageCount_;

    XML_SetUserData(parser, &ctx);
    XML_SetElementHandler(parser, startElement, endElement);

    const size_t BUFFER_SIZE = 64 * 1024;
    char buffer[BUFFER_SIZE];

    bool done = false;
    while (!done) {
        file.read(buffer, BUFFER_SIZE);
        size_t len = file.gcount();
        done = (len < BUFFER_SIZE);

        if (XML_Parse(parser, buffer, static_cast<int>(len), done) == XML_STATUS_ERROR) {
            XML_ParserFree(parser);
            throw std::runtime_error("XML parse error: " +
                std::string(XML_ErrorString(XML_GetErrorCode(parser))));
        }
    }

    XML_ParserFree(parser);

    stats_.nodesRead = nodes_.size();
    stats_.waysRead = ways_.size();
}

void OSMImporter::parseOSMFile(const std::string& osmFile) {
    // Check file extension
    if (osmFile.ends_with(".pbf") || osmFile.ends_with(".osm.pbf")) {
        throw std::runtime_error("PBF format not supported yet. Please use .osm XML format.");
    }

    parseOSMXML(osmFile);
}

void OSMImporter::identifyIntersections() {
    intersectionNodes_.clear();

    for (const auto& [nodeId, count] : nodeUsageCount_) {
        if (count >= 2) {
            intersectionNodes_.insert(nodeId);
        }
    }

    // Also mark first and last nodes of each way as "intersections"
    // to ensure proper segment creation
    for (const auto& way : ways_) {
        if (!way.nodeIds.empty()) {
            intersectionNodes_.insert(way.nodeIds.front());
            intersectionNodes_.insert(way.nodeIds.back());
        }
    }

    stats_.intersectionsFound = intersectionNodes_.size();
}

double OSMImporter::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    // Haversine formula
    const double R = 6371000.0;  // Earth radius in meters
    const double dLat = (lat2 - lat1) * M_PI / 180.0;
    const double dLon = (lon2 - lon1) * M_PI / 180.0;

    const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                     std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
                     std::sin(dLon / 2) * std::sin(dLon / 2);

    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return R * c;
}

unsigned OSMImporter::inferLanes(const OSMWay& way) {
    if (way.lanes > 0) {
        return static_cast<unsigned>(way.lanes);
    }

    auto it = DEFAULT_LANES.find(way.highwayType);
    if (it != DEFAULT_LANES.end()) {
        return it->second;
    }

    return 1;  // Default
}

double OSMImporter::inferMaxSpeed(const OSMWay& way) {
    if (way.maxspeed > 0) {
        return way.maxspeed;
    }

    auto it = DEFAULT_SPEEDS.find(way.highwayType);
    if (it != DEFAULT_SPEEDS.end()) {
        return it->second;
    }

    return 13.9;  // Default 50 km/h
}

simulator::roadID OSMImporter::generateRoadId(int64_t osmWayId, int segmentIndex) {
    // Combine way ID and segment index into unique road ID
    // Use lower 48 bits for way ID, upper 16 bits for segment index
    return static_cast<simulator::roadID>(
        (static_cast<uint64_t>(segmentIndex) << 48) |
        (static_cast<uint64_t>(osmWayId) & 0xFFFFFFFFFFFF)
    );
}

void OSMImporter::splitWaysAtIntersections() {
    segments_.clear();
    roadSegmentMap_.clear();
    nodeToRoads_.clear();

    for (const auto& way : ways_) {
        if (way.nodeIds.size() < 2) continue;

        int segmentIndex = 0;
        size_t segmentStart = 0;

        for (size_t i = 1; i < way.nodeIds.size(); i++) {
            int64_t nodeId = way.nodeIds[i];

            // Split at intersections (but not at the very start)
            bool isIntersection = (intersectionNodes_.count(nodeId) > 0);
            bool isEnd = (i == way.nodeIds.size() - 1);

            if (isIntersection || isEnd) {
                // Create segment from segmentStart to i
                int64_t startNodeId = way.nodeIds[segmentStart];
                int64_t endNodeId = way.nodeIds[i];

                if (nodes_.count(startNodeId) == 0 || nodes_.count(endNodeId) == 0) {
                    segmentStart = i;
                    continue;
                }

                const OSMNode& startNode = nodes_[startNodeId];
                const OSMNode& endNode = nodes_[endNodeId];

                // Calculate segment length by summing distances between consecutive nodes
                double length = 0.0;
                for (size_t j = segmentStart; j < i; j++) {
                    if (nodes_.count(way.nodeIds[j]) > 0 && nodes_.count(way.nodeIds[j + 1]) > 0) {
                        const OSMNode& n1 = nodes_[way.nodeIds[j]];
                        const OSMNode& n2 = nodes_[way.nodeIds[j + 1]];
                        length += calculateDistance(n1.lat, n1.lon, n2.lat, n2.lon);
                    }
                }

                if (length < 1.0) {
                    length = 1.0;  // Minimum 1 meter
                }

                RoadSegment segment;
                segment.startNodeId = startNodeId;
                segment.endNodeId = endNodeId;
                segment.startLat = startNode.lat;
                segment.startLon = startNode.lon;
                segment.endLat = endNode.lat;
                segment.endLon = endNode.lon;
                segment.length = length;
                segment.lanes = inferLanes(way);
                segment.maxSpeed = inferMaxSpeed(way);
                segment.oneway = way.oneway;
                segment.name = way.name;
                segment.osmWayId = way.osmId;

                // Generate road ID
                simulator::roadID roadId = generateRoadId(way.osmId, segmentIndex);

                segments_.push_back(segment);
                roadSegmentMap_[roadId] = segment;

                // Track which roads start/end at each node
                nodeToRoads_[startNodeId].push_back({roadId, true});   // Road starts here
                nodeToRoads_[endNodeId].push_back({roadId, false});    // Road ends here

                segmentIndex++;
                segmentStart = i;
            }
        }

        // For two-way roads, create reverse segments
        if (!way.oneway) {
            // Reset and create reverse segments
            segmentIndex = 0;
            size_t segmentEnd = way.nodeIds.size() - 1;

            for (size_t i = way.nodeIds.size() - 1; i > 0; i--) {
                int64_t nodeId = way.nodeIds[i - 1];

                bool isIntersection = (intersectionNodes_.count(nodeId) > 0);
                bool isStart = (i == 1);

                if (isIntersection || isStart) {
                    int64_t startNodeId = way.nodeIds[segmentEnd];
                    int64_t endNodeId = way.nodeIds[i - 1];

                    if (nodes_.count(startNodeId) == 0 || nodes_.count(endNodeId) == 0) {
                        segmentEnd = i - 1;
                        continue;
                    }

                    const OSMNode& startNode = nodes_[startNodeId];
                    const OSMNode& endNode = nodes_[endNodeId];

                    double length = 0.0;
                    for (size_t j = segmentEnd; j > i - 1; j--) {
                        if (nodes_.count(way.nodeIds[j]) > 0 && nodes_.count(way.nodeIds[j - 1]) > 0) {
                            const OSMNode& n1 = nodes_[way.nodeIds[j]];
                            const OSMNode& n2 = nodes_[way.nodeIds[j - 1]];
                            length += calculateDistance(n1.lat, n1.lon, n2.lat, n2.lon);
                        }
                    }

                    if (length < 1.0) {
                        length = 1.0;
                    }

                    RoadSegment segment;
                    segment.startNodeId = startNodeId;
                    segment.endNodeId = endNodeId;
                    segment.startLat = startNode.lat;
                    segment.startLon = startNode.lon;
                    segment.endLat = endNode.lat;
                    segment.endLon = endNode.lon;
                    segment.length = length;
                    segment.lanes = inferLanes(way);
                    segment.maxSpeed = inferMaxSpeed(way);
                    segment.oneway = true;  // Reverse direction is treated as oneway
                    segment.name = way.name;
                    segment.osmWayId = way.osmId;

                    // Use negative segment index for reverse direction
                    simulator::roadID roadId = generateRoadId(way.osmId, -(segmentIndex + 1));

                    segments_.push_back(segment);
                    roadSegmentMap_[roadId] = segment;

                    nodeToRoads_[startNodeId].push_back({roadId, true});
                    nodeToRoads_[endNodeId].push_back({roadId, false});

                    segmentIndex++;
                    segmentEnd = i - 1;
                }
            }
        }
    }

    stats_.roadSegmentsCreated = segments_.size();
}

void OSMImporter::buildConnections(std::vector<simulator::Road>& roads) {
    // For each road, find other roads that start where this road ends
    for (auto& road : roads) {
        simulator::roadID roadId = road.getId();
        auto it = roadSegmentMap_.find(roadId);
        if (it == roadSegmentMap_.end()) continue;

        int64_t endNodeId = it->second.endNodeId;

        // Find all roads that start at this node
        if (nodeToRoads_.count(endNodeId) == 0) continue;

        std::vector<simulator::roadID> outgoingRoads;
        for (const auto& [otherRoadId, isStart] : nodeToRoads_[endNodeId]) {
            if (isStart && otherRoadId != roadId) {
                outgoingRoads.push_back(otherRoadId);
            }
        }

        if (outgoingRoads.empty()) continue;

        // Distribute connections across lanes with equal probability
        double prob = 1.0 / static_cast<double>(outgoingRoads.size());
        unsigned numLanes = road.getLanesNo();

        for (unsigned lane = 0; lane < numLanes; lane++) {
            for (simulator::roadID outRoadId : outgoingRoads) {
                road.addLaneConnection(lane, outRoadId, prob);
                stats_.connectionsCreated++;
            }
        }
    }
}

std::vector<simulator::Road> OSMImporter::importFromFile(const std::string& osmFile) {
    std::cout << "Parsing OSM file: " << osmFile << std::endl;

    // Reset state
    nodes_.clear();
    ways_.clear();
    nodeUsageCount_.clear();
    intersectionNodes_.clear();
    segments_.clear();
    roadSegmentMap_.clear();
    nodeToRoads_.clear();
    stats_ = ImportStats();

    // Step 1: Parse OSM file
    parseOSMFile(osmFile);
    std::cout << "  Parsed " << stats_.nodesRead << " nodes, " << stats_.waysRead << " ways" << std::endl;

    // Step 2: Identify intersections
    identifyIntersections();
    std::cout << "  Found " << stats_.intersectionsFound << " intersections" << std::endl;

    // Step 3: Split ways at intersections
    splitWaysAtIntersections();
    std::cout << "  Created " << stats_.roadSegmentsCreated << " road segments" << std::endl;

    // Step 4: Convert to Road objects
    // Note: Road constructor ignores the passed ID and generates sequential IDs
    // So we need to track the mapping from our internal IDs to actual road IDs
    std::vector<simulator::Road> roads;
    roads.reserve(segments_.size());

    // Create roads from segments (in order) and track the actual assigned IDs
    std::map<size_t, simulator::roadID> segmentIndexToRoadId;
    std::map<int64_t, std::vector<std::pair<size_t, bool>>> nodeToSegmentIndex;  // node -> [(segIndex, isStart)]

    for (size_t i = 0; i < segments_.size(); i++) {
        const auto& segment = segments_[i];

        simulator::Road road(
            0,  // ID will be auto-generated
            segment.length,
            segment.lanes,
            static_cast<unsigned>(segment.maxSpeed)
        );

        // The road now has an auto-assigned ID
        simulator::roadID actualRoadId = road.getId();
        segmentIndexToRoadId[i] = actualRoadId;

        // Track which roads connect at which nodes
        nodeToSegmentIndex[segment.startNodeId].push_back({i, true});   // Road starts here
        nodeToSegmentIndex[segment.endNodeId].push_back({i, false});    // Road ends here

        roads.push_back(std::move(road));
    }

    // Store segment info by actual road ID for JSON export
    roadSegmentMap_.clear();
    for (size_t i = 0; i < segments_.size(); i++) {
        roadSegmentMap_[segmentIndexToRoadId[i]] = segments_[i];
    }

    // Build nodeToRoads_ using actual road IDs
    nodeToRoads_.clear();
    for (const auto& [nodeId, segInfos] : nodeToSegmentIndex) {
        for (const auto& [segIndex, isStart] : segInfos) {
            simulator::roadID actualId = segmentIndexToRoadId[segIndex];
            nodeToRoads_[nodeId].push_back({actualId, isStart});
        }
    }

    // Step 5: Build connections
    buildConnections(roads);
    std::cout << "  Created " << stats_.connectionsCreated << " connections" << std::endl;

    return roads;
}

void OSMImporter::saveToJson(const std::vector<simulator::Road>& roads,
                              const std::string& jsonFile,
                              const std::string& networkName) {
    json output;
    output["name"] = networkName;
    output["version"] = "1.0";

    // Calculate bounding box
    double minLat = 90, maxLat = -90, minLon = 180, maxLon = -180;
    for (const auto& [roadId, segment] : roadSegmentMap_) {
        minLat = std::min(minLat, std::min(segment.startLat, segment.endLat));
        maxLat = std::max(maxLat, std::max(segment.startLat, segment.endLat));
        minLon = std::min(minLon, std::min(segment.startLon, segment.endLon));
        maxLon = std::max(maxLon, std::max(segment.startLon, segment.endLon));
    }
    output["bbox"] = {minLon, minLat, maxLon, maxLat};

    // Export roads
    json roadsJson = json::array();
    for (const auto& road : roads) {
        simulator::roadID roadId = road.getId();

        json roadJson;
        roadJson["id"] = roadId;

        // Get segment data for coordinates
        if (roadSegmentMap_.count(roadId) > 0) {
            const auto& segment = roadSegmentMap_[roadId];
            roadJson["startLat"] = segment.startLat;
            roadJson["startLon"] = segment.startLon;
            roadJson["endLat"] = segment.endLat;
            roadJson["endLon"] = segment.endLon;
            roadJson["name"] = segment.name;
            roadJson["osmWayId"] = segment.osmWayId;
        }

        roadJson["length"] = road.getLength();
        roadJson["lanes"] = road.getLanesNo();
        roadJson["maxSpeed"] = road.getMaxSpeed();

        // Export connections (we need to rebuild these from the road object)
        // For now, we'll re-derive from nodeToRoads_
        if (roadSegmentMap_.count(roadId) > 0) {
            const auto& segment = roadSegmentMap_[roadId];
            json connectionsJson = json::array();

            if (nodeToRoads_.count(segment.endNodeId) > 0) {
                for (const auto& [otherRoadId, isStart] : nodeToRoads_[segment.endNodeId]) {
                    if (isStart && otherRoadId != roadId) {
                        json connJson;
                        connJson["roadId"] = otherRoadId;
                        connJson["lane"] = 0;  // Simplified: same connection for all lanes
                        size_t numOutgoing = 0;
                        for (const auto& [r, s] : nodeToRoads_[segment.endNodeId]) {
                            if (s && r != roadId) numOutgoing++;
                        }
                        connJson["probability"] = 1.0 / static_cast<double>(numOutgoing);
                        connectionsJson.push_back(connJson);
                    }
                }
            }
            roadJson["connections"] = connectionsJson;
        }

        roadsJson.push_back(roadJson);
    }

    output["roads"] = roadsJson;

    // Statistics
    output["stats"] = {
        {"totalRoads", roads.size()},
        {"totalIntersections", stats_.intersectionsFound},
        {"totalConnections", stats_.connectionsCreated}
    };

    // Write to file
    std::ofstream file(jsonFile);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open output file: " + jsonFile);
    }

    file << output.dump(2);
    file.close();

    std::cout << "Saved network to: " << jsonFile << std::endl;
}

} // namespace osm
