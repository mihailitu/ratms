import React, { useState, useEffect } from 'react';
import { apiClient } from '../services/apiClient';
import type { SimulationRecord, ComparisonResponse, SimulationStatistics } from '../types/api';
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from 'recharts';

const COLORS = ['#3b82f6', '#22c55e', '#ef4444', '#f59e0b', '#8b5cf6', '#ec4899'];

const Analytics: React.FC = () => {
  const [simulations, setSimulations] = useState<SimulationRecord[]>([]);
  const [selectedSimIds, setSelectedSimIds] = useState<number[]>([]);
  const [metricTypes, setMetricTypes] = useState<string[]>([]);
  const [selectedMetricType, setSelectedMetricType] = useState<string>('avg_queue_length');
  const [comparisonData, setComparisonData] = useState<ComparisonResponse | null>(null);
  const [statistics, setStatistics] = useState<Map<number, SimulationStatistics>>(new Map());
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Fetch available simulations and metric types on mount
  useEffect(() => {
    const fetchData = async () => {
      try {
        const [simsResponse, typesResponse] = await Promise.all([
          apiClient.getSimulations(),
          apiClient.getMetricTypes(),
        ]);
        setSimulations(simsResponse);
        setMetricTypes(typesResponse.metric_types);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to load data');
      }
    };
    fetchData();
  }, []);

  // Handle simulation selection
  const handleSimulationToggle = (simId: number) => {
    setSelectedSimIds((prev) =>
      prev.includes(simId) ? prev.filter((id) => id !== simId) : [...prev, simId]
    );
  };

  // Handle compare button click
  const handleCompare = async () => {
    if (selectedSimIds.length === 0) {
      setError('Please select at least one simulation');
      return;
    }

    setLoading(true);
    setError(null);

    try {
      // Fetch comparison data
      const comparison = await apiClient.compareSimulations(selectedSimIds, selectedMetricType);
      setComparisonData(comparison);

      // Fetch statistics for each selected simulation
      const statsMap = new Map<number, SimulationStatistics>();
      for (const simId of selectedSimIds) {
        const stats = await apiClient.getSimulationStatistics(simId, selectedMetricType);
        statsMap.set(simId, stats);
      }
      setStatistics(statsMap);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load comparison data');
    } finally {
      setLoading(false);
    }
  };

  // Handle CSV export
  const handleExport = async (simulationId: number) => {
    try {
      const blob = await apiClient.exportMetrics(simulationId, selectedMetricType);
      const url = window.URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `simulation_${simulationId}_${selectedMetricType}_metrics.csv`;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      window.URL.revokeObjectURL(url);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Export failed');
    }
  };

  // Transform comparison data for Recharts
  const getChartData = () => {
    if (!comparisonData) return [];

    // Get all unique timestamps
    const timestamps = new Set<number>();
    comparisonData.simulations.forEach((sim) => {
      sim.metrics.forEach((metric) => timestamps.add(metric.timestamp));
    });

    // Create data points for each timestamp
    const sortedTimestamps = Array.from(timestamps).sort((a, b) => a - b);
    return sortedTimestamps.map((timestamp) => {
      const dataPoint: any = { timestamp };
      comparisonData.simulations.forEach((sim) => {
        const metric = sim.metrics.find((m) => m.timestamp === timestamp);
        dataPoint[`sim_${sim.simulation_id}`] = metric?.value ?? null;
      });
      return dataPoint;
    });
  };

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold text-gray-900">Performance Analytics</h1>
        <p className="mt-2 text-gray-600">Compare simulations and analyze performance metrics</p>
      </div>

      {/* Selection Section */}
      <div className="bg-white shadow rounded-lg p-6 space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Select Simulations
          </label>
          <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-2">
            {simulations
              .filter((sim) => sim.status === 'completed')
              .map((sim) => (
                <label key={sim.id} className="flex items-center space-x-2 cursor-pointer">
                  <input
                    type="checkbox"
                    checked={selectedSimIds.includes(sim.id)}
                    onChange={() => handleSimulationToggle(sim.id)}
                    className="rounded text-blue-600 focus:ring-blue-500"
                  />
                  <span className="text-sm text-gray-700">{sim.name || `Sim ${sim.id}`}</span>
                </label>
              ))}
          </div>
          {simulations.filter((sim) => sim.status === 'completed').length === 0 && (
            <p className="text-sm text-gray-500">No completed simulations available</p>
          )}
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">Metric Type</label>
          <select
            value={selectedMetricType}
            onChange={(e) => setSelectedMetricType(e.target.value)}
            className="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500"
          >
            {metricTypes.map((type) => (
              <option key={type} value={type}>
                {type.replace(/_/g, ' ').replace(/\b\w/g, (l) => l.toUpperCase())}
              </option>
            ))}
          </select>
        </div>

        <button
          onClick={handleCompare}
          disabled={loading || selectedSimIds.length === 0}
          className="w-full bg-blue-600 text-white px-4 py-2 rounded-md hover:bg-blue-700 disabled:bg-gray-300 disabled:cursor-not-allowed"
        >
          {loading ? 'Loading...' : 'Compare Simulations'}
        </button>
      </div>

      {/* Error Display */}
      {error && (
        <div className="bg-red-50 border border-red-200 text-red-700 px-4 py-3 rounded">
          {error}
        </div>
      )}

      {/* Comparison Chart */}
      {comparisonData && (
        <div className="bg-white shadow rounded-lg p-6">
          <h2 className="text-xl font-semibold text-gray-900 mb-4">
            Comparison Chart: {selectedMetricType.replace(/_/g, ' ').replace(/\b\w/g, (l) => l.toUpperCase())}
          </h2>
          <ResponsiveContainer width="100%" height={400}>
            <LineChart data={getChartData()}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis
                dataKey="timestamp"
                label={{ value: 'Time (s)', position: 'insideBottom', offset: -5 }}
              />
              <YAxis
                label={{ value: selectedMetricType, angle: -90, position: 'insideLeft' }}
              />
              <Tooltip />
              <Legend />
              {comparisonData.simulations.map((sim, index) => (
                <Line
                  key={sim.simulation_id}
                  type="monotone"
                  dataKey={`sim_${sim.simulation_id}`}
                  name={sim.simulation_name}
                  stroke={COLORS[index % COLORS.length]}
                  strokeWidth={2}
                  dot={false}
                />
              ))}
            </LineChart>
          </ResponsiveContainer>
        </div>
      )}

      {/* Statistics Table */}
      {statistics.size > 0 && (
        <div className="bg-white shadow rounded-lg p-6">
          <h2 className="text-xl font-semibold text-gray-900 mb-4">Statistical Summary</h2>
          <div className="overflow-x-auto">
            <table className="min-w-full divide-y divide-gray-200">
              <thead className="bg-gray-50">
                <tr>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Simulation
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Min
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Max
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Mean
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Median
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Std Dev
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    P25
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    P75
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    P95
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Samples
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Actions
                  </th>
                </tr>
              </thead>
              <tbody className="bg-white divide-y divide-gray-200">
                {Array.from(statistics.entries()).map(([simId, stats]) => {
                  const metricStats = stats.statistics[selectedMetricType];
                  if (!metricStats) return null;
                  return (
                    <tr key={simId}>
                      <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                        {stats.simulation_name}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.min_value.toFixed(2)}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.max_value.toFixed(2)}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.mean_value.toFixed(2)}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.median_value.toFixed(2)}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.stddev_value.toFixed(2)}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.p25_value.toFixed(2)}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.p75_value.toFixed(2)}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.p95_value.toFixed(2)}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {metricStats.sample_count}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        <button
                          onClick={() => handleExport(simId)}
                          className="text-blue-600 hover:text-blue-800"
                        >
                          Export CSV
                        </button>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>
      )}
    </div>
  );
};

export default Analytics;
