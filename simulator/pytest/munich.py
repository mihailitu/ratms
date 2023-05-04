import osmnx as ox
import json
from shapely.geometry import mapping
from tqdm import tqdm

# Define the city
city = "Segarcea, Romania"

# Retrieve the street network for the city
print("Retrieving street network...")
G = ox.graph_from_place(city, network_type='all')

# Convert the street network to a GeoJSON FeatureCollection
print("Converting street network to GeoJSON...")
features = ox.graph_to_gdfs(G, nodes=False, fill_edge_geometry=True)
features['geometry'] = features['geometry'].apply(lambda x: mapping(x))
features = features.to_dict('records')
geojson = {"type": "FeatureCollection", "features": features}

# Save the GeoJSON to a file
print("Saving GeoJSON file...")
with open("munich_map.json", "w") as f:
    for chunk in tqdm(json.dumps(geojson, default=lambda o: o.__dict__, indent=2), 
                      total=len(json.dumps(geojson, default=lambda o: o.__dict__, indent=2)), 
                      desc='Saving GeoJSON'):
        f.write(chunk)

print("Done!")
