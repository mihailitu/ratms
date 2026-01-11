import { useState, useEffect, useCallback } from 'react';
import { apiClient } from '../services/apiClient';
import type { PredictionResult, PredictedMetrics } from '../types/api';
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';

interface ForecastComparisonProps {
  horizonMinutes?: number;
}

interface ComparisonData {
  roadId: number;
  currentVehicles: number;
  predictedVehicles: number;
  currentQueue: number;
  predictedQueue: number;
  currentSpeed: number;
  predictedSpeed: number;
  confidence: number;
}

export default function ForecastComparison({ horizonMinutes = 30 }: ForecastComparisonProps) {
  const [prediction, setPrediction] = useState<PredictionResult | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [selectedHorizon, setSelectedHorizon] = useState(horizonMinutes);
  const [viewMode, setViewMode] = useState<'chart' | 'table'>('chart');

  const fetchPrediction = useCallback(async () => {
    try {
      setError(null);
      const response = await apiClient.getPredictionForecast(selectedHorizon);
      if (response.success && response.data) {
        setPrediction(response.data);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch prediction');
    } finally {
      setLoading(false);
    }
  }, [selectedHorizon]);

  useEffect(() => {
    fetchPrediction();
    const interval = setInterval(fetchPrediction, 10000);
    return () => clearInterval(interval);
  }, [fetchPrediction]);

  const getComparisonData = (): ComparisonData[] => {
    if (!prediction?.roadPredictions) return [];

    return prediction.roadPredictions
      .filter((road: PredictedMetrics) => road.hasCurrentData || road.hasHistoricalPattern)
      .slice(0, 10)
      .map((road: PredictedMetrics) => ({
        roadId: road.roadId,
        currentVehicles: road.hasCurrentData ? Math.round(road.vehicleCount / (1 - 0.3)) : 0, // Reverse blend estimate
        predictedVehicles: Math.round(road.vehicleCount),
        currentQueue: road.hasCurrentData ? road.queueLength * 0.9 : 0, // Estimate
        predictedQueue: road.queueLength,
        currentSpeed: road.hasCurrentData ? road.avgSpeed * 1.1 : 0, // Estimate
        predictedSpeed: road.avgSpeed,
        confidence: road.confidence,
      }));
  };

  const getConfidenceColor = (confidence: number) => {
    if (confidence >= 0.7) return 'text-green-600';
    if (confidence >= 0.4) return 'text-yellow-600';
    return 'text-red-600';
  };

  const getConfidenceBg = (confidence: number) => {
    if (confidence >= 0.7) return 'bg-green-100';
    if (confidence >= 0.4) return 'bg-yellow-100';
    return 'bg-red-100';
  };

  if (loading) {
    return (
      <div className="bg-white rounded-lg shadow p-4">
        <div className="animate-pulse space-y-3">
          <div className="h-5 bg-gray-200 rounded w-1/3"></div>
          <div className="h-48 bg-gray-200 rounded"></div>
        </div>
      </div>
    );
  }

  const comparisonData = getComparisonData();

  return (
    <div className="bg-white rounded-lg shadow p-4">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-semibold text-gray-800">Traffic Forecast</h3>
        <div className="flex items-center gap-3">
          <select
            value={selectedHorizon}
            onChange={(e) => setSelectedHorizon(parseInt(e.target.value))}
            className="text-sm border border-gray-300 rounded px-2 py-1"
          >
            <option value={10}>10 min</option>
            <option value={30}>30 min</option>
            <option value={60}>1 hour</option>
            <option value={120}>2 hours</option>
          </select>
          <div className="flex border border-gray-300 rounded overflow-hidden">
            <button
              onClick={() => setViewMode('chart')}
              className={`px-3 py-1 text-sm ${viewMode === 'chart' ? 'bg-blue-600 text-white' : 'bg-white text-gray-600'}`}
            >
              Chart
            </button>
            <button
              onClick={() => setViewMode('table')}
              className={`px-3 py-1 text-sm ${viewMode === 'table' ? 'bg-blue-600 text-white' : 'bg-white text-gray-600'}`}
            >
              Table
            </button>
          </div>
        </div>
      </div>

      {error && (
        <div className="mb-3 text-sm text-red-600 bg-red-50 p-2 rounded">{error}</div>
      )}

      {prediction && (
        <div className="mb-4 flex items-center gap-4 text-sm">
          <span className="text-gray-500">
            Target: <span className="font-medium">{prediction.targetTimeSlotString}</span>
          </span>
          <span className="text-gray-500">
            Avg Confidence:{' '}
            <span className={`font-medium ${getConfidenceColor(prediction.averageConfidence)}`}>
              {(prediction.averageConfidence * 100).toFixed(0)}%
            </span>
          </span>
        </div>
      )}

      {comparisonData.length === 0 ? (
        <div className="h-48 flex items-center justify-center text-gray-500 text-sm">
          No prediction data available. Run the simulation to collect traffic patterns.
        </div>
      ) : viewMode === 'chart' ? (
        <ResponsiveContainer width="100%" height={250}>
          <BarChart data={comparisonData} margin={{ top: 5, right: 20, left: 0, bottom: 5 }}>
            <CartesianGrid strokeDasharray="3 3" stroke="#e5e7eb" />
            <XAxis
              dataKey="roadId"
              tick={{ fontSize: 11, fill: '#6b7280' }}
              tickFormatter={(value) => `R${value}`}
            />
            <YAxis tick={{ fontSize: 11, fill: '#6b7280' }} />
            <Tooltip
              contentStyle={{
                backgroundColor: 'white',
                border: '1px solid #e5e7eb',
                borderRadius: '0.375rem',
                fontSize: '12px',
              }}
              formatter={(value: number, name: string) => [
                value.toFixed(1),
                name.replace('predicted', 'Predicted ').replace('current', 'Current '),
              ]}
            />
            <Legend wrapperStyle={{ fontSize: '11px' }} />
            <Bar dataKey="currentVehicles" name="Current" fill="#94a3b8" />
            <Bar dataKey="predictedVehicles" name="Predicted" fill="#3b82f6" />
          </BarChart>
        </ResponsiveContainer>
      ) : (
        <div className="overflow-x-auto">
          <table className="min-w-full divide-y divide-gray-200">
            <thead className="bg-gray-50">
              <tr>
                <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Road</th>
                <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Vehicles</th>
                <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Queue</th>
                <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Speed</th>
                <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Confidence</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-gray-200">
              {comparisonData.map((row) => (
                <tr key={row.roadId}>
                  <td className="px-3 py-2 text-sm text-gray-900">Road {row.roadId}</td>
                  <td className="px-3 py-2 text-sm text-gray-700">{row.predictedVehicles}</td>
                  <td className="px-3 py-2 text-sm text-gray-700">{row.predictedQueue.toFixed(1)}</td>
                  <td className="px-3 py-2 text-sm text-gray-700">{row.predictedSpeed.toFixed(1)} m/s</td>
                  <td className="px-3 py-2 text-sm">
                    <span className={`px-2 py-0.5 rounded ${getConfidenceBg(row.confidence)} ${getConfidenceColor(row.confidence)}`}>
                      {(row.confidence * 100).toFixed(0)}%
                    </span>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
