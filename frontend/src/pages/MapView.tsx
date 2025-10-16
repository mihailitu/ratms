import { useEffect, useState, useRef } from 'react';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import { apiClient } from '../services/apiClient';
import type { NetworkRecord } from '../types/api';

export default function MapView() {
  const mapRef = useRef<L.Map | null>(null);
  const mapContainerRef = useRef<HTMLDivElement>(null);
  const [networks, setNetworks] = useState<NetworkRecord[]>([]);
  const [selectedNetwork, setSelectedNetwork] = useState<number | null>(null);
  const [loading, setLoading] = useState(true);

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
      } finally {
        setLoading(false);
      }
    };

    fetchNetworks();
  }, []);

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

  const selectedNetworkData = networks.find((n) => n.id === selectedNetwork);

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="max-w-7xl mx-auto px-4 py-8">
        <div className="mb-6">
          <h1 className="text-4xl font-bold text-gray-900 mb-2">Network Map View</h1>
          <p className="text-gray-600">Visualize road networks and traffic flow</p>
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
