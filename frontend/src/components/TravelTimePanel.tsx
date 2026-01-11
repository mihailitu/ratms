import { useState, useEffect } from 'react';
import { apiClient } from '../services/apiClient';
import type { ODPair, TravelTimeStats, RoadGeometry } from '../types/api';

interface TravelTimePanelProps {
  isOpen: boolean;
  onToggle: () => void;
  roads: RoadGeometry[];
}

export default function TravelTimePanel({ isOpen, onToggle, roads }: TravelTimePanelProps) {
  const [odPairs, setODPairs] = useState<ODPair[]>([]);
  const [stats, setStats] = useState<TravelTimeStats[]>([]);
  const [trackedCount, setTrackedCount] = useState(0);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Form state for adding new O-D pair
  const [newOriginId, setNewOriginId] = useState<number | ''>('');
  const [newDestId, setNewDestId] = useState<number | ''>('');
  const [newName, setNewName] = useState('');

  // Expanded O-D pair for details
  const [expandedPair, setExpandedPair] = useState<number | null>(null);

  const fetchData = async () => {
    try {
      setLoading(true);
      setError(null);

      const [pairsRes, statsRes, trackedRes] = await Promise.all([
        apiClient.getODPairs(),
        apiClient.getAllTravelTimeStats(),
        apiClient.getTrackedVehicles()
      ]);

      setODPairs(pairsRes.odPairs || []);
      setStats(statsRes.stats || []);
      setTrackedCount(trackedRes.count || 0);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch travel time data');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (isOpen) {
      fetchData();
      // Auto-refresh every 10 seconds when panel is open
      const interval = setInterval(fetchData, 10000);
      return () => clearInterval(interval);
    }
  }, [isOpen]);

  const handleAddODPair = async () => {
    if (newOriginId === '' || newDestId === '') return;
    if (newOriginId === newDestId) {
      setError('Origin and destination must be different');
      return;
    }

    try {
      setLoading(true);
      await apiClient.createODPair(
        Number(newOriginId),
        Number(newDestId),
        newName || `Route ${newOriginId} to ${newDestId}`
      );
      setNewOriginId('');
      setNewDestId('');
      setNewName('');
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to add O-D pair');
    } finally {
      setLoading(false);
    }
  };

  const handleDeleteODPair = async (id: number) => {
    try {
      setLoading(true);
      await apiClient.deleteODPair(id);
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to delete O-D pair');
    } finally {
      setLoading(false);
    }
  };

  const getStatsForPair = (odPairId: number): TravelTimeStats | undefined => {
    return stats.find(s => s.odPairId === odPairId);
  };

  const formatTime = (seconds: number): string => {
    if (seconds < 60) return `${seconds.toFixed(1)}s`;
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins}m ${secs.toFixed(0)}s`;
  };

  const getRoadLabel = (roadId: number) => {
    const road = roads.find(r => r.id === roadId);
    return road ? `Road ${roadId} (${road.length.toFixed(0)}m)` : `Road ${roadId}`;
  };

  return (
    <div className={`fixed right-0 top-20 h-[calc(100vh-5rem)] bg-white shadow-lg transition-all duration-300 z-[1000] ${isOpen ? 'w-96' : 'w-0'} overflow-hidden`}>
      <div className="w-96 h-full flex flex-col">
        {/* Header */}
        <div className="p-4 border-b bg-gray-50 flex justify-between items-center">
          <div>
            <h2 className="text-lg font-semibold text-gray-900">Travel Times</h2>
            <p className="text-xs text-gray-500">{trackedCount} vehicles being tracked</p>
          </div>
          <button
            onClick={fetchData}
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
              <button
                onClick={() => setError(null)}
                className="text-xs text-red-600 hover:text-red-800 mt-1"
              >
                Dismiss
              </button>
            </div>
          )}

          {/* Add new O-D pair */}
          <div className="bg-purple-50 rounded-lg p-3 mb-4">
            <h3 className="text-sm font-medium text-purple-900 mb-2">Add O-D Pair</h3>
            <div className="space-y-2">
              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="block text-xs text-purple-700 mb-1">Origin Road</label>
                  <select
                    value={newOriginId}
                    onChange={(e) => setNewOriginId(e.target.value ? Number(e.target.value) : '')}
                    className="w-full px-2 py-1 text-sm border rounded bg-white"
                  >
                    <option value="">Select...</option>
                    {roads.map(road => (
                      <option key={road.id} value={road.id}>
                        Road {road.id}
                      </option>
                    ))}
                  </select>
                </div>
                <div>
                  <label className="block text-xs text-purple-700 mb-1">Destination Road</label>
                  <select
                    value={newDestId}
                    onChange={(e) => setNewDestId(e.target.value ? Number(e.target.value) : '')}
                    className="w-full px-2 py-1 text-sm border rounded bg-white"
                  >
                    <option value="">Select...</option>
                    {roads.filter(r => r.id !== newOriginId).map(road => (
                      <option key={road.id} value={road.id}>
                        Road {road.id}
                      </option>
                    ))}
                  </select>
                </div>
              </div>
              <div>
                <label className="block text-xs text-purple-700 mb-1">Name (optional)</label>
                <input
                  type="text"
                  value={newName}
                  onChange={(e) => setNewName(e.target.value)}
                  placeholder="e.g., Main St to Downtown"
                  className="w-full px-2 py-1 text-sm border rounded"
                />
              </div>
              <button
                onClick={handleAddODPair}
                disabled={loading || newOriginId === '' || newDestId === ''}
                className="w-full px-3 py-1.5 text-sm bg-purple-600 text-white rounded hover:bg-purple-700 disabled:opacity-50 disabled:cursor-not-allowed"
              >
                Add O-D Pair
              </button>
            </div>
          </div>

          {/* O-D pairs list */}
          <h3 className="text-sm font-medium text-gray-700 mb-2">
            Monitored Routes ({odPairs.length})
          </h3>
          {loading && odPairs.length === 0 ? (
            <div className="text-center text-gray-500 py-4">Loading...</div>
          ) : odPairs.length === 0 ? (
            <div className="text-center text-gray-500 py-4">
              No O-D pairs configured. Add one above to start tracking travel times.
            </div>
          ) : (
            <div className="space-y-2">
              {odPairs.map((pair) => {
                const pairStats = getStatsForPair(pair.id);
                const isExpanded = expandedPair === pair.id;

                return (
                  <div
                    key={pair.id}
                    className="bg-gray-50 rounded-lg border overflow-hidden"
                  >
                    {/* Header */}
                    <div
                      className="p-3 cursor-pointer hover:bg-gray-100"
                      onClick={() => setExpandedPair(isExpanded ? null : pair.id)}
                    >
                      <div className="flex items-center justify-between">
                        <div className="flex-1">
                          <p className="text-sm font-medium text-gray-800">
                            {pair.name || `Route ${pair.id}`}
                          </p>
                          <p className="text-xs text-gray-500">
                            {getRoadLabel(pair.originRoadId)} &rarr; {getRoadLabel(pair.destinationRoadId)}
                          </p>
                        </div>
                        <div className="flex items-center gap-2">
                          {pairStats && pairStats.sampleCount > 0 ? (
                            <span className="text-sm font-mono text-green-600">
                              {formatTime(pairStats.avgTime)}
                            </span>
                          ) : (
                            <span className="text-xs text-gray-400">No data</span>
                          )}
                          <svg
                            className={`w-4 h-4 text-gray-400 transition-transform ${isExpanded ? 'rotate-180' : ''}`}
                            fill="none"
                            stroke="currentColor"
                            viewBox="0 0 24 24"
                          >
                            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 9l-7 7-7-7" />
                          </svg>
                        </div>
                      </div>
                    </div>

                    {/* Expanded details */}
                    {isExpanded && (
                      <div className="border-t p-3 bg-white">
                        {pairStats && pairStats.sampleCount > 0 ? (
                          <div className="space-y-2">
                            <div className="grid grid-cols-2 gap-2 text-xs">
                              <div className="bg-gray-50 p-2 rounded">
                                <p className="text-gray-500">Average</p>
                                <p className="font-mono text-gray-800">{formatTime(pairStats.avgTime)}</p>
                              </div>
                              <div className="bg-gray-50 p-2 rounded">
                                <p className="text-gray-500">Samples</p>
                                <p className="font-mono text-gray-800">{pairStats.sampleCount}</p>
                              </div>
                              <div className="bg-gray-50 p-2 rounded">
                                <p className="text-gray-500">Min</p>
                                <p className="font-mono text-gray-800">{formatTime(pairStats.minTime)}</p>
                              </div>
                              <div className="bg-gray-50 p-2 rounded">
                                <p className="text-gray-500">Max</p>
                                <p className="font-mono text-gray-800">{formatTime(pairStats.maxTime)}</p>
                              </div>
                              <div className="bg-gray-50 p-2 rounded">
                                <p className="text-gray-500">Median (P50)</p>
                                <p className="font-mono text-gray-800">{formatTime(pairStats.p50Time)}</p>
                              </div>
                              <div className="bg-gray-50 p-2 rounded">
                                <p className="text-gray-500">P95</p>
                                <p className="font-mono text-gray-800">{formatTime(pairStats.p95Time)}</p>
                              </div>
                            </div>
                          </div>
                        ) : (
                          <p className="text-xs text-gray-500 text-center py-2">
                            No travel time samples yet. Waiting for vehicles to complete this route.
                          </p>
                        )}
                        <button
                          onClick={(e) => {
                            e.stopPropagation();
                            handleDeleteODPair(pair.id);
                          }}
                          className="mt-3 w-full px-2 py-1 text-xs text-red-600 border border-red-200 rounded hover:bg-red-50"
                        >
                          Remove O-D Pair
                        </button>
                      </div>
                    )}
                  </div>
                );
              })}
            </div>
          )}

          {/* Info box */}
          <div className="mt-4 p-3 bg-gray-100 rounded-lg">
            <p className="text-xs text-gray-600">
              Origin-Destination (O-D) pairs track how long vehicles take to travel between
              two roads. Vehicles entering the origin road are tracked until they reach
              the destination road.
            </p>
          </div>
        </div>
      </div>

      {/* Toggle button */}
      <button
        onClick={onToggle}
        className="absolute left-0 top-1/2 -translate-x-full -translate-y-1/2 bg-white shadow-lg rounded-l-lg p-2 border border-r-0"
      >
        <svg
          className={`w-5 h-5 text-gray-600 transition-transform ${isOpen ? 'rotate-180' : ''}`}
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
