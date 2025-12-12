#!/usr/bin/env python3
"""
OSM to RATMS Network Converter

Downloads road network data from OpenStreetMap and converts it to
RATMS JSON format for traffic simulation.

Usage:
    python3 osm_to_ratms.py --bbox "minLon,minLat,maxLon,maxLat" --output output.json --name "Area Name"

Example for Bucharest Dristor:
    python3 osm_to_ratms.py --bbox "26.12,44.41,26.16,44.43" --output bucharest_dristor.json --name "Bucharest Dristor"
"""

import argparse
import json
import math
import requests
from collections import defaultdict


OVERPASS_URL = "https://overpass-api.de/api/interpreter"

# Road types to include (in order of priority for speed limits)
ROAD_TYPES = {
    'motorway': 120,
    'motorway_link': 80,
    'trunk': 100,
    'trunk_link': 60,
    'primary': 50,
    'primary_link': 40,
    'secondary': 50,
    'secondary_link': 40,
    'tertiary': 40,
    'tertiary_link': 30,
    'residential': 30,
    'living_street': 20,
    'unclassified': 30,
    'service': 20,
}

# Default lanes by road type
DEFAULT_LANES = {
    'motorway': 3,
    'motorway_link': 1,
    'trunk': 2,
    'trunk_link': 1,
    'primary': 2,
    'primary_link': 1,
    'secondary': 2,
    'secondary_link': 1,
    'tertiary': 1,
    'tertiary_link': 1,
    'residential': 1,
    'living_street': 1,
    'unclassified': 1,
    'service': 1,
}


def haversine_distance(lat1, lon1, lat2, lon2):
    """Calculate distance between two points in meters."""
    R = 6371000  # Earth radius in meters
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    delta_phi = math.radians(lat2 - lat1)
    delta_lambda = math.radians(lon2 - lon1)

    a = math.sin(delta_phi/2)**2 + math.cos(phi1) * math.cos(phi2) * math.sin(delta_lambda/2)**2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))

    return R * c


def fetch_osm_data(bbox):
    """Fetch road data from Overpass API."""
    min_lon, min_lat, max_lon, max_lat = bbox

    # Build query for all road types
    highway_filter = '|'.join(ROAD_TYPES.keys())

    query = f"""
    [out:json][timeout:60];
    (
      way["highway"~"^({highway_filter})$"]({min_lat},{min_lon},{max_lat},{max_lon});
    );
    out body;
    >;
    out skel qt;
    """

    print(f"Fetching OSM data for bbox: {bbox}")
    response = requests.post(OVERPASS_URL, data={'data': query})
    response.raise_for_status()

    return response.json()


def parse_osm_data(osm_data):
    """Parse OSM data into nodes and ways."""
    nodes = {}
    ways = []

    for element in osm_data['elements']:
        if element['type'] == 'node':
            nodes[element['id']] = {
                'lat': element['lat'],
                'lon': element['lon']
            }
        elif element['type'] == 'way':
            tags = element.get('tags', {})
            highway_type = tags.get('highway', 'unclassified')

            # Get speed limit (km/h to m/s)
            max_speed_kmh = ROAD_TYPES.get(highway_type, 30)
            if 'maxspeed' in tags:
                try:
                    max_speed_kmh = int(tags['maxspeed'].replace(' km/h', '').replace(' mph', ''))
                except ValueError:
                    pass
            max_speed_ms = max_speed_kmh / 3.6

            # Get number of lanes
            lanes = DEFAULT_LANES.get(highway_type, 1)
            if 'lanes' in tags:
                try:
                    lanes = int(tags['lanes'])
                except ValueError:
                    pass

            # Check if one-way
            oneway = tags.get('oneway', 'no') in ['yes', '1', 'true']

            ways.append({
                'id': element['id'],
                'nodes': element['nodes'],
                'name': tags.get('name', ''),
                'highway': highway_type,
                'maxSpeed': max_speed_ms,
                'lanes': max(1, lanes // 2) if not oneway else lanes,  # Split lanes for bidirectional
                'oneway': oneway,
            })

    return nodes, ways


def build_road_network(nodes, ways, bbox):
    """Build RATMS road network from OSM ways."""
    roads = []
    road_id = 0

    # Map from (start_node, end_node) to road_id for connection building
    node_pair_to_road = {}

    # Map from node_id to list of road_ids that end at this node
    node_to_incoming_roads = defaultdict(list)
    node_to_outgoing_roads = defaultdict(list)

    min_lon, min_lat, max_lon, max_lat = bbox

    for way in ways:
        way_nodes = way['nodes']

        # Create road segments for each consecutive pair of nodes
        for i in range(len(way_nodes) - 1):
            start_node_id = way_nodes[i]
            end_node_id = way_nodes[i + 1]

            if start_node_id not in nodes or end_node_id not in nodes:
                continue

            start = nodes[start_node_id]
            end = nodes[end_node_id]

            # Skip if outside bbox
            if not (min_lat <= start['lat'] <= max_lat and min_lon <= start['lon'] <= max_lon):
                continue
            if not (min_lat <= end['lat'] <= max_lat and min_lon <= end['lon'] <= max_lon):
                continue

            length = haversine_distance(start['lat'], start['lon'], end['lat'], end['lon'])

            # Skip very short segments
            if length < 5:
                continue

            # Create forward road
            road = {
                'id': road_id,
                'osmWayId': way['id'],
                'name': way['name'],
                'startLat': start['lat'],
                'startLon': start['lon'],
                'endLat': end['lat'],
                'endLon': end['lon'],
                'length': round(length, 1),
                'maxSpeed': round(way['maxSpeed'], 1),
                'lanes': way['lanes'],
                'connections': []
            }
            roads.append(road)
            node_pair_to_road[(start_node_id, end_node_id)] = road_id
            node_to_incoming_roads[end_node_id].append(road_id)
            node_to_outgoing_roads[start_node_id].append(road_id)
            road_id += 1

            # Create reverse road if not one-way
            if not way['oneway']:
                road = {
                    'id': road_id,
                    'osmWayId': way['id'],
                    'name': way['name'],
                    'startLat': end['lat'],
                    'startLon': end['lon'],
                    'endLat': start['lat'],
                    'endLon': start['lon'],
                    'length': round(length, 1),
                    'maxSpeed': round(way['maxSpeed'], 1),
                    'lanes': way['lanes'],
                    'connections': []
                }
                roads.append(road)
                node_pair_to_road[(end_node_id, start_node_id)] = road_id
                node_to_incoming_roads[start_node_id].append(road_id)
                node_to_outgoing_roads[end_node_id].append(road_id)
                road_id += 1

    # Build connections: roads that end at a node connect to roads that start at that node
    for node_id in node_to_incoming_roads:
        incoming = node_to_incoming_roads[node_id]
        outgoing = node_to_outgoing_roads[node_id]

        if not outgoing:
            continue

        prob = round(1.0 / len(outgoing), 6) if outgoing else 0

        for in_road_id in incoming:
            for out_road_id in outgoing:
                # Don't connect road to itself or immediate U-turn
                in_road = roads[in_road_id]
                out_road = roads[out_road_id]

                # Check for U-turn (same OSM way, opposite direction)
                if (in_road['startLat'] == out_road['endLat'] and
                    in_road['startLon'] == out_road['endLon'] and
                    in_road['endLat'] == out_road['startLat'] and
                    in_road['endLon'] == out_road['startLon']):
                    continue

                roads[in_road_id]['connections'].append({
                    'roadId': out_road_id,
                    'lane': 0,
                    'probability': prob
                })

    return roads


def main():
    parser = argparse.ArgumentParser(description='Convert OSM data to RATMS network format')
    parser.add_argument('--bbox', required=True, help='Bounding box: minLon,minLat,maxLon,maxLat')
    parser.add_argument('--output', required=True, help='Output JSON file')
    parser.add_argument('--name', default='Network', help='Network name')

    args = parser.parse_args()

    # Parse bbox
    bbox = [float(x) for x in args.bbox.split(',')]
    if len(bbox) != 4:
        print("Error: bbox must have 4 values: minLon,minLat,maxLon,maxLat")
        return 1

    # Fetch and parse OSM data
    osm_data = fetch_osm_data(bbox)
    print(f"Received {len(osm_data['elements'])} elements from OSM")

    nodes, ways = parse_osm_data(osm_data)
    print(f"Parsed {len(nodes)} nodes and {len(ways)} ways")

    # Build road network
    roads = build_road_network(nodes, ways, bbox)
    print(f"Created {len(roads)} road segments")

    # Count connections
    total_connections = sum(len(r['connections']) for r in roads)
    print(f"Total connections: {total_connections}")

    # Build output
    network = {
        'name': args.name,
        'bbox': bbox,
        'roads': roads
    }

    # Write output
    with open(args.output, 'w') as f:
        json.dump(network, f, indent=2)

    print(f"Wrote network to {args.output}")
    return 0


if __name__ == '__main__':
    exit(main())
