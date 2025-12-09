import { useState, useEffect } from 'react';
import { apiClient } from '../services/apiClient';
import type { SpawnRateConfig, RoadGeometry } from '../types/api';

interface SpawnRatePanelProps {
  isOpen: boolean;
  onToggle: () => void;
  roads: RoadGeometry[];
}

export default function SpawnRatePanel({ isOpen, onToggle, roads }: SpawnRatePanelProps) {
  const [spawnRates, setSpawnRates] = useState<SpawnRateConfig[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [editingRoad, setEditingRoad] = useState<number | null>(null);
  const [editValue, setEditValue] = useState(0);
  const [newRoadId, setNewRoadId] = useState<number | ''>('');
  const [newRate, setNewRate] = useState(10);

  const fetchSpawnRates = async () => {
    try {
      setLoading(true);
      setError(null);
      const response = await apiClient.getSpawnRates();
      setSpawnRates(response.rates);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch spawn rates');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (isOpen) {
      fetchSpawnRates();
    }
  }, [isOpen]);

  const handleEdit = (rate: SpawnRateConfig) => {
    setEditingRoad(rate.roadId);
    setEditValue(rate.vehiclesPerMinute);
  };

  const handleSave = async (roadId: number) => {
    try {
      setLoading(true);
      await apiClient.setSpawnRates({
        rates: [{ roadId, vehiclesPerMinute: editValue }],
      });
      setEditingRoad(null);
      await fetchSpawnRates();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to update spawn rate');
    } finally {
      setLoading(false);
    }
  };

  const handleAddNew = async () => {
    if (newRoadId === '' || newRate <= 0) return;

    try {
      setLoading(true);
      await apiClient.setSpawnRates({
        rates: [{ roadId: Number(newRoadId), vehiclesPerMinute: newRate }],
      });
      setNewRoadId('');
      setNewRate(10);
      await fetchSpawnRates();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to add spawn rate');
    } finally {
      setLoading(false);
    }
  };

  const handleRemove = async (roadId: number) => {
    try {
      setLoading(true);
      // Set rate to 0 to effectively disable spawning
      await apiClient.setSpawnRates({
        rates: [{ roadId, vehiclesPerMinute: 0 }],
      });
      await fetchSpawnRates();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to remove spawn rate');
    } finally {
      setLoading(false);
    }
  };

  const handleCancel = () => {
    setEditingRoad(null);
  };

  // Get road name or fallback to ID
  const getRoadLabel = (roadId: number) => {
    const road = roads.find(r => r.id === roadId);
    return road ? `Road ${roadId} (${road.length.toFixed(0)}m)` : `Road ${roadId}`;
  };

  // Get available roads that don't have spawn rates yet
  const availableRoads = roads.filter(
    r => !spawnRates.some(s => s.roadId === r.id && s.vehiclesPerMinute > 0)
  );

  return (
    <div className={`fixed left-0 top-20 h-[calc(100vh-5rem)] bg-white shadow-lg transition-all duration-300 z-[1000] ${isOpen ? 'w-80' : 'w-0'} overflow-hidden`}>
      <div className="w-80 h-full flex flex-col">
        {/* Header */}
        <div className="p-4 border-b bg-gray-50 flex justify-between items-center">
          <h2 className="text-lg font-semibold text-gray-900">Spawn Rates</h2>
          <button
            onClick={fetchSpawnRates}
            disabled={loading}
            className="text-sm text-blue-600 hover:text-blue-800 disabled:opacity-50"
          >
            Refresh
          </button>
        </div>

        {/* Content */}
        <div className="flex-1 overflow-y-auto p-4">
          {error && (
            <div className="mb-4 p-3 bg-red-50 border border-red-200 rounded-lg">
              <p className="text-sm text-red-700">{error}</p>
            </div>
          )}

          {/* Add new spawn rate */}
          <div className="bg-blue-50 rounded-lg p-3 mb-4">
            <h3 className="text-sm font-medium text-blue-900 mb-2">Add Spawn Point</h3>
            <div className="space-y-2">
              <div>
                <label className="block text-xs text-blue-700 mb-1">Road</label>
                <select
                  value={newRoadId}
                  onChange={(e) => setNewRoadId(e.target.value ? Number(e.target.value) : '')}
                  className="w-full px-2 py-1 text-sm border rounded bg-white"
                >
                  <option value="">Select a road...</option>
                  {availableRoads.map(road => (
                    <option key={road.id} value={road.id}>
                      Road {road.id} ({road.length.toFixed(0)}m, {road.lanes} lanes)
                    </option>
                  ))}
                </select>
              </div>
              <div>
                <label className="block text-xs text-blue-700 mb-1">Rate (vehicles/min)</label>
                <input
                  type="number"
                  value={newRate}
                  onChange={(e) => setNewRate(Number(e.target.value))}
                  className="w-full px-2 py-1 text-sm border rounded"
                  min="1"
                  max="60"
                />
              </div>
              <button
                onClick={handleAddNew}
                disabled={loading || newRoadId === ''}
                className="w-full px-3 py-1.5 text-sm bg-blue-600 text-white rounded hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed"
              >
                Add Spawn Point
              </button>
            </div>
          </div>

          {/* Existing spawn rates */}
          <h3 className="text-sm font-medium text-gray-700 mb-2">Active Spawn Points</h3>
          {loading && spawnRates.length === 0 ? (
            <div className="text-center text-gray-500 py-4">Loading...</div>
          ) : spawnRates.filter(r => r.vehiclesPerMinute > 0).length === 0 ? (
            <div className="text-center text-gray-500 py-4">
              No active spawn points. Add one above.
            </div>
          ) : (
            <div className="space-y-2">
              {spawnRates
                .filter(r => r.vehiclesPerMinute > 0)
                .sort((a, b) => a.roadId - b.roadId)
                .map((rate) => (
                  <div key={rate.roadId} className="bg-gray-50 rounded-lg p-3 border">
                    <div className="flex items-center justify-between mb-2">
                      <span className="text-sm font-medium text-gray-700">
                        {getRoadLabel(rate.roadId)}
                      </span>
                      <button
                        onClick={() => handleRemove(rate.roadId)}
                        className="text-xs text-red-600 hover:text-red-800"
                        title="Remove spawn point"
                      >
                        Remove
                      </button>
                    </div>

                    {editingRoad === rate.roadId ? (
                      <div className="space-y-2">
                        <div>
                          <label className="block text-xs text-gray-500">Vehicles per minute</label>
                          <input
                            type="number"
                            value={editValue}
                            onChange={(e) => setEditValue(Number(e.target.value))}
                            className="w-full px-2 py-1 text-sm border rounded"
                            min="1"
                            max="60"
                          />
                        </div>
                        <div className="flex gap-2">
                          <button
                            onClick={() => handleSave(rate.roadId)}
                            disabled={loading}
                            className="flex-1 px-2 py-1 text-xs bg-blue-600 text-white rounded hover:bg-blue-700 disabled:opacity-50"
                          >
                            Save
                          </button>
                          <button
                            onClick={handleCancel}
                            className="flex-1 px-2 py-1 text-xs bg-gray-200 text-gray-700 rounded hover:bg-gray-300"
                          >
                            Cancel
                          </button>
                        </div>
                      </div>
                    ) : (
                      <div className="flex items-center justify-between">
                        <div className="flex items-center gap-2">
                          <div className="w-2 h-2 bg-green-500 rounded-full animate-pulse" />
                          <span className="text-sm text-gray-600">
                            {rate.vehiclesPerMinute} vehicles/min
                          </span>
                        </div>
                        <button
                          onClick={() => handleEdit(rate)}
                          className="text-xs text-blue-600 hover:text-blue-800"
                        >
                          Edit
                        </button>
                      </div>
                    )}
                  </div>
                ))}
            </div>
          )}

          {/* Info box */}
          <div className="mt-4 p-3 bg-gray-100 rounded-lg">
            <p className="text-xs text-gray-600">
              Spawn points continuously add new vehicles to the simulation at the specified rate.
              Vehicles spawn at position 0 of each road.
            </p>
          </div>
        </div>
      </div>

      {/* Toggle button */}
      <button
        onClick={onToggle}
        className="absolute right-0 top-1/2 translate-x-full -translate-y-1/2 bg-white shadow-lg rounded-r-lg p-2 border border-l-0"
      >
        <svg
          className={`w-5 h-5 text-gray-600 transition-transform ${isOpen ? '' : 'rotate-180'}`}
          fill="none"
          stroke="currentColor"
          viewBox="0 0 24 24"
        >
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" />
        </svg>
      </button>
    </div>
  );
}
