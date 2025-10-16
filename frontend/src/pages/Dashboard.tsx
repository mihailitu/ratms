import { useEffect, useState } from 'react';
import { apiClient } from '../services/apiClient';
import type { SimulationStatus, NetworkRecord } from '../types/api';
import SimulationControl from '../components/SimulationControl';

export default function Dashboard() {
  const [status, setStatus] = useState<SimulationStatus | null>(null);
  const [networks, setNetworks] = useState<NetworkRecord[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchData = async () => {
    try {
      setLoading(true);
      setError(null);
      const [statusData, networksData] = await Promise.all([
        apiClient.getSimulationStatus(),
        apiClient.getNetworks(),
      ]);
      setStatus(statusData);
      setNetworks(networksData);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch data');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchData();
    const interval = setInterval(fetchData, 2000); // Poll every 2 seconds
    return () => clearInterval(interval);
  }, []);

  if (loading && !status) {
    return (
      <div className="flex items-center justify-center h-screen">
        <div className="text-xl">Loading...</div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="flex items-center justify-center h-screen">
        <div className="text-xl text-red-500">Error: {error}</div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="max-w-7xl mx-auto px-4 py-8">
        {/* Header */}
        <div className="mb-8">
          <h1 className="text-4xl font-bold text-gray-900 mb-2">RATMS Dashboard</h1>
          <p className="text-gray-600">Real-time Adaptive Traffic Management System</p>
        </div>

        {/* Simulation Control */}
        <div className="mb-8">
          <SimulationControl status={status} onStatusChange={fetchData} />
        </div>

        {/* Metrics Overview */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-8">
          <div className="bg-white rounded-lg shadow p-6">
            <div className="text-sm font-medium text-gray-500 mb-1">Simulation Status</div>
            <div className="text-2xl font-bold">
              <span className={status?.status === 'running' ? 'text-green-600' : 'text-gray-600'}>
                {status?.status === 'running' ? 'Running' : 'Stopped'}
              </span>
            </div>
          </div>

          <div className="bg-white rounded-lg shadow p-6">
            <div className="text-sm font-medium text-gray-500 mb-1">Road Count</div>
            <div className="text-2xl font-bold text-gray-900">
              {status?.road_count || 0}
            </div>
          </div>

          <div className="bg-white rounded-lg shadow p-6">
            <div className="text-sm font-medium text-gray-500 mb-1">Server Status</div>
            <div className="text-2xl font-bold">
              <span className={status?.server_running ? 'text-green-600' : 'text-red-600'}>
                {status?.server_running ? 'Online' : 'Offline'}
              </span>
            </div>
          </div>
        </div>

        {/* Networks */}
        <div className="bg-white rounded-lg shadow">
          <div className="px-6 py-4 border-b border-gray-200">
            <h2 className="text-xl font-semibold text-gray-900">Networks</h2>
          </div>
          <div className="p-6">
            {networks.length === 0 ? (
              <p className="text-gray-500">No networks configured</p>
            ) : (
              <div className="space-y-4">
                {networks.map((network) => (
                  <div key={network.id} className="border border-gray-200 rounded-lg p-4">
                    <div className="flex justify-between items-start">
                      <div>
                        <h3 className="font-semibold text-gray-900">{network.name}</h3>
                        <p className="text-sm text-gray-600 mt-1">{network.description}</p>
                      </div>
                      <div className="text-right">
                        <div className="text-sm text-gray-500">
                          {network.road_count} roads, {network.intersection_count} intersections
                        </div>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
