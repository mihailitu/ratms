import { useEffect, useState } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { apiClient } from '../services/apiClient';
import type { SimulationRecord, MetricRecord } from '../types/api';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';

export default function SimulationDetail() {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const [simulation, setSimulation] = useState<SimulationRecord | null>(null);
  const [metrics, setMetrics] = useState<MetricRecord[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    const fetchData = async () => {
      if (!id) return;

      try {
        setLoading(true);
        const [simData, metricsData] = await Promise.all([
          apiClient.getSimulation(parseInt(id)),
          apiClient.getMetrics(parseInt(id)),
        ]);
        setSimulation(simData);
        setMetrics(metricsData);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch simulation data');
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, [id]);

  const formatTimestamp = (timestamp: number) => {
    return new Date(timestamp * 1000).toLocaleString();
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'running':
        return 'bg-green-100 text-green-800';
      case 'completed':
        return 'bg-blue-100 text-blue-800';
      case 'failed':
        return 'bg-red-100 text-red-800';
      default:
        return 'bg-gray-100 text-gray-800';
    }
  };

  // Group metrics by type for charts
  const metricsByType = metrics.reduce((acc, metric) => {
    if (!acc[metric.metric_type]) {
      acc[metric.metric_type] = [];
    }
    acc[metric.metric_type].push(metric);
    return acc;
  }, {} as Record<string, MetricRecord[]>);

  if (loading) {
    return (
      <div className="flex items-center justify-center h-screen">
        <div className="text-xl">Loading simulation details...</div>
      </div>
    );
  }

  if (error || !simulation) {
    return (
      <div className="flex items-center justify-center h-screen">
        <div className="text-center">
          <div className="text-xl text-red-500 mb-4">Error: {error || 'Simulation not found'}</div>
          <button
            onClick={() => navigate('/simulations')}
            className="px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700"
          >
            Back to Simulations
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="max-w-7xl mx-auto px-4 py-8">
        <div className="mb-6">
          <button
            onClick={() => navigate('/simulations')}
            className="text-blue-600 hover:text-blue-800 mb-4"
          >
            ← Back to Simulations
          </button>
          <h1 className="text-4xl font-bold text-gray-900 mb-2">
            Simulation #{simulation.id}: {simulation.name}
          </h1>
          <p className="text-gray-600">{simulation.description}</p>
        </div>

        {/* Simulation Info */}
        <div className="bg-white rounded-lg shadow p-6 mb-6">
          <h2 className="text-xl font-semibold text-gray-900 mb-4">Simulation Details</h2>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
            <div>
              <div className="text-sm text-gray-500">Status</div>
              <span className={`mt-1 px-2 inline-flex text-xs leading-5 font-semibold rounded-full ${getStatusColor(simulation.status)}`}>
                {simulation.status}
              </span>
            </div>
            <div>
              <div className="text-sm text-gray-500">Network ID</div>
              <div className="text-lg font-medium text-gray-900">{simulation.network_id}</div>
            </div>
            <div>
              <div className="text-sm text-gray-500">Started</div>
              <div className="text-lg font-medium text-gray-900">
                {formatTimestamp(simulation.start_time)}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-500">Duration</div>
              <div className="text-lg font-medium text-gray-900">
                {simulation.duration_seconds > 0
                  ? `${Math.floor(simulation.duration_seconds / 60)}m ${Math.floor(simulation.duration_seconds % 60)}s`
                  : 'N/A'}
              </div>
            </div>
          </div>
        </div>

        {/* Metrics Charts */}
        <div className="space-y-6">
          {metrics.length === 0 ? (
            <div className="bg-white rounded-lg shadow p-8 text-center text-gray-500">
              No metrics data available for this simulation
            </div>
          ) : (
            Object.entries(metricsByType).map(([type, data]) => (
              <div key={type} className="bg-white rounded-lg shadow p-6">
                <h3 className="text-lg font-semibold text-gray-900 mb-4">
                  {type.charAt(0).toUpperCase() + type.slice(1)} Over Time
                </h3>
                <ResponsiveContainer width="100%" height={300}>
                  <LineChart data={data}>
                    <CartesianGrid strokeDasharray="3 3" />
                    <XAxis
                      dataKey="timestamp"
                      label={{ value: 'Time (s)', position: 'insideBottom', offset: -5 }}
                    />
                    <YAxis label={{ value: data[0]?.unit || 'Value', angle: -90, position: 'insideLeft' }} />
                    <Tooltip />
                    <Legend />
                    <Line type="monotone" dataKey="value" stroke="#3b82f6" name={type} />
                  </LineChart>
                </ResponsiveContainer>
              </div>
            ))
          )}
        </div>
      </div>
    </div>
  );
}
