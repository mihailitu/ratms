import { useState, useEffect, useCallback } from 'react';
import { apiClient } from '../services/apiClient';
import type { PredictionResult, ValidationConfig, RolloutState, ContinuousOptimizationStatus } from '../types/api';
import PipelineStatusIndicator from '../components/PipelineStatusIndicator';
import RolloutMonitor from '../components/RolloutMonitor';
import PredictionConfidenceChart from '../components/PredictionConfidenceChart';
import PredictionConfigPanel from '../components/PredictionConfigPanel';
import ValidationConfigPanel from '../components/ValidationConfigPanel';
import ForecastComparison from '../components/ForecastComparison';

interface ConfidenceHistoryPoint {
  timestamp: number;
  confidence: number;
}

interface ExtendedContOptStatus extends ContinuousOptimizationStatus {
  mode?: 'reactive' | 'predictive';
  prediction?: {
    enabled: boolean;
    available: boolean;
    horizonMinutes: number;
    pipelineStatus?: string;
    averageAccuracy?: number;
  };
}

export default function Statistics() {
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const [prediction, setPrediction] = useState<PredictionResult | null>(null);
  const [validationConfig, setValidationConfig] = useState<ValidationConfig | null>(null);
  const [rolloutState, setRolloutState] = useState<RolloutState | null>(null);
  const [contOptStatus, setContOptStatus] = useState<ExtendedContOptStatus | null>(null);

  const [confidenceHistory, setConfidenceHistory] = useState<ConfidenceHistoryPoint[]>([]);
  const [isRollingBack, setIsRollingBack] = useState(false);
  const [activeTab, setActiveTab] = useState<'overview' | 'forecast' | 'config'>('overview');

  const fetchData = useCallback(async () => {
    try {
      setError(null);

      const [predictionRes, validationRes, rolloutRes, contOptRes] = await Promise.allSettled([
        apiClient.getPredictionCurrent(),
        apiClient.getValidationConfig(),
        apiClient.getRolloutStatus(),
        apiClient.getContinuousOptimizationStatus(),
      ]);

      if (predictionRes.status === 'fulfilled' && predictionRes.value.success) {
        const predData = predictionRes.value.data;
        setPrediction(predData);

        if (predData.averageConfidence > 0) {
          setConfidenceHistory(prev => {
            const newPoint = {
              timestamp: predData.predictionTimestamp || Math.floor(Date.now() / 1000),
              confidence: predData.averageConfidence,
            };
            const updated = [...prev, newPoint].slice(-20);
            return updated;
          });
        }
      }

      if (validationRes.status === 'fulfilled' && validationRes.value.success) {
        setValidationConfig(validationRes.value.data);
      }

      if (rolloutRes.status === 'fulfilled' && rolloutRes.value.success) {
        setRolloutState(rolloutRes.value.data);
      }

      if (contOptRes.status === 'fulfilled' && contOptRes.value.success) {
        setContOptStatus(contOptRes.value.data as ExtendedContOptStatus);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch data');
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchData();
    const interval = setInterval(fetchData, 3000);
    return () => clearInterval(interval);
  }, [fetchData]);

  const handleRollback = async () => {
    setIsRollingBack(true);
    try {
      await apiClient.rollback();
      fetchData();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Rollback failed');
    } finally {
      setIsRollingBack(false);
    }
  };

  const getOptimizationSuccessRate = () => {
    if (!contOptStatus) return { successful: 0, total: 0, rate: 0 };
    const { totalOptimizationRuns, successfulOptimizations } = contOptStatus;
    const rate = totalOptimizationRuns > 0 ? (successfulOptimizations / totalOptimizationRuns) * 100 : 0;
    return { successful: successfulOptimizations, total: totalOptimizationRuns, rate };
  };

  const getPipelineStatus = () => {
    return contOptStatus?.prediction?.pipelineStatus || 'idle';
  };

  if (loading) {
    return (
      <div className="p-6">
        <div className="animate-pulse space-y-4">
          <div className="h-8 bg-gray-200 rounded w-1/4"></div>
          <div className="grid grid-cols-4 gap-4">
            {[1, 2, 3, 4].map(i => (
              <div key={i} className="h-24 bg-gray-200 rounded"></div>
            ))}
          </div>
          <div className="h-48 bg-gray-200 rounded"></div>
        </div>
      </div>
    );
  }

  const optStats = getOptimizationSuccessRate();

  return (
    <div className="p-6 space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-bold text-gray-900">Statistics Dashboard</h1>
          <p className="text-gray-500 text-sm mt-1">
            Real-time metrics for predictive optimization
          </p>
        </div>
        <div className="flex border border-gray-300 rounded-lg overflow-hidden">
          <button
            onClick={() => setActiveTab('overview')}
            className={`px-4 py-2 text-sm font-medium transition-colors ${
              activeTab === 'overview'
                ? 'bg-blue-600 text-white'
                : 'bg-white text-gray-600 hover:bg-gray-50'
            }`}
          >
            Overview
          </button>
          <button
            onClick={() => setActiveTab('forecast')}
            className={`px-4 py-2 text-sm font-medium transition-colors ${
              activeTab === 'forecast'
                ? 'bg-blue-600 text-white'
                : 'bg-white text-gray-600 hover:bg-gray-50'
            }`}
          >
            Forecast
          </button>
          <button
            onClick={() => setActiveTab('config')}
            className={`px-4 py-2 text-sm font-medium transition-colors ${
              activeTab === 'config'
                ? 'bg-blue-600 text-white'
                : 'bg-white text-gray-600 hover:bg-gray-50'
            }`}
          >
            Configuration
          </button>
        </div>
      </div>

      {error && (
        <div className="bg-red-50 border border-red-200 text-red-700 px-4 py-3 rounded">
          {error}
        </div>
      )}

      {activeTab === 'config' && (
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          <PredictionConfigPanel onConfigChange={fetchData} />
          <ValidationConfigPanel onConfigChange={fetchData} />
        </div>
      )}

      {activeTab === 'forecast' && (
        <div className="space-y-6">
          <ForecastComparison />
          {prediction && prediction.roadPredictions && prediction.roadPredictions.length > 0 && (
            <div className="bg-white rounded-lg shadow p-4">
              <h3 className="text-lg font-semibold text-gray-800 mb-4">
                Road Predictions
                <span className="text-sm font-normal text-gray-500 ml-2">
                  ({prediction.roadPredictions.length} roads)
                </span>
              </h3>
              <div className="overflow-x-auto">
                <table className="min-w-full divide-y divide-gray-200">
                  <thead className="bg-gray-50">
                    <tr>
                      <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Road</th>
                      <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Vehicles</th>
                      <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Queue</th>
                      <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Avg Speed</th>
                      <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase">Confidence</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-gray-200">
                    {prediction.roadPredictions.slice(0, 20).map(road => (
                      <tr key={road.roadId}>
                        <td className="px-3 py-2 text-sm text-gray-900">Road {road.roadId}</td>
                        <td className="px-3 py-2 text-sm text-gray-700">{road.vehicleCount.toFixed(1)}</td>
                        <td className="px-3 py-2 text-sm text-gray-700">{road.queueLength.toFixed(1)}</td>
                        <td className="px-3 py-2 text-sm text-gray-700">{road.avgSpeed.toFixed(1)} m/s</td>
                        <td className="px-3 py-2 text-sm">
                          <span className={`font-medium ${
                            road.confidence >= 0.7 ? 'text-green-600' :
                            road.confidence >= 0.4 ? 'text-yellow-600' : 'text-red-600'
                          }`}>
                            {(road.confidence * 100).toFixed(0)}%
                          </span>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
                {prediction.roadPredictions.length > 20 && (
                  <p className="text-xs text-gray-500 mt-2 px-3">
                    Showing 20 of {prediction.roadPredictions.length} roads
                  </p>
                )}
              </div>
            </div>
          )}
        </div>
      )}

      {activeTab === 'overview' && (
        <>
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
        <div className="bg-white rounded-lg shadow p-4 border-l-4 border-blue-500">
          <div className="text-sm text-gray-500">Prediction Confidence</div>
          <div className="text-2xl font-bold text-blue-600">
            {prediction?.averageConfidence
              ? `${(prediction.averageConfidence * 100).toFixed(1)}%`
              : 'N/A'}
          </div>
          <div className="text-xs text-gray-400 mt-1">
            {prediction?.targetTimeSlotString || 'No prediction'}
          </div>
        </div>

        <div className="bg-white rounded-lg shadow p-4 border-l-4 border-green-500">
          <div className="text-sm text-gray-500">Validation</div>
          <div className="text-2xl font-bold text-green-600">
            {validationConfig?.enabled ? 'Enabled' : 'Disabled'}
          </div>
          <div className="text-xs text-gray-400 mt-1">
            Threshold: {validationConfig?.improvementThreshold || 5}% improvement
          </div>
        </div>

        <div className="bg-white rounded-lg shadow p-4 border-l-4 border-yellow-500">
          <div className="text-sm text-gray-500">Rollout Status</div>
          <div className="text-2xl font-bold text-yellow-600 capitalize">
            {rolloutState?.status || 'Idle'}
          </div>
          <div className="text-xs text-gray-400 mt-1">
            {rolloutState?.status === 'in_progress'
              ? `${rolloutState.updateCount} updates`
              : rolloutState?.regressionPercent
                ? `Last: ${rolloutState.regressionPercent.toFixed(1)}% regression`
                : 'No active rollout'}
          </div>
        </div>

        <div className="bg-white rounded-lg shadow p-4 border-l-4 border-purple-500">
          <div className="text-sm text-gray-500">Optimization Runs</div>
          <div className="text-2xl font-bold text-purple-600">
            {optStats.successful}/{optStats.total}
          </div>
          <div className="text-xs text-gray-400 mt-1">
            Success rate: {optStats.rate.toFixed(0)}%
            {contOptStatus?.lastImprovementPercent
              ? ` | Last: +${contOptStatus.lastImprovementPercent.toFixed(1)}%`
              : ''}
          </div>
        </div>
      </div>

      <PipelineStatusIndicator status={getPipelineStatus()} />

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <PredictionConfidenceChart history={confidenceHistory} />

        {rolloutState && (
          <RolloutMonitor
            rolloutState={rolloutState}
            onRollback={handleRollback}
            isRollingBack={isRollingBack}
          />
        )}
      </div>

      {contOptStatus && (
        <div className="bg-white rounded-lg shadow p-4">
          <h3 className="text-lg font-semibold text-gray-800 mb-4">Continuous Optimization Status</h3>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
            <div>
              <span className="text-sm text-gray-500">Status</span>
              <div className="flex items-center gap-2 mt-1">
                <div className={`w-2 h-2 rounded-full ${contOptStatus.running ? 'bg-green-500 animate-pulse' : 'bg-gray-400'}`}></div>
                <span className="font-medium">{contOptStatus.running ? 'Running' : 'Stopped'}</span>
              </div>
            </div>
            <div>
              <span className="text-sm text-gray-500">Mode</span>
              <div className="font-medium mt-1 capitalize">{contOptStatus.mode || 'reactive'}</div>
            </div>
            <div>
              <span className="text-sm text-gray-500">Next Optimization</span>
              <div className="font-medium mt-1">
                {contOptStatus.running
                  ? `${contOptStatus.nextOptimizationIn}s`
                  : '-'}
              </div>
            </div>
            <div>
              <span className="text-sm text-gray-500">Active Transitions</span>
              <div className="font-medium mt-1">{contOptStatus.activeTransitions?.length || 0}</div>
            </div>
          </div>

          {contOptStatus.prediction?.enabled && (
            <div className="mt-4 pt-4 border-t border-gray-200">
              <h4 className="text-sm font-medium text-gray-700 mb-2">Prediction Settings</h4>
              <div className="grid grid-cols-2 md:grid-cols-3 gap-4 text-sm">
                <div>
                  <span className="text-gray-500">Horizon:</span>
                  <span className="ml-1 font-medium">{contOptStatus.prediction.horizonMinutes} min</span>
                </div>
                <div>
                  <span className="text-gray-500">Available:</span>
                  <span className="ml-1 font-medium">{contOptStatus.prediction.available ? 'Yes' : 'No'}</span>
                </div>
                {contOptStatus.prediction.averageAccuracy !== undefined && (
                  <div>
                    <span className="text-gray-500">Avg Accuracy:</span>
                    <span className="ml-1 font-medium">
                      {(contOptStatus.prediction.averageAccuracy * 100).toFixed(1)}%
                    </span>
                  </div>
                )}
              </div>
            </div>
          )}
        </div>
      )}
        </>
      )}
    </div>
  );
}
