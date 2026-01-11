import { useEffect, useState, useCallback } from 'react';
import { apiClient } from '../services/apiClient';
import type {
  SystemHealth,
  TrafficProfile,
  ExtendedContinuousOptimizationStatus,
} from '../types/api';

interface CityTrafficMetrics {
  activeVehicles: number;
  avgNetworkSpeed: number;
  congestedRoads: number;
  totalRoads: number;
  trafficLights: number;
}

export default function ProductionDashboard() {
  const [health, setHealth] = useState<SystemHealth | null>(null);
  const [profiles, setProfiles] = useState<TrafficProfile[]>([]);
  const [contOptStatus, setContOptStatus] = useState<ExtendedContinuousOptimizationStatus | null>(null);
  const [trafficMetrics, setTrafficMetrics] = useState<CityTrafficMetrics | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [actionLoading, setActionLoading] = useState<string | null>(null);
  const [usePrediction, setUsePrediction] = useState(false);
  const [predictionHorizon, setPredictionHorizon] = useState(30);

  const fetchData = useCallback(async () => {
    try {
      const [healthData, profilesData, contOptData] = await Promise.all([
        apiClient.getSystemHealth(),
        apiClient.getTrafficProfiles().catch(() => ({ success: false, data: { profiles: [] } })),
        apiClient.getContinuousOptimizationStatus().catch(() => ({ success: false, data: null })),
      ]);

      setHealth(healthData);

      // Extract traffic metrics from enhanced health endpoint
      if (healthData?.traffic) {
        setTrafficMetrics({
          activeVehicles: healthData.traffic.activeVehicles,
          avgNetworkSpeed: healthData.traffic.avgNetworkSpeed,
          congestedRoads: healthData.traffic.congestedRoads,
          totalRoads: healthData.traffic.totalRoads,
          trafficLights: healthData.traffic.trafficLights,
        });
      }

      if (profilesData.success && profilesData.data?.profiles) {
        setProfiles(profilesData.data.profiles);
      }
      if (contOptData.success && contOptData.data) {
        const extStatus = contOptData.data as ExtendedContinuousOptimizationStatus;
        setContOptStatus(extStatus);
        if (extStatus.prediction) {
          setUsePrediction(extStatus.prediction.enabled);
          if (extStatus.prediction.horizonMinutes) {
            setPredictionHorizon(extStatus.prediction.horizonMinutes);
          }
        }
      }
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch system status');
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchData();
    const interval = setInterval(fetchData, 5000);
    return () => clearInterval(interval);
  }, [fetchData]);

  const handleStartContOpt = async () => {
    setActionLoading('startContOpt');
    try {
      await apiClient.startContinuousOptimization({
        usePrediction,
        predictionHorizonMinutes: usePrediction ? predictionHorizon : undefined,
      });
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to start optimization');
    } finally {
      setActionLoading(null);
    }
  };

  const handleStopContOpt = async () => {
    setActionLoading('stopContOpt');
    try {
      await apiClient.stopContinuousOptimization();
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to stop optimization');
    } finally {
      setActionLoading(null);
    }
  };

  const handleSetProfile = async (profileName: string) => {
    setActionLoading('setProfile');
    try {
      await apiClient.setActiveProfile(profileName);
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to set profile');
    } finally {
      setActionLoading(null);
    }
  };

  const formatUptime = (seconds: number): string => {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const mins = Math.floor((seconds % 3600) / 60);
    if (days > 0) return `${days}d ${hours}h ${mins}m`;
    if (hours > 0) return `${hours}h ${mins}m`;
    return `${mins}m`;
  };

  if (loading) {
    return (
      <div className="min-h-screen bg-gray-100 flex items-center justify-center">
        <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600"></div>
      </div>
    );
  }

  const isSystemHealthy = health?.status === 'healthy' && health?.simulation?.running;

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="max-w-7xl mx-auto px-4 py-8">
        <div className="mb-6">
          <h1 className="text-4xl font-bold text-gray-900">City Traffic Dashboard</h1>
          <p className="text-gray-600">Real-time traffic monitoring and management</p>
        </div>

        {error && (
          <div className="mb-6 bg-red-50 border border-red-200 text-red-700 px-4 py-3 rounded">
            {error}
          </div>
        )}

        {/* City Status Overview */}
        <div className="grid grid-cols-1 md:grid-cols-5 gap-4 mb-6">
          <div className={`bg-white rounded-lg shadow p-4 border-l-4 ${isSystemHealthy ? 'border-green-500' : 'border-red-500'}`}>
            <div className="text-sm text-gray-500">System Status</div>
            <div className={`text-2xl font-bold ${isSystemHealthy ? 'text-green-600' : 'text-red-600'}`}>
              {isSystemHealthy ? 'ONLINE' : 'OFFLINE'}
            </div>
            <div className="text-xs text-gray-400 mt-1">
              Uptime: {health ? formatUptime(health.uptime) : '-'}
            </div>
          </div>

          <div className="bg-white rounded-lg shadow p-4 border-l-4 border-blue-500">
            <div className="text-sm text-gray-500">Active Vehicles</div>
            <div className="text-2xl font-bold text-blue-600">
              {trafficMetrics?.activeVehicles?.toLocaleString() || '-'}
            </div>
            <div className="text-xs text-gray-400 mt-1">
              In network
            </div>
          </div>

          <div className="bg-white rounded-lg shadow p-4 border-l-4 border-purple-500">
            <div className="text-sm text-gray-500">Avg Speed</div>
            <div className="text-2xl font-bold text-purple-600">
              {trafficMetrics?.avgNetworkSpeed?.toFixed(1) || '-'} km/h
            </div>
            <div className="text-xs text-gray-400 mt-1">
              Network average
            </div>
          </div>

          <div className={`bg-white rounded-lg shadow p-4 border-l-4 ${
            (trafficMetrics?.congestedRoads || 0) > 50 ? 'border-red-500' :
            (trafficMetrics?.congestedRoads || 0) > 20 ? 'border-yellow-500' : 'border-green-500'
          }`}>
            <div className="text-sm text-gray-500">Congested Roads</div>
            <div className={`text-2xl font-bold ${
              (trafficMetrics?.congestedRoads || 0) > 50 ? 'text-red-600' :
              (trafficMetrics?.congestedRoads || 0) > 20 ? 'text-yellow-600' : 'text-green-600'
            }`}>
              {trafficMetrics?.congestedRoads || 0}
            </div>
            <div className="text-xs text-gray-400 mt-1">
              {trafficMetrics?.totalRoads ? `${((trafficMetrics.congestedRoads / trafficMetrics.totalRoads) * 100).toFixed(1)}% of network` : '-'}
            </div>
          </div>

          <div className="bg-white rounded-lg shadow p-4 border-l-4 border-indigo-500">
            <div className="text-sm text-gray-500">Traffic Lights</div>
            <div className="text-2xl font-bold text-indigo-600">
              {trafficMetrics?.trafficLights?.toLocaleString() || '-'}
            </div>
            <div className="text-xs text-gray-400 mt-1">
              Managed signals
            </div>
          </div>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Traffic Profiles */}
          <div className="bg-white rounded-lg shadow p-6">
            <h2 className="text-xl font-semibold text-gray-900 mb-4">Traffic Profiles</h2>
            <p className="text-sm text-gray-500 mb-4">
              Select a traffic profile to apply network-wide timing patterns
            </p>

            <div className="space-y-3">
              {profiles.map((profile) => (
                <div
                  key={profile.id}
                  className={`flex items-center justify-between p-3 rounded-lg border ${
                    profile.isActive ? 'border-blue-500 bg-blue-50' : 'border-gray-200'
                  }`}
                >
                  <div>
                    <div className="font-medium">{profile.name.replace('_', ' ').toUpperCase()}</div>
                    <div className="text-sm text-gray-500">{profile.description}</div>
                  </div>
                  <button
                    onClick={() => handleSetProfile(profile.name)}
                    disabled={profile.isActive || actionLoading !== null}
                    className={`px-3 py-1 rounded text-sm font-medium transition-colors disabled:opacity-50 ${
                      profile.isActive
                        ? 'bg-blue-100 text-blue-700'
                        : 'bg-gray-100 hover:bg-gray-200 text-gray-700'
                    }`}
                  >
                    {profile.isActive ? 'Active' : 'Activate'}
                  </button>
                </div>
              ))}
              {profiles.length === 0 && (
                <div className="text-gray-500 text-center py-4">
                  No traffic profiles configured
                </div>
              )}
            </div>
          </div>

          {/* Live Data Feed */}
          <div className="bg-white rounded-lg shadow p-6">
            <h2 className="text-xl font-semibold text-gray-900 mb-4">Live Data Feed</h2>

            <div className="space-y-4">
              <div className="flex items-center justify-between p-4 bg-gray-50 rounded-lg">
                <div>
                  <div className="font-medium">Feed Status</div>
                  <div className="text-sm text-gray-500">Real-time traffic data source</div>
                </div>
                <div className="flex items-center gap-2">
                  <div className={`w-3 h-3 rounded-full ${health?.feed?.running ? 'bg-green-500 animate-pulse' : 'bg-gray-400'}`}></div>
                  <span className={`font-medium ${health?.feed?.running ? 'text-green-600' : 'text-gray-500'}`}>
                    {health?.feed?.running ? 'Connected' : 'Disconnected'}
                  </span>
                </div>
              </div>

              <div className="grid grid-cols-2 gap-4">
                <div className="p-3 bg-gray-50 rounded-lg">
                  <div className="text-xs text-gray-500">Source</div>
                  <div className="font-medium capitalize">{health?.feed?.source || 'None'}</div>
                </div>
                <div className="p-3 bg-gray-50 rounded-lg">
                  <div className="text-xs text-gray-500">Update Rate</div>
                  <div className="font-medium">1s interval</div>
                </div>
                <div className="p-3 bg-gray-50 rounded-lg">
                  <div className="text-xs text-gray-500">Data Quality</div>
                  <div className={`font-medium ${health?.feed?.healthy ? 'text-green-600' : 'text-yellow-600'}`}>
                    {health?.feed?.healthy ? 'High' : 'Low'}
                  </div>
                </div>
                <div className="p-3 bg-gray-50 rounded-lg">
                  <div className="text-xs text-gray-500">Last Update</div>
                  <div className="font-medium">{new Date().toLocaleTimeString()}</div>
                </div>
              </div>
            </div>
          </div>

          {/* Optimization Engine */}
          <div className="bg-white rounded-lg shadow p-6 lg:col-span-2">
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-xl font-semibold text-gray-900">Traffic Optimization</h2>
              {contOptStatus?.running && (
                <span className={`px-2 py-1 rounded text-xs font-medium ${
                  contOptStatus.mode === 'predictive'
                    ? 'bg-purple-100 text-purple-800'
                    : 'bg-blue-100 text-blue-800'
                }`}>
                  {contOptStatus.mode === 'predictive' ? 'Predictive Mode' : 'Reactive Mode'}
                </span>
              )}
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              <div className="space-y-4">
                <div className="flex items-center justify-between">
                  <div>
                    <div className="font-medium">Background Optimization</div>
                    <div className="text-sm text-gray-500">
                      Continuously optimizes traffic light timings
                    </div>
                  </div>
                  <button
                    onClick={contOptStatus?.running ? handleStopContOpt : handleStartContOpt}
                    disabled={actionLoading !== null}
                    className={`px-4 py-2 rounded-md font-medium transition-colors disabled:opacity-50 ${
                      contOptStatus?.running
                        ? 'bg-red-600 hover:bg-red-700 text-white'
                        : 'bg-blue-600 hover:bg-blue-700 text-white'
                    }`}
                  >
                    {actionLoading === 'startContOpt' || actionLoading === 'stopContOpt'
                      ? 'Loading...'
                      : contOptStatus?.running
                      ? 'Stop'
                      : 'Start'}
                  </button>
                </div>

                {/* Predictive Mode Toggle */}
                {!contOptStatus?.running && (
                  <div className="border border-gray-200 rounded-lg p-4 space-y-3">
                    <div className="flex items-center justify-between">
                      <div>
                        <div className="font-medium text-sm">Predictive Mode</div>
                        <div className="text-xs text-gray-500">Optimize for predicted future traffic</div>
                      </div>
                      <button
                        onClick={() => setUsePrediction(!usePrediction)}
                        className={`relative inline-flex h-6 w-11 items-center rounded-full transition-colors ${
                          usePrediction ? 'bg-purple-600' : 'bg-gray-300'
                        }`}
                      >
                        <span
                          className={`inline-block h-4 w-4 transform rounded-full bg-white transition-transform ${
                            usePrediction ? 'translate-x-6' : 'translate-x-1'
                          }`}
                        />
                      </button>
                    </div>
                    {usePrediction && (
                      <div>
                        <label className="text-xs text-gray-600 block mb-1">Prediction Horizon</label>
                        <div className="flex items-center gap-2">
                          <input
                            type="range"
                            min={10}
                            max={120}
                            step={5}
                            value={predictionHorizon}
                            onChange={(e) => setPredictionHorizon(parseInt(e.target.value))}
                            className="flex-1 h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer"
                          />
                          <span className="text-sm font-medium w-16 text-right">{predictionHorizon} min</span>
                        </div>
                      </div>
                    )}
                  </div>
                )}

                {contOptStatus && (
                  <div className="bg-gray-50 rounded-lg p-4 space-y-2">
                    <div className="grid grid-cols-2 gap-2 text-sm">
                      <div>
                        <span className="text-gray-500">Total Runs:</span>{' '}
                        <span className="font-medium">{contOptStatus.totalOptimizationRuns}</span>
                      </div>
                      <div>
                        <span className="text-gray-500">Successful:</span>{' '}
                        <span className="font-medium text-green-600">{contOptStatus.successfulOptimizations}</span>
                      </div>
                      <div>
                        <span className="text-gray-500">Last Improvement:</span>{' '}
                        <span className={`font-medium ${contOptStatus.lastImprovementPercent > 0 ? 'text-green-600' : 'text-gray-500'}`}>
                          {contOptStatus.lastImprovementPercent.toFixed(1)}%
                        </span>
                      </div>
                      <div>
                        <span className="text-gray-500">Next Opt In:</span>{' '}
                        <span className="font-medium">{Math.floor(contOptStatus.nextOptimizationIn / 60)}m {contOptStatus.nextOptimizationIn % 60}s</span>
                      </div>
                    </div>
                  </div>
                )}
              </div>

              {/* Active Transitions */}
              <div>
                <h3 className="font-medium text-gray-700 mb-2">Active Timing Transitions</h3>
                {contOptStatus?.activeTransitions && contOptStatus.activeTransitions.length > 0 ? (
                  <div className="space-y-2 max-h-48 overflow-y-auto">
                    {contOptStatus.activeTransitions.slice(0, 5).map((t, idx) => (
                      <div key={idx} className="bg-gray-50 rounded p-2 text-sm">
                        <div className="flex justify-between">
                          <span>Road {t.roadId}, Lane {t.lane}</span>
                          <span className={t.isComplete ? 'text-green-600' : 'text-blue-600'}>
                            {t.isComplete ? 'Complete' : `${(t.progress * 100).toFixed(0)}%`}
                          </span>
                        </div>
                        <div className="w-full bg-gray-200 rounded-full h-1 mt-1">
                          <div
                            className="bg-blue-600 h-1 rounded-full transition-all"
                            style={{ width: `${t.progress * 100}%` }}
                          />
                        </div>
                      </div>
                    ))}
                    {contOptStatus.activeTransitions.length > 5 && (
                      <div className="text-sm text-gray-500 text-center">
                        +{contOptStatus.activeTransitions.length - 5} more transitions
                      </div>
                    )}
                  </div>
                ) : (
                  <div className="text-sm text-gray-500 bg-gray-50 rounded p-4 text-center">
                    No active timing transitions
                  </div>
                )}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
