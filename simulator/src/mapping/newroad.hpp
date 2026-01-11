#include <string>
#include <vector>
#include <cmath>

class Road {
public:
    struct Point {
        double x;
        double y;
    };

    std::string id;
    std::string name;
    int speed_limit;
    int lanes;
    std::string traffic;
    std::vector<Point> points;

    Road(const std::string& id, const std::string& name, int speed_limit, int lanes, const std::string& traffic, const std::vector<Point>& points)
        : id(id), name(name), speed_limit(speed_limit), lanes(lanes), traffic(traffic), points(points) {}

    double length() const {
        double length = 0.0;
        for (int i = 1; i < points.size(); i++) {
            double dx = points[i].x - points[i - 1].x;
            double dy = points[i].y - points[i - 1].y;
            length += std::sqrt(dx * dx + dy * dy);
        }
        return length;
    }

    double duration() const {
        return length() / static_cast<double>(speed_limit);
    }
};