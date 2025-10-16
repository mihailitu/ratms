import { useEffect, useState, useRef } from 'react';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import { apiClient } from '../services/apiClient';
import type { NetworkRecord, RoadGeometry } from '../types/api';
import { useSimulationStream } from '../hooks/useSimulationStream';

export default function MapView() {
  const mapRef = useRef<L.Map | null>(null);
  const mapContainerRef = useRef<HTMLDivElement>(null);
  const [networks, setNetworks] = useState<NetworkRecord[]>([]);
  const [selectedNetwork, setSelectedNetwork] = useState<number | null>(null);
  const [streamEnabled, setStreamEnabled] = useState(false);
  const [roads, setRoads] = useState<RoadGeometry[]>([]);
  const vehicleMarkersRef = useRef<Map<number, L.CircleMarker>>(new Map());
  const roadPolylinesRef = useRef<Map<number, L.Polyline>>(new Map());

  // Subscribe to simulation stream
  const { latestUpdate, isConnected, error: streamError } = useSimulationStream(streamEnabled);

  useEffect(() => {
    const fetchNetworks = async () => {
      try {
        const data = await apiClient.getNetworks();
        setNetworks(data);
        if (data.length > 0) {
          setSelectedNetwork(data[0].id);
        }
      } catch (err) {
        console.error('Failed to fetch networks:', err);
      }
    };

    const fetchRoads = async () => {
      try {
        const response = await apiClient.getRoads();
        setRoads(response.roads);
      } catch (err) {
        console.error('Failed to fetch roads:', err);
      }
    };

    fetchNetworks();
    fetchRoads();
  }, []);

  // Render roads as polylines on map
  useEffect(() => {
    if (!mapRef.current || roads.length === 0) return;

    const map = mapRef.current;

    // Remove old polylines
    roadPolylinesRef.current.forEach((polyline) => {
      map.removeLayer(polyline);
    });
    roadPolylinesRef.current.clear();

    // Add new polylines for each road
    roads.forEach((road) => {
      const polyline = L.polyline(
        [[road.startLat, road.startLon], [road.endLat, road.endLon]],
        {
          color: '#3b82f6',
          weight: 3,
          opacity: 0.7,
        }
      ).addTo(map);

      polyline.bindPopup(`
        <div>
          <strong>Road ${road.id}</strong><br/>
          Length: ${road.length}m<br/>
          Lanes: ${road.lanes}<br/>
          Max Speed: ${road.maxSpeed} m/s
        </div>
      `);

      roadPolylinesRef.current.set(road.id, polyline);
    });

    // Fit map to show all roads
    if (roads.length > 0) {
      const bounds = L.latLngBounds(
        roads.map(r => [r.startLat, r.startLon] as [number, number])
      );
      roads.forEach(r => bounds.extend([r.endLat, r.endLon]));
      map.fitBounds(bounds, { padding: [50, 50] });
    }
  }, [roads]);

  useEffect(() => {
    if (!mapContainerRef.current || mapRef.current) return;

    // Initialize map centered on a default location
    const map = L.map(mapContainerRef.current).setView([48.1351, 11.582], 13);

    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
    }).addTo(map);

    // Add a sample marker (replace with actual road network data)
    L.marker([48.1351, 11.582])
      .addTo(map)
      .bindPopup('Test Intersection')
      .openPopup();

    mapRef.current = map;

    return () => {
      if (mapRef.current) {
        mapRef.current.remove();
        mapRef.current = null;
      }
    };
  }, []);

  // Update vehicle markers when receiving simulation updates
  useEffect(() => {
    if (!mapRef.current || !latestUpdate) return;

    const map = mapRef.current;
    const currentVehicleIds = new Set(latestUpdate.vehicles.map(v => v.id));

    // Remove vehicles that no longer exist
    vehicleMarkersRef.current.forEach((marker, vehicleId) => {
      if (!currentVehicleIds.has(vehicleId)) {
        map.removeLayer(marker);
        vehicleMarkersRef.current.delete(vehicleId);
      }
    });

    // Add or update vehicle markers
    latestUpdate.vehicles.forEach((vehicle) => {
      // Find the road this vehicle is on
      const road = roads.find(r => r.id === vehicle.roadId);

      let lat, lng;
      if (road) {
        // Interpolate position along the road
        const ratio = Math.min(vehicle.position / road.length, 1.0);
        lat = road.startLat + (road.endLat - road.startLat) * ratio;
        lng = road.startLon + (road.endLon - road.startLon) * ratio;

        // Offset by lane (perpendicular to road direction)
        if (vehicle.lane > 0) {
          const perpOffset = 0.00001 * vehicle.lane; // Small offset per lane
          lat += perpOffset;
        }
      } else {
        // Fallback to grid placement if road not found
        lat = 48.1351 + (vehicle.roadId * 0.001);
        lng = 11.582 + (vehicle.position * 0.0001);
      }

      let marker = vehicleMarkersRef.current.get(vehicle.id);

      if (!marker) {
        // Create new marker with color based on speed
        const speedRatio = Math.min(vehicle.velocity / 20, 1.0); // Normalize to 20 m/s
        const color = speedRatio > 0.7 ? '#22c55e' : speedRatio > 0.3 ? '#3b82f6' : '#ef4444';

        marker = L.circleMarker([lat, lng], {
          radius: 5,
          fillColor: color,
          color: '#ffffff',
          weight: 1,
          opacity: 1,
          fillOpacity: 0.9,
        }).addTo(map);

        marker.bindPopup(() => {
          return `<div>
            <strong>Vehicle ${vehicle.id}</strong><br/>
            Road: ${vehicle.roadId}<br/>
            Lane: ${vehicle.lane}<br/>
            Position: ${vehicle.position.toFixed(1)}m<br/>
            Speed: ${vehicle.velocity.toFixed(1)} m/s<br/>
            Accel: ${vehicle.acceleration.toFixed(2)} m/s²
          </div>`;
        });

        vehicleMarkersRef.current.set(vehicle.id, marker);
      } else {
        // Update existing marker position and color
        marker.setLatLng([lat, lng]);

        // Update color based on speed
        const speedRatio = Math.min(vehicle.velocity / 20, 1.0);
        const color = speedRatio > 0.7 ? '#22c55e' : speedRatio > 0.3 ? '#3b82f6' : '#ef4444';
        marker.setStyle({ fillColor: color });
      }
    });
  }, [latestUpdate, roads]);

  const selectedNetworkData = networks.find((n) => n.id === selectedNetwork);

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="max-w-7xl mx-auto px-4 py-8">
        <div className="mb-6">
          <div className="flex justify-between items-center mb-2">
            <div>
              <h1 className="text-4xl font-bold text-gray-900">Network Map View</h1>
              <p className="text-gray-600">Visualize road networks and traffic flow</p>
            </div>
            <div className="flex items-center gap-4">
              {/* Connection Status */}
              <div className="flex items-center gap-2">
                <div className={`w-3 h-3 rounded-full ${isConnected ? 'bg-green-500' : 'bg-gray-300'}`} />
                <span className="text-sm text-gray-600">
                  {isConnected ? 'Live' : 'Disconnected'}
                </span>
              </div>

              {/* Stream Control */}
              <button
                onClick={() => setStreamEnabled(!streamEnabled)}
                className={`px-4 py-2 rounded-md font-medium transition-colors ${
                  streamEnabled
                    ? 'bg-red-600 hover:bg-red-700 text-white'
                    : 'bg-green-600 hover:bg-green-700 text-white'
                }`}
              >
                {streamEnabled ? 'Stop Streaming' : 'Start Streaming'}
              </button>
            </div>
          </div>

          {/* Stream Info */}
          {latestUpdate && (
            <div className="mt-2 text-sm text-gray-600">
              Step: {latestUpdate.step} | Time: {latestUpdate.time.toFixed(1)}s |
              Vehicles: {latestUpdate.vehicles.length} |
              Traffic Lights: {latestUpdate.trafficLights.length}
            </div>
          )}

          {streamError && (
            <div className="mt-2 text-sm text-red-600">
              Stream error: {streamError}
            </div>
          )}
        </div>

        {/* Network Selector */}
        <div className="bg-white rounded-lg shadow p-4 mb-6">
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Select Network
          </label>
          <select
            value={selectedNetwork || ''}
            onChange={(e) => setSelectedNetwork(parseInt(e.target.value))}
            className="block w-full px-3 py-2 bg-white border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
          >
            {networks.map((network) => (
              <option key={network.id} value={network.id}>
                {network.name} ({network.road_count} roads, {network.intersection_count} intersections)
              </option>
            ))}
          </select>
          {selectedNetworkData && (
            <p className="mt-2 text-sm text-gray-600">{selectedNetworkData.description}</p>
          )}
        </div>

        {/* Map Container */}
        <div className="bg-white rounded-lg shadow overflow-hidden">
          <div
            ref={mapContainerRef}
            className="w-full h-[600px]"
          />
        </div>

        <div className="mt-4 bg-yellow-50 border border-yellow-200 rounded-lg p-4">
          <p className="text-sm text-yellow-800">
            <strong>Note:</strong> Map visualization will be enhanced in future iterations with real-time
            traffic data, vehicle positions, and traffic light states from the simulation.
          </p>
        </div>
      </div>
    </div>
  );
}
