import { useState } from 'react';
import { apiClient } from '../services/apiClient';
import type { SimulationStatus } from '../types/api';

interface SimulationControlProps {
  status: SimulationStatus | null;
  onStatusChange: () => void;
}

export default function SimulationControl({ status, onStatusChange }: SimulationControlProps) {
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleStart = async () => {
    try {
      setLoading(true);
      setError(null);
      await apiClient.startSimulation();
      onStatusChange();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to start simulation');
    } finally {
      setLoading(false);
    }
  };

  const handleStop = async () => {
    try {
      setLoading(true);
      setError(null);
      await apiClient.stopSimulation();
      onStatusChange();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to stop simulation');
    } finally {
      setLoading(false);
    }
  };

  const isRunning = status?.status === 'running';

  return (
    <div className="bg-white rounded-lg shadow p-6">
      <h2 className="text-xl font-semibold text-gray-900 mb-4">Simulation Control</h2>

      <div className="flex items-center gap-4">
        <button
          onClick={handleStart}
          disabled={loading || isRunning}
          className={`px-6 py-2 rounded-lg font-medium transition-colors ${
            isRunning
              ? 'bg-gray-300 text-gray-500 cursor-not-allowed'
              : 'bg-green-600 text-white hover:bg-green-700'
          }`}
        >
          {loading && !isRunning ? 'Starting...' : 'Start Simulation'}
        </button>

        <button
          onClick={handleStop}
          disabled={loading || !isRunning}
          className={`px-6 py-2 rounded-lg font-medium transition-colors ${
            !isRunning
              ? 'bg-gray-300 text-gray-500 cursor-not-allowed'
              : 'bg-red-600 text-white hover:bg-red-700'
          }`}
        >
          {loading && isRunning ? 'Stopping...' : 'Stop Simulation'}
        </button>

        {status && (
          <div className="ml-4 flex items-center gap-2">
            <div
              className={`w-3 h-3 rounded-full ${
                isRunning ? 'bg-green-500 animate-pulse' : 'bg-gray-400'
              }`}
            />
            <span className="text-sm text-gray-600">
              {isRunning ? 'Running' : 'Stopped'}
            </span>
          </div>
        )}
      </div>

      {error && (
        <div className="mt-4 p-3 bg-red-50 border border-red-200 rounded-lg">
          <p className="text-sm text-red-700">{error}</p>
        </div>
      )}
    </div>
  );
}
