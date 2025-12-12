import { useEffect, useState, useRef, useMemo } from 'react';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import { apiClient } from '../services/apiClient';
import type { NetworkRecord, RoadGeometry, MapViewMode } from '../types/api';
import { useSimulationStream } from '../hooks/useSimulationStream';
import TrafficLightPanel from '../components/TrafficLightPanel';
import ProfilePanel from '../components/ProfilePanel';
import TravelTimePanel from '../components/TravelTimePanel';

export default function MapView() {
  const mapRef = useRef<L.Map | null>(null);
  const mapContainerRef = useRef<HTMLDivElement>(null);
  const [networks, setNetworks] = useState<NetworkRecord[]>([]);
  const [selectedNetwork, setSelectedNetwork] = useState<number | null>(null);
  const [roads, setRoads] = useState<RoadGeometry[]>([]);
  const vehicleMarkersRef = useRef<Map<number, L.CircleMarker>>(new Map());
  const roadPolylinesRef = useRef<Map<number, L.Polyline>>(new Map());
  const trafficLightMarkersRef = useRef<Map<string, L.CircleMarker>>(new Map());
  const [viewMode, setViewMode] = useState<MapViewMode>('speed');
  const [trafficLightPanelOpen, setTrafficLightPanelOpen] = useState(false);
  const [profilePanelOpen, setProfilePanelOpen] = useState(false);
  const [travelTimePanelOpen, setTravelTimePanelOpen] = useState(false);
  const [lastUpdateTime, setLastUpdateTime] = useState<Date | null>(null);

  // Auto-connect to stream (always enabled in production mode)
  const { latestUpdate, isConnected, error: streamError } = useSimulationStream(true);

  // Update last update time when we receive data
  useEffect(() => {
    if (latestUpdate) {
      setLastUpdateTime(new Date());
    }
  }, [latestUpdate]);

  // Calculate density per road (vehicles per km)
  const roadDensities = useMemo(() => {
    const densities = new Map<number, { count: number; density: number }>();
    if (!latestUpdate) return densities;

    // Count vehicles per road
    const counts = new Map<number, number>();
    latestUpdate.vehicles.forEach(v => {
      counts.set(v.roadId, (counts.get(v.roadId) || 0) + 1);
    });

    // Calculate density using road length
    counts.forEach((count, roadId) => {
      const road = roads.find(r => r.id === roadId);
      const lengthKm = road ? road.length / 1000 : 1;
      densities.set(roadId, {
        count,
        density: count / lengthKm
      });
    });

    return densities;
  }, [latestUpdate, roads]);

  // Get color for density-based visualization (vehicles per km)
  const getDensityColor = (density: number): string => {
    if (density < 20) return '#22c55e';   // Green: free flow
    if (density < 50) return '#eab308';   // Yellow: moderate
    if (density < 100) return '#f97316';  // Orange: heavy
    return '#ef4444';                      // Red: congestion
  };

  // Get color for speed-based visualization
  const getSpeedColor = (velocity: number, maxSpeed: number = 20): string => {
    const speedRatio = Math.min(velocity / maxSpeed, 1.0);
    if (speedRatio > 0.7) return '#22c55e';  // Green: fast
    if (speedRatio > 0.3) return '#3b82f6';  // Blue: medium
    return '#ef4444';                         // Red: slow
  };

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
          weight: 4,
          opacity: 0.8,
        }
      ).addTo(map);

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

  // Update road colors based on view mode and density
  useEffect(() => {
    if (roads.length === 0) return;

    roads.forEach((road) => {
      const polyline = roadPolylinesRef.current.get(road.id);
      if (!polyline) return;

      let color = '#3b82f6'; // Default blue
      let weight = 4;

      if (viewMode === 'density') {
        const densityData = roadDensities.get(road.id);
        if (densityData) {
          color = getDensityColor(densityData.density);
          // Thicker lines for roads with more vehicles
          weight = Math.min(4 + densityData.count * 0.5, 10);
        } else {
          color = '#22c55e'; // Green for empty roads
        }
      }

      polyline.setStyle({ color, weight });

      // Update popup with density info
      const densityData = roadDensities.get(road.id);
      const densityInfo = densityData
        ? `<br/>Vehicles: ${densityData.count}<br/>Density: ${densityData.density.toFixed(1)} veh/km`
        : '<br/>Vehicles: 0';

      polyline.bindPopup(`
        <div>
          <strong>Road ${road.id}</strong><br/>
          Length: ${road.length.toFixed(0)}m<br/>
          Lanes: ${road.lanes}<br/>
          Max Speed: ${road.maxSpeed} m/s${densityInfo}
        </div>
      `);
    });
  }, [roads, viewMode, roadDensities]);

  useEffect(() => {
    if (!mapContainerRef.current || mapRef.current) return;

    // Initialize map centered on a default location
    const map = L.map(mapContainerRef.current).setView([48.1351, 11.582], 13);

    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
    }).addTo(map);

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
        const color = getSpeedColor(vehicle.velocity, road?.maxSpeed);

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
            Accel: ${vehicle.acceleration.toFixed(2)} m/s2
          </div>`;
        });

        vehicleMarkersRef.current.set(vehicle.id, marker);
      } else {
        // Update existing marker position and color
        marker.setLatLng([lat, lng]);

        // Update color based on speed
        const color = getSpeedColor(vehicle.velocity, road?.maxSpeed);
        marker.setStyle({ fillColor: color });
      }
    });
  }, [latestUpdate, roads]);

  // Update traffic light markers when receiving simulation updates
  useEffect(() => {
    if (!mapRef.current || !latestUpdate) return;

    const map = mapRef.current;

    // Create a unique key for each traffic light (roadId + lane)
    const currentTrafficLightKeys = new Set(
      latestUpdate.trafficLights.map(tl => `${tl.roadId}-${tl.lane}`)
    );

    // Remove traffic lights that no longer exist
    trafficLightMarkersRef.current.forEach((marker, key) => {
      if (!currentTrafficLightKeys.has(key)) {
        map.removeLayer(marker);
        trafficLightMarkersRef.current.delete(key);
      }
    });

    // Add or update traffic light markers
    latestUpdate.trafficLights.forEach((trafficLight) => {
      const key = `${trafficLight.roadId}-${trafficLight.lane}`;
      const { lat, lon, state } = trafficLight;

      // Determine color based on state
      const stateColors: Record<string, string> = {
        'R': '#ef4444', // Red
        'Y': '#eab308', // Yellow
        'G': '#22c55e'  // Green
      };
      const color = stateColors[state] || '#94a3b8'; // Gray fallback

      let marker = trafficLightMarkersRef.current.get(key);

      if (!marker) {
        // Create new traffic light marker (square shape)
        marker = L.circleMarker([lat, lon], {
          radius: 6,
          fillColor: color,
          color: '#000000',
          weight: 2,
          opacity: 1,
          fillOpacity: 1,
        }).addTo(map);

        marker.bindPopup(() => {
          const stateNames: Record<string, string> = {
            'R': 'Red',
            'Y': 'Yellow',
            'G': 'Green'
          };
          return `<div>
            <strong>Traffic Light</strong><br/>
            Road: ${trafficLight.roadId}<br/>
            Lane: ${trafficLight.lane}<br/>
            State: <span style="color: ${color}; font-weight: bold;">${stateNames[state] || state}</span>
          </div>`;
        });

        trafficLightMarkersRef.current.set(key, marker);
      } else {
        // Update existing marker color based on state
        marker.setStyle({ fillColor: color });
      }
    });
  }, [latestUpdate]);

  const selectedNetworkData = networks.find((n) => n.id === selectedNetwork);

  // Format time since last update
  const getLastUpdateText = () => {
    if (!lastUpdateTime) return 'Waiting for data...';
    const seconds = Math.floor((new Date().getTime() - lastUpdateTime.getTime()) / 1000);
    if (seconds < 2) return 'Just now';
    if (seconds < 60) return `${seconds}s ago`;
    return `${Math.floor(seconds / 60)}m ago`;
  };

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="max-w-7xl mx-auto px-4 py-8">
        <div className="mb-6">
          <div className="flex justify-between items-center mb-2">
            <div>
              <h1 className="text-4xl font-bold text-gray-900">Traffic Map</h1>
              <p className="text-gray-600">Real-time traffic visualization</p>
            </div>
            <div className="flex items-center gap-4">
              {/* Live Status */}
              <div className="flex items-center gap-2 px-3 py-2 bg-white rounded-lg shadow">
                <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-green-500 animate-pulse' : 'bg-red-500'}`} />
                <span className="text-sm font-medium text-gray-700">
                  {isConnected ? 'Live' : 'Disconnected'}
                </span>
              </div>

              {/* View Mode Toggle */}
              <div className="flex items-center gap-1 bg-white rounded-lg shadow p-1">
                <button
                  onClick={() => setViewMode('speed')}
                  className={`px-3 py-1.5 rounded text-sm font-medium transition-colors ${
                    viewMode === 'speed'
                      ? 'bg-blue-600 text-white'
                      : 'text-gray-600 hover:bg-gray-100'
                  }`}
                >
                  Speed
                </button>
                <button
                  onClick={() => setViewMode('density')}
                  className={`px-3 py-1.5 rounded text-sm font-medium transition-colors ${
                    viewMode === 'density'
                      ? 'bg-blue-600 text-white'
                      : 'text-gray-600 hover:bg-gray-100'
                  }`}
                >
                  Density
                </button>
              </div>

              {/* Panel Toggle Buttons */}
              <button
                onClick={() => setTrafficLightPanelOpen(!trafficLightPanelOpen)}
                className={`px-3 py-2 rounded-lg font-medium text-sm transition-colors shadow ${
                  trafficLightPanelOpen
                    ? 'bg-amber-600 text-white'
                    : 'bg-white text-gray-700 hover:bg-gray-50'
                }`}
              >
                Traffic Lights
              </button>
              <button
                onClick={() => setProfilePanelOpen(!profilePanelOpen)}
                className={`px-3 py-2 rounded-lg font-medium text-sm transition-colors shadow ${
                  profilePanelOpen
                    ? 'bg-indigo-600 text-white'
                    : 'bg-white text-gray-700 hover:bg-gray-50'
                }`}
              >
                Profiles
              </button>
              <button
                onClick={() => setTravelTimePanelOpen(!travelTimePanelOpen)}
                className={`px-3 py-2 rounded-lg font-medium text-sm transition-colors shadow ${
                  travelTimePanelOpen
                    ? 'bg-teal-600 text-white'
                    : 'bg-white text-gray-700 hover:bg-gray-50'
                }`}
              >
                Travel Times
              </button>
            </div>
          </div>

          {/* Stream Info */}
          {latestUpdate && (
            <div className="mt-2 flex items-center gap-4 text-sm text-gray-600">
              <span>Last update: {getLastUpdateText()}</span>
              <span>|</span>
              <span>Vehicles: {latestUpdate.vehicles.length.toLocaleString()}</span>
              <span>|</span>
              <span>Traffic Lights: {latestUpdate.trafficLights.length.toLocaleString()}</span>
            </div>
          )}

          {streamError && (
            <div className="mt-2 text-sm text-red-600">
              Connection error: {streamError}
            </div>
          )}
        </div>

        {/* Network Selector */}
        <div className="bg-white rounded-lg shadow p-4 mb-6">
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Network
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

        {/* Legend */}
        <div className="mt-4 bg-white border border-gray-200 rounded-lg p-4">
          <div className="text-sm text-gray-800">
            <strong>Legend ({viewMode === 'speed' ? 'Speed Mode' : 'Density Mode'}):</strong>
            <div className="mt-2 grid grid-cols-2 md:grid-cols-4 gap-2">
              {viewMode === 'speed' ? (
                <>
                  <div className="flex items-center gap-2">
                    <span className="inline-block w-3 h-3 rounded-full bg-green-500"></span>
                    <span>Fast (&gt;70% speed)</span>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className="inline-block w-3 h-3 rounded-full bg-blue-500"></span>
                    <span>Medium (30-70%)</span>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className="inline-block w-3 h-3 rounded-full bg-red-500"></span>
                    <span>Slow (&lt;30%)</span>
                  </div>
                </>
              ) : (
                <>
                  <div className="flex items-center gap-2">
                    <span className="inline-block w-4 h-2 rounded bg-green-500"></span>
                    <span>Free flow (&lt;20/km)</span>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className="inline-block w-4 h-2 rounded bg-yellow-500"></span>
                    <span>Moderate (20-50/km)</span>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className="inline-block w-4 h-2 rounded bg-orange-500"></span>
                    <span>Heavy (50-100/km)</span>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className="inline-block w-4 h-2 rounded bg-red-500"></span>
                    <span>Congestion (&gt;100/km)</span>
                  </div>
                </>
              )}
            </div>
            <div className="mt-3 grid grid-cols-3 gap-2">
              <div className="flex items-center gap-2">
                <span className="inline-block w-3 h-3 rounded-full bg-green-500 border-2 border-black"></span>
                <span>Green Light</span>
              </div>
              <div className="flex items-center gap-2">
                <span className="inline-block w-3 h-3 rounded-full bg-yellow-500 border-2 border-black"></span>
                <span>Yellow Light</span>
              </div>
              <div className="flex items-center gap-2">
                <span className="inline-block w-3 h-3 rounded-full bg-red-500 border-2 border-black"></span>
                <span>Red Light</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Control Panels */}
      <TrafficLightPanel
        isOpen={trafficLightPanelOpen}
        onToggle={() => setTrafficLightPanelOpen(!trafficLightPanelOpen)}
      />
      <ProfilePanel
        isOpen={profilePanelOpen}
        onToggle={() => setProfilePanelOpen(!profilePanelOpen)}
      />
      <TravelTimePanel
        isOpen={travelTimePanelOpen}
        onToggle={() => setTravelTimePanelOpen(!travelTimePanelOpen)}
        roads={roads}
      />
    </div>
  );
}
