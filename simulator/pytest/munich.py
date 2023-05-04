import osmnx as ox
import json
from tqdm import tqdm

# Define the city
city = "Munich, Germany"

# Retrieve the street network for the city
print("Retrieving street network...")
G = ox.graph_from_place(city, network_type='all')

# Convert the street network to a GeoJSON FeatureCollection
print("Converting street network to GeoJSON...")
features = ox.graph_to_gdfs(G, nodes=False, fill_edge_geometry=True).to_dict('records')
geojson = {"type": "FeatureCollection", "features": features}

# Save the GeoJSON to a file
print("Saving GeoJSON file...")
with open("munich_map.json", "w") as f:
    json.dump(geojson, f, indent=2)

print("Done!")
