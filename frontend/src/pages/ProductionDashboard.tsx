import { useEffect, useState, useCallback } from 'react';
import { apiClient } from '../services/apiClient';
import type {
  SystemHealth,
  TrafficProfile,
  SpawningStatus,
  ExtendedContinuousOptimizationStatus,
  SimulationConfig,
} from '../types/api';

export default function ProductionDashboard() {
  const [health, setHealth] = useState<SystemHealth | null>(null);
  const [profiles, setProfiles] = useState<TrafficProfile[]>([]);
  const [spawningStatus, setSpawningStatus] = useState<SpawningStatus | null>(null);
  const [contOptStatus, setContOptStatus] = useState<ExtendedContinuousOptimizationStatus | null>(null);
  const [simConfig, setSimConfig] = useState<SimulationConfig | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [actionLoading, setActionLoading] = useState<string | null>(null);
  const [usePrediction, setUsePrediction] = useState(false);
  const [predictionHorizon, setPredictionHorizon] = useState(30);
  const [stepLimitInput, setStepLimitInput] = useState(10000);

  const fetchData = useCallback(async () => {
    try {
      const [healthData, profilesData, spawningData, contOptData, configData] = await Promise.all([
        apiClient.getSystemHealth(),
        apiClient.getTrafficProfiles().catch(() => ({ success: false, data: { profiles: [] } })),
        apiClient.getSpawningStatus().catch(() => ({ success: false, data: null })),
        apiClient.getContinuousOptimizationStatus().catch(() => ({ success: false, data: null })),
        apiClient.getSimulationConfig().catch(() => null),
      ]);

      setHealth(healthData);
      if (profilesData.success && profilesData.data?.profiles) {
        setProfiles(profilesData.data.profiles);
      }
      if (spawningData.success && spawningData.data) {
        setSpawningStatus(spawningData.data);
      }
      if (contOptData.success && contOptData.data) {
        setContOptStatus(contOptData.data as ExtendedContinuousOptimizationStatus);
        // Sync prediction state from server
        if (contOptData.data.prediction) {
          setUsePrediction(contOptData.data.prediction.enabled);
          if (contOptData.data.prediction.horizonMinutes) {
            setPredictionHorizon(contOptData.data.prediction.horizonMinutes);
          }
        }
      }
      if (configData) {
        setSimConfig(configData);
        setStepLimitInput(configData.stepLimit);
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
    const interval = setInterval(fetchData, 5000); // Refresh every 5 seconds
    return () => clearInterval(interval);
  }, [fetchData]);

  const handleStartContinuous = async () => {
    setActionLoading('startSim');
    try {
      await apiClient.startContinuousSimulation();
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to start simulation');
    } finally {
      setActionLoading(null);
    }
  };

  const handleStopSimulation = async () => {
    setActionLoading('stopSim');
    try {
      await apiClient.stopSimulation();
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to stop simulation');
    } finally {
      setActionLoading(null);
    }
  };

  const handlePauseSimulation = async () => {
    setActionLoading('pauseSim');
    try {
      await apiClient.pauseSimulation();
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to pause simulation');
    } finally {
      setActionLoading(null);
    }
  };

  const handleResumeSimulation = async () => {
    setActionLoading('resumeSim');
    try {
      await apiClient.resumeSimulation();
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to resume simulation');
    } finally {
      setActionLoading(null);
    }
  };

  const handleToggleContinuousMode = async () => {
    setActionLoading('toggleCont');
    try {
      await apiClient.setSimulationConfig({ continuousMode: !simConfig?.continuousMode });
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to toggle continuous mode');
    } finally {
      setActionLoading(null);
    }
  };

  const handleUpdateStepLimit = async () => {
    setActionLoading('updateLimit');
    try {
      await apiClient.setSimulationConfig({ stepLimit: stepLimitInput });
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to update step limit');
    } finally {
      setActionLoading(null);
    }
  };

  const handleStartSpawning = async () => {
    setActionLoading('startSpawn');
    try {
      await apiClient.startSpawning();
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to start spawning');
    } finally {
      setActionLoading(null);
    }
  };

  const handleStopSpawning = async () => {
    setActionLoading('stopSpawn');
    try {
      await apiClient.stopSpawning();
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to stop spawning');
    } finally {
      setActionLoading(null);
    }
  };

  const handleStartContOpt = async () => {
    setActionLoading('startContOpt');
    try {
      await apiClient.startContinuousOptimization({
        usePrediction,
        predictionHorizonMinutes: usePrediction ? predictionHorizon : undefined,
      });
      await fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to start continuous optimization');
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
      setError(err instanceof Error ? err.message : 'Failed to stop continuous optimization');
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

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="max-w-7xl mx-auto px-4 py-8">
        <div className="mb-6">
          <h1 className="text-4xl font-bold text-gray-900">Production Dashboard</h1>
          <p className="text-gray-600">Monitor and control the production traffic management system</p>
        </div>

        {error && (
          <div className="mb-6 bg-red-50 border border-red-200 text-red-700 px-4 py-3 rounded">
            {error}
          </div>
        )}

        {/* System Health Overview */}
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4 mb-6">
          <div className={`bg-white rounded-lg shadow p-4 border-l-4 ${health?.status === 'healthy' ? 'border-green-500' : 'border-red-500'}`}>
            <div className="text-sm text-gray-500">System Status</div>
            <div className={`text-2xl font-bold ${health?.status === 'healthy' ? 'text-green-600' : 'text-red-600'}`}>
              {health?.status?.toUpperCase() || 'UNKNOWN'}
            </div>
            <div className="text-xs text-gray-400 mt-1">
              Uptime: {health ? formatUptime(health.uptime) : '-'}
            </div>
          </div>

          <div className={`bg-white rounded-lg shadow p-4 border-l-4 ${
            health?.simulation.paused ? 'border-yellow-500' :
            health?.simulation.running ? 'border-blue-500' : 'border-gray-300'
          }`}>
            <div className="text-sm text-gray-500">Simulation</div>
            <div className={`text-2xl font-bold ${
              health?.simulation.paused ? 'text-yellow-600' :
              health?.simulation.running ? 'text-blue-600' : 'text-gray-400'
            }`}>
              {health?.simulation.paused ? 'PAUSED' :
               health?.simulation.running ? 'RUNNING' : 'STOPPED'}
            </div>
            <div className="text-xs text-gray-400 mt-1">
              {health?.simulation.continuousMode ? 'Continuous Mode' : `Step Limit: ${health?.simulation.stepLimit?.toLocaleString() || 10000}`}
              {health?.restartCount ? ` | Restarts: ${health.restartCount}` : ''}
            </div>
          </div>

          <div className="bg-white rounded-lg shadow p-4 border-l-4 border-purple-500">
            <div className="text-sm text-gray-500">Simulation Progress</div>
            <div className="text-2xl font-bold text-purple-600">
              {health?.simulation.currentStep?.toLocaleString() || 0}
            </div>
            <div className="text-xs text-gray-400 mt-1">
              Time: {health?.simulation.simulationTime?.toFixed(1) || 0}s
            </div>
          </div>

          <div className="bg-white rounded-lg shadow p-4 border-l-4 border-indigo-500">
            <div className="text-sm text-gray-500">Server Version</div>
            <div className="text-2xl font-bold text-indigo-600">
              {health?.version || '-'}
            </div>
            <div className="text-xs text-gray-400 mt-1">
              {health?.service || 'RATMS API Server'}
            </div>
          </div>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Simulation Control */}
          <div className="bg-white rounded-lg shadow p-6">
            <h2 className="text-xl font-semibold text-gray-900 mb-4">Simulation Control</h2>

            <div className="space-y-4">
              {/* Start/Stop Controls */}
              <div className="flex items-center justify-between">
                <div>
                  <div className="font-medium">Simulation Engine</div>
                  <div className="text-sm text-gray-500">
                    {health?.simulation.continuousMode ? 'Continuous mode - runs indefinitely' : 'Limited mode - stops at step limit'}
                  </div>
                </div>
                <div className="flex gap-2">
                  {health?.simulation.running && !health?.simulation.paused && (
                    <button
                      onClick={handlePauseSimulation}
                      disabled={actionLoading !== null}
                      className="px-3 py-2 rounded-md font-medium transition-colors disabled:opacity-50 bg-yellow-600 hover:bg-yellow-700 text-white"
                    >
                      {actionLoading === 'pauseSim' ? '...' : 'Pause'}
                    </button>
                  )}
                  {health?.simulation.paused && (
                    <button
                      onClick={handleResumeSimulation}
                      disabled={actionLoading !== null}
                      className="px-3 py-2 rounded-md font-medium transition-colors disabled:opacity-50 bg-green-600 hover:bg-green-700 text-white"
                    >
                      {actionLoading === 'resumeSim' ? '...' : 'Resume'}
                    </button>
                  )}
                  <button
                    onClick={health?.simulation.running ? handleStopSimulation : handleStartContinuous}
                    disabled={actionLoading !== null}
                    className={`px-4 py-2 rounded-md font-medium transition-colors disabled:opacity-50 ${
                      health?.simulation.running
                        ? 'bg-red-600 hover:bg-red-700 text-white'
                        : 'bg-blue-600 hover:bg-blue-700 text-white'
                    }`}
                  >
                    {actionLoading === 'startSim' || actionLoading === 'stopSim'
                      ? 'Loading...'
                      : health?.simulation.running
                      ? 'Stop'
                      : 'Start'}
                  </button>
                </div>
              </div>

              {/* Continuous Mode Toggle */}
              <div className="border border-gray-200 rounded-lg p-4">
                <div className="flex items-center justify-between">
                  <div>
                    <div className="font-medium text-sm">Continuous Mode</div>
                    <div className="text-xs text-gray-500">Run indefinitely (ignores step limit)</div>
                  </div>
                  <button
                    onClick={handleToggleContinuousMode}
                    disabled={actionLoading !== null || health?.simulation.running}
                    className={`relative inline-flex h-6 w-11 items-center rounded-full transition-colors disabled:opacity-50 ${
                      simConfig?.continuousMode ? 'bg-blue-600' : 'bg-gray-300'
                    }`}
                  >
                    <span
                      className={`inline-block h-4 w-4 transform rounded-full bg-white transition-transform ${
                        simConfig?.continuousMode ? 'translate-x-6' : 'translate-x-1'
                      }`}
                    />
                  </button>
                </div>

                {/* Step Limit Configuration */}
                {!simConfig?.continuousMode && (
                  <div className="mt-3 pt-3 border-t border-gray-200">
                    <label className="text-xs text-gray-600 block mb-1">Step Limit</label>
                    <div className="flex items-center gap-2">
                      <input
                        type="number"
                        min={100}
                        step={1000}
                        value={stepLimitInput}
                        onChange={(e) => setStepLimitInput(parseInt(e.target.value) || 10000)}
                        disabled={health?.simulation.running}
                        className="flex-1 px-2 py-1 border border-gray-300 rounded text-sm disabled:opacity-50"
                      />
                      <button
                        onClick={handleUpdateStepLimit}
                        disabled={actionLoading !== null || health?.simulation.running || stepLimitInput === simConfig?.stepLimit}
                        className="px-3 py-1 bg-blue-600 text-white rounded text-sm font-medium disabled:opacity-50 hover:bg-blue-700"
                      >
                        {actionLoading === 'updateLimit' ? '...' : 'Apply'}
                      </button>
                    </div>
                  </div>
                )}
              </div>

              {/* Simulation Stats */}
              {health?.simulation.running && (
                <div className="bg-gray-50 rounded-lg p-3">
                  <div className="grid grid-cols-2 gap-2 text-sm">
                    <div>
                      <span className="text-gray-500">Simulation Time:</span>{' '}
                      <span className="font-medium">{health.simulation.simulationTime.toFixed(1)}s</span>
                    </div>
                    <div>
                      <span className="text-gray-500">Steps:</span>{' '}
                      <span className="font-medium">{health.simulation.currentStep.toLocaleString()}</span>
                    </div>
                    {!health.simulation.continuousMode && (
                      <div className="col-span-2">
                        <span className="text-gray-500">Progress:</span>{' '}
                        <span className="font-medium">
                          {((health.simulation.currentStep / health.simulation.stepLimit) * 100).toFixed(1)}%
                        </span>
                        <div className="w-full bg-gray-200 rounded-full h-1 mt-1">
                          <div
                            className="bg-blue-600 h-1 rounded-full transition-all"
                            style={{ width: `${Math.min((health.simulation.currentStep / health.simulation.stepLimit) * 100, 100)}%` }}
                          />
                        </div>
                      </div>
                    )}
                  </div>
                </div>
              )}
            </div>
          </div>

          {/* Traffic Profiles */}
          <div className="bg-white rounded-lg shadow p-6">
            <h2 className="text-xl font-semibold text-gray-900 mb-4">Traffic Profiles</h2>

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
            </div>

            {/* Spawning Control */}
            <div className="mt-4 pt-4 border-t">
              <div className="flex items-center justify-between">
                <div>
                  <div className="font-medium">Vehicle Spawning</div>
                  <div className="text-sm text-gray-500">
                    {spawningStatus?.active
                      ? `Active with ${spawningStatus.flowRateCount} flow rates`
                      : 'Not active'}
                  </div>
                </div>
                <button
                  onClick={spawningStatus?.active ? handleStopSpawning : handleStartSpawning}
                  disabled={actionLoading !== null}
                  className={`px-4 py-2 rounded-md font-medium transition-colors disabled:opacity-50 ${
                    spawningStatus?.active
                      ? 'bg-orange-600 hover:bg-orange-700 text-white'
                      : 'bg-green-600 hover:bg-green-700 text-white'
                  }`}
                >
                  {actionLoading === 'startSpawn' || actionLoading === 'stopSpawn'
                    ? 'Loading...'
                    : spawningStatus?.active
                    ? 'Stop Spawning'
                    : 'Start Spawning'}
                </button>
              </div>
            </div>
          </div>

          {/* Continuous Optimization */}
          <div className="bg-white rounded-lg shadow p-6 lg:col-span-2">
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-xl font-semibold text-gray-900">Continuous Optimization</h2>
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
                    <div className="font-medium">Background GA Optimization</div>
                    <div className="text-sm text-gray-500">
                      Runs every {contOptStatus?.config.optimizationIntervalSeconds || 900}s with gradual application
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

                    {/* Prediction Status */}
                    {contOptStatus.prediction?.enabled && (
                      <div className="mt-3 pt-3 border-t border-gray-200">
                        <div className="text-xs text-gray-500 mb-1">Prediction Status</div>
                        <div className="grid grid-cols-2 gap-2 text-sm">
                          <div>
                            <span className="text-gray-500">Horizon:</span>{' '}
                            <span className="font-medium">{contOptStatus.prediction.horizonMinutes} min</span>
                          </div>
                          <div>
                            <span className="text-gray-500">Available:</span>{' '}
                            <span className={`font-medium ${contOptStatus.prediction.available ? 'text-green-600' : 'text-yellow-600'}`}>
                              {contOptStatus.prediction.available ? 'Yes' : 'No'}
                            </span>
                          </div>
                          {contOptStatus.prediction.averageAccuracy !== undefined && (
                            <div className="col-span-2">
                              <span className="text-gray-500">Avg Accuracy:</span>{' '}
                              <span className="font-medium">{(contOptStatus.prediction.averageAccuracy * 100).toFixed(1)}%</span>
                            </div>
                          )}
                        </div>
                      </div>
                    )}
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
