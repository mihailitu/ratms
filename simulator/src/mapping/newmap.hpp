#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "newroad.hpp"

using json = nlohmann::json;

class RoadMap {
public:
    std::vector<Road> roads;

    RoadMap(const std::string& filename) {
        std::ifstream input_file(filename);
        if (!input_file) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        json data;
        input_file >> data;

        for (auto& feature : data["features"]) {
            std::string id = feature["properties"]["id"];
            std::string name = feature["properties"]["name"];
            int speed_limit = feature["properties"]["speed_limit"];
            int lanes = feature["properties"]["lanes"];
            std::string traffic = feature["properties"]["traffic"];

            std::vector<Road::Point> points;
            for (auto& point : feature["properties"]["points"]) {
                double x = point["x"];
                double y = point["y"];
                points.push_back({x, y});
            }

            Road road(id, name, speed_limit, lanes, traffic, points);
            roads.push_back(road);
        }
    }
};