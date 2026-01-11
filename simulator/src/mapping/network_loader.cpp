#include "network_loader.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace mapping {

using json = nlohmann::json;

std::vector<simulator::Road> NetworkLoader::loadFromJson(const std::string& jsonFile) {
    std::ifstream file(jsonFile);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open network file: " + jsonFile);
    }

    json data;
    try {
        file >> data;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + jsonFile + ": " + e.what());
    }

    std::vector<simulator::Road> roads;

    if (!data.contains("roads") || !data["roads"].is_array()) {
        throw std::runtime_error("Invalid network file: missing 'roads' array");
    }

    // First pass: create all roads
    std::map<simulator::roadID, size_t> roadIdToIndex;

    for (const auto& roadJson : data["roads"]) {
        simulator::roadID id = roadJson["id"].get<simulator::roadID>();
        double length = roadJson["length"].get<double>();
        unsigned lanes = roadJson["lanes"].get<unsigned>();
        unsigned maxSpeed = roadJson["maxSpeed"].get<unsigned>();

        simulator::Road road(id, length, lanes, maxSpeed);

        // Set coordinates if available
        if (roadJson.contains("startLat") && roadJson.contains("startLon") &&
            roadJson.contains("endLat") && roadJson.contains("endLon")) {

            double startLat = roadJson["startLat"].get<double>();
            double startLon = roadJson["startLon"].get<double>();
            double endLat = roadJson["endLat"].get<double>();
            double endLon = roadJson["endLon"].get<double>();

            // Set geographic coordinates (lon, lat format as per roadPosGeo)
            road.setGeoCoordinates({startLon, startLat}, {endLon, endLat});
        }

        roadIdToIndex[id] = roads.size();
        roads.push_back(std::move(road));
    }

    // Second pass: add connections
    size_t idx = 0;
    for (const auto& roadJson : data["roads"]) {
        if (roadJson.contains("connections") && roadJson["connections"].is_array()) {
            for (const auto& connJson : roadJson["connections"]) {
                simulator::roadID targetId = connJson["roadId"].get<simulator::roadID>();
                double probability = connJson.value("probability", 1.0);

                // Add connection to all lanes
                unsigned numLanes = roads[idx].getLanesNo();
                for (unsigned l = 0; l < numLanes; l++) {
                    roads[idx].addLaneConnection(l, targetId, probability);
                }
            }
        }
        idx++;
    }

    std::cout << "Loaded " << roads.size() << " roads from " << jsonFile << std::endl;
    return roads;
}

void NetworkLoader::loadIntoCityMap(const std::string& jsonFile,
                                     std::map<simulator::roadID, simulator::Road>& cityMap) {
    auto roads = loadFromJson(jsonFile);

    cityMap.clear();
    for (auto& road : roads) {
        simulator::roadID id = road.getId();
        cityMap[id] = std::move(road);
    }

    std::cout << "Populated cityMap with " << cityMap.size() << " roads" << std::endl;
}

NetworkLoader::NetworkInfo NetworkLoader::getNetworkInfo(const std::string& jsonFile) {
    std::ifstream file(jsonFile);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open network file: " + jsonFile);
    }

    json data;
    try {
        file >> data;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }

    NetworkInfo info;

    if (data.contains("name")) {
        info.name = data["name"].get<std::string>();
    }
    if (data.contains("version")) {
        info.version = data["version"].get<std::string>();
    }
    if (data.contains("bbox") && data["bbox"].is_array() && data["bbox"].size() >= 4) {
        info.bboxMinLon = data["bbox"][0].get<double>();
        info.bboxMinLat = data["bbox"][1].get<double>();
        info.bboxMaxLon = data["bbox"][2].get<double>();
        info.bboxMaxLat = data["bbox"][3].get<double>();
    }
    if (data.contains("stats")) {
        if (data["stats"].contains("totalRoads")) {
            info.totalRoads = data["stats"]["totalRoads"].get<size_t>();
        }
        if (data["stats"].contains("totalIntersections")) {
            info.totalIntersections = data["stats"]["totalIntersections"].get<size_t>();
        }
        if (data["stats"].contains("totalConnections")) {
            info.totalConnections = data["stats"]["totalConnections"].get<size_t>();
        }
    }

    return info;
}

} // namespace mapping
