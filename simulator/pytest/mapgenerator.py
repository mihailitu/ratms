import random
import json

# Define the number of roads to generate
num_roads = 1000

# Define the bounds of the map
min_lon, max_lon = -118.4, -118.3
min_lat, max_lat = 33.95, 34.05

# Define the properties of each road
speed_limits = [25, 35, 45, 55, 65]
lanes = [1, 2, 3, 4]
traffic_levels = ["low", "medium", "high"]

# Define a function to generate a random road
def generate_road():
    num_points = random.randint(2, 10)
    points = []
    for i in range(num_points):
        lon = random.uniform(min_lon, max_lon)
        lat = random.uniform(min_lat, max_lat)
        points.append([lon, lat])
    speed_limit = random.choice(speed_limits)
    lane_count = random.choice(lanes)
    traffic = random.choice(traffic_levels)
    return {
        "type": "Feature",
        "geometry": {
            "type": "LineString",
            "coordinates": points
        },
        "properties": {
            "speed_limit": speed_limit,
            "lanes": lane_count,
            "traffic": traffic
        }
    }

# Generate a list of random roads
roads = []
for i in range(num_roads):
    road = generate_road()
    road["properties"]["id"] = f"road_{i}"
    road["properties"]["name"] = f"Road {i}"
    roads.append(road)

# Create the map data dictionary
map_data = {
    "type": "FeatureCollection",
    "features": roads
}

# Write the map data to a JSON file
with open("random_map.json", "w") as f:
    json.dump(map_data, f, indent=2)
