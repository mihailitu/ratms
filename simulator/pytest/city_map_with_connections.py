import osmnx as ox
import json
from shapely.geometry import mapping
from tqdm import tqdm
import re

# Define the city
city = "Segarcea, Romania"

try:
    # Retrieve the street network for the city
    print("Retrieving street network...")
    G = ox.graph_from_place(city, network_type='all')

    # Convert the street network to a GeoJSON FeatureCollection
    print("Converting street network to GeoJSON...")
    features = ox.graph_to_gdfs(G, nodes=False, fill_edge_geometry=True)
    features['geometry'] = features['geometry'].apply(lambda x: mapping(x))
    features = features.to_dict('records')

    # Add connections to other roads
    print("Adding connections to other roads...")
    node_to_edges = {}
    for edge in G.edges(keys=True, data=True):
        u, v, key, data = edge
        u_coord = (G.nodes[u]['y'], G.nodes[u]['x'])
        v_coord = (G.nodes[v]['y'], G.nodes[v]['x'])
        if u_coord not in node_to_edges:
            node_to_edges[u_coord] = []
        node_to_edges[u_coord].append((v_coord, key, data))
        if v_coord not in node_to_edges:
            node_to_edges[v_coord] = []
        node_to_edges[v_coord].append((u_coord, key, data))

    for feature in tqdm(features, desc='Adding connections'):
        try:
            start_coord = (feature['geometry']['coordinates'][0][1], feature['geometry']['coordinates'][0][0])
            end_coord = (feature['geometry']['coordinates'][-1][1], feature['geometry']['coordinates'][-1][0])
            connections = []
            for (coord, key, data) in node_to_edges[start_coord] + node_to_edges[end_coord]:
                if key != feature['properties']['key']:
                    connections.append(f"{coord[0]}_{coord[1]}_{key}")
            feature['properties']['connections'] = connections
        except Exception as e:
            print(f"Error: {e}")

    out_fname = './' + re.sub(r'\W+', '', city) + '.json'
        # Save the GeoJSON to a file
    print("Saving GeoJSON file..." + out_fname)
    geojson = {"type": "FeatureCollection", "features": features}
    # Define a regular expression pattern to remove non-literal characters from the output
    with open(out_fname, "w") as f:
        json.dump(geojson, f, indent=2)

    print("Done!")

except Exception as e:
    print(f"Error: {e}")
