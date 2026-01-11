import { useState, useEffect } from 'react';
import { apiClient } from '../services/apiClient';
import type { TrafficLightConfig, SetTrafficLightsRequest } from '../types/api';

interface TrafficLightPanelProps {
  isOpen: boolean;
  onToggle: () => void;
}

export default function TrafficLightPanel({ isOpen, onToggle }: TrafficLightPanelProps) {
  const [trafficLights, setTrafficLights] = useState<TrafficLightConfig[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [editingLight, setEditingLight] = useState<string | null>(null);
  const [editValues, setEditValues] = useState({ greenTime: 0, yellowTime: 0, redTime: 0 });

  const fetchTrafficLights = async () => {
    try {
      setLoading(true);
      setError(null);
      const response = await apiClient.getTrafficLights();
      setTrafficLights(response.trafficLights);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch traffic lights');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (isOpen) {
      fetchTrafficLights();
    }
  }, [isOpen]);

  const handleEdit = (light: TrafficLightConfig) => {
    const key = `${light.roadId}-${light.lane}`;
    setEditingLight(key);
    setEditValues({
      greenTime: light.greenTime,
      yellowTime: light.yellowTime,
      redTime: light.redTime,
    });
  };

  const handleSave = async (light: TrafficLightConfig) => {
    try {
      setLoading(true);
      const request: SetTrafficLightsRequest = {
        updates: [{
          roadId: light.roadId,
          lane: light.lane,
          greenTime: editValues.greenTime,
          yellowTime: editValues.yellowTime,
          redTime: editValues.redTime,
        }],
      };
      await apiClient.setTrafficLights(request);
      setEditingLight(null);
      await fetchTrafficLights();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to update traffic light');
    } finally {
      setLoading(false);
    }
  };

  const handleCancel = () => {
    setEditingLight(null);
  };

  const getStateColor = (state: string) => {
    switch (state) {
      case 'G': return 'bg-green-500';
      case 'Y': return 'bg-yellow-500';
      case 'R': return 'bg-red-500';
      default: return 'bg-gray-400';
    }
  };

  const getStateName = (state: string) => {
    switch (state) {
      case 'G': return 'Green';
      case 'Y': return 'Yellow';
      case 'R': return 'Red';
      default: return 'Unknown';
    }
  };

  // Group traffic lights by road
  const lightsByRoad = trafficLights.reduce((acc, light) => {
    if (!acc[light.roadId]) {
      acc[light.roadId] = [];
    }
    acc[light.roadId].push(light);
    return acc;
  }, {} as Record<number, TrafficLightConfig[]>);

  return (
    <div className={`fixed right-0 top-20 h-[calc(100vh-5rem)] bg-white shadow-lg transition-all duration-300 z-[1000] ${isOpen ? 'w-80' : 'w-0'} overflow-hidden`}>
      <div className="w-80 h-full flex flex-col">
        {/* Header */}
        <div className="p-4 border-b bg-gray-50 flex justify-between items-center">
          <h2 className="text-lg font-semibold text-gray-900">Traffic Lights</h2>
          <button
            onClick={fetchTrafficLights}
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

          {loading && trafficLights.length === 0 ? (
            <div className="text-center text-gray-500 py-8">Loading...</div>
          ) : trafficLights.length === 0 ? (
            <div className="text-center text-gray-500 py-8">No traffic lights found</div>
          ) : (
            <div className="space-y-4">
              {Object.entries(lightsByRoad).map(([roadId, lights]) => (
                <div key={roadId} className="bg-gray-50 rounded-lg p-3">
                  <h3 className="text-sm font-medium text-gray-700 mb-2">Road {roadId}</h3>
                  <div className="space-y-2">
                    {lights.map((light) => {
                      const key = `${light.roadId}-${light.lane}`;
                      const isEditing = editingLight === key;

                      return (
                        <div key={key} className="bg-white rounded p-2 border">
                          <div className="flex items-center justify-between mb-2">
                            <span className="text-sm text-gray-600">Lane {light.lane}</span>
                            <div className="flex items-center gap-2">
                              <div className={`w-4 h-4 rounded-full ${getStateColor(light.currentState)}`} />
                              <span className="text-xs text-gray-500">{getStateName(light.currentState)}</span>
                            </div>
                          </div>

                          {isEditing ? (
                            <div className="space-y-2">
                              <div className="grid grid-cols-3 gap-2">
                                <div>
                                  <label className="block text-xs text-gray-500">Green</label>
                                  <input
                                    type="number"
                                    value={editValues.greenTime}
                                    onChange={(e) => setEditValues({ ...editValues, greenTime: Number(e.target.value) })}
                                    className="w-full px-2 py-1 text-sm border rounded"
                                    min="1"
                                    max="120"
                                  />
                                </div>
                                <div>
                                  <label className="block text-xs text-gray-500">Yellow</label>
                                  <input
                                    type="number"
                                    value={editValues.yellowTime}
                                    onChange={(e) => setEditValues({ ...editValues, yellowTime: Number(e.target.value) })}
                                    className="w-full px-2 py-1 text-sm border rounded"
                                    min="1"
                                    max="10"
                                  />
                                </div>
                                <div>
                                  <label className="block text-xs text-gray-500">Red</label>
                                  <input
                                    type="number"
                                    value={editValues.redTime}
                                    onChange={(e) => setEditValues({ ...editValues, redTime: Number(e.target.value) })}
                                    className="w-full px-2 py-1 text-sm border rounded"
                                    min="1"
                                    max="120"
                                  />
                                </div>
                              </div>
                              <div className="flex gap-2">
                                <button
                                  onClick={() => handleSave(light)}
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
                              <div className="text-xs text-gray-500">
                                <span className="text-green-600">{light.greenTime}s</span> /
                                <span className="text-yellow-600"> {light.yellowTime}s</span> /
                                <span className="text-red-600"> {light.redTime}s</span>
                              </div>
                              <button
                                onClick={() => handleEdit(light)}
                                className="text-xs text-blue-600 hover:text-blue-800"
                              >
                                Edit
                              </button>
                            </div>
                          )}
                        </div>
                      );
                    })}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>

      {/* Toggle button */}
      <button
        onClick={onToggle}
        className="absolute left-0 top-1/2 -translate-x-full -translate-y-1/2 bg-white shadow-lg rounded-l-lg p-2 border border-r-0"
      >
        <svg
          className={`w-5 h-5 text-gray-600 transition-transform ${isOpen ? '' : 'rotate-180'}`}
          fill="none"
          stroke="currentColor"
          viewBox="0 0 24 24"
        >
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
        </svg>
      </button>
    </div>
  );
}
