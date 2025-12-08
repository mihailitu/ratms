import { useEffect, useState } from 'react';
import { apiClient } from '../services/apiClient';
import type {
  OptimizationRun,
  StartOptimizationRequest
} from '../types/api';
import GAVisualizer from '../components/GAVisualizer';

export default function Optimization() {
  const [runs, setRuns] = useState<OptimizationRun[]>([]);
  const [selectedRun, setSelectedRun] = useState<OptimizationRun | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [startingOptimization, setStartingOptimization] = useState(false);

  // Form state for starting new optimization
  const [formData, setFormData] = useState<StartOptimizationRequest>({
    populationSize: 30,
    generations: 50,
    mutationRate: 0.15,
    mutationStdDev: 5.0,
    crossoverRate: 0.8,
    tournamentSize: 3,
    elitismRate: 0.1,
    minGreenTime: 10.0,
    maxGreenTime: 60.0,
    minRedTime: 10.0,
    maxRedTime: 60.0,
    simulationSteps: 1000,
    dt: 0.1,
    networkId: 1,
  });

  const fetchHistory = async () => {
    try {
      setError(null);
      const response = await apiClient.getOptimizationHistory();
      setRuns(response.runs);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch optimization history');
    } finally {
      setLoading(false);
    }
  };

  const fetchRunStatus = async (runId: number) => {
    try {
      const response = await apiClient.getOptimizationStatus(runId);
      // Update the run in the list
      setRuns(prev => prev.map(run => run.id === runId ? response.run : run));
      if (selectedRun?.id === runId) {
        setSelectedRun(response.run);
      }
    } catch (err) {
      console.error(`Failed to fetch status for run ${runId}:`, err);
    }
  };

  useEffect(() => {
    fetchHistory();
  }, []);

  // Poll for updates on running optimizations
  useEffect(() => {
    const runningRuns = runs.filter(run => run.status === 'running');
    if (runningRuns.length === 0) return;

    const interval = setInterval(() => {
      runningRuns.forEach(run => fetchRunStatus(run.id));
    }, 2000); // Poll every 2 seconds

    return () => clearInterval(interval);
  }, [runs]);

  const handleStartOptimization = async (e: React.FormEvent) => {
    e.preventDefault();
    setStartingOptimization(true);
    setError(null);

    try {
      const response = await apiClient.startOptimization(formData);
      setRuns(prev => [response.run, ...prev]);
      setSelectedRun(response.run);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to start optimization');
    } finally {
      setStartingOptimization(false);
    }
  };

  const handleStopOptimization = async (runId: number) => {
    try {
      await apiClient.stopOptimization(runId);
      await fetchRunStatus(runId);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to stop optimization');
    }
  };

  const handleViewResults = async (run: OptimizationRun) => {
    if (run.status === 'completed') {
      try {
        const response = await apiClient.getOptimizationResults(run.id);
        setSelectedRun(response.run);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch results');
      }
    } else {
      setSelectedRun(run);
    }
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'running': return 'text-blue-600';
      case 'completed': return 'text-green-600';
      case 'failed': return 'text-red-600';
      case 'stopped': return 'text-yellow-600';
      default: return 'text-gray-600';
    }
  };

  const getStatusBadgeColor = (status: string) => {
    switch (status) {
      case 'running': return 'bg-blue-100 text-blue-800';
      case 'completed': return 'bg-green-100 text-green-800';
      case 'failed': return 'bg-red-100 text-red-800';
      case 'stopped': return 'bg-yellow-100 text-yellow-800';
      default: return 'bg-gray-100 text-gray-800';
    }
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center h-screen">
        <div className="text-xl">Loading...</div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="max-w-7xl mx-auto px-4 py-8">
        {/* Header */}
        <div className="mb-8">
          <h1 className="text-4xl font-bold text-gray-900 mb-2">Traffic Light Optimization</h1>
          <p className="text-gray-600">Genetic Algorithm-based optimization for traffic signal timing</p>
        </div>

        {error && (
          <div className="mb-6 bg-red-50 border border-red-200 text-red-700 px-4 py-3 rounded">
            {error}
          </div>
        )}

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Left Column: Configuration & History */}
          <div className="space-y-6">
            {/* Start New Optimization */}
            <div className="bg-white rounded-lg shadow">
              <div className="px-6 py-4 border-b border-gray-200">
                <h2 className="text-xl font-semibold text-gray-900">Start New Optimization</h2>
              </div>
              <div className="p-6">
                <form onSubmit={handleStartOptimization} className="space-y-4">
                  <div className="grid grid-cols-2 gap-4">
                    <div>
                      <label htmlFor="populationSize" className="block text-sm font-medium text-gray-700 mb-1">
                        Population Size
                      </label>
                      <input
                        id="populationSize"
                        type="number"
                        value={formData.populationSize}
                        onChange={(e) => setFormData({...formData, populationSize: parseInt(e.target.value)})}
                        className="w-full px-3 py-2 border border-gray-300 rounded-md"
                        min="10"
                        max="200"
                      />
                    </div>
                    <div>
                      <label htmlFor="generations" className="block text-sm font-medium text-gray-700 mb-1">
                        Generations
                      </label>
                      <input
                        id="generations"
                        type="number"
                        value={formData.generations}
                        onChange={(e) => setFormData({...formData, generations: parseInt(e.target.value)})}
                        className="w-full px-3 py-2 border border-gray-300 rounded-md"
                        min="10"
                        max="500"
                      />
                    </div>
                  </div>

                  <div className="grid grid-cols-2 gap-4">
                    <div>
                      <label htmlFor="mutationRate" className="block text-sm font-medium text-gray-700 mb-1">
                        Mutation Rate
                      </label>
                      <input
                        id="mutationRate"
                        type="number"
                        step="0.01"
                        value={formData.mutationRate}
                        onChange={(e) => setFormData({...formData, mutationRate: parseFloat(e.target.value)})}
                        className="w-full px-3 py-2 border border-gray-300 rounded-md"
                        min="0"
                        max="1"
                      />
                    </div>
                    <div>
                      <label htmlFor="crossoverRate" className="block text-sm font-medium text-gray-700 mb-1">
                        Crossover Rate
                      </label>
                      <input
                        id="crossoverRate"
                        type="number"
                        step="0.01"
                        value={formData.crossoverRate}
                        onChange={(e) => setFormData({...formData, crossoverRate: parseFloat(e.target.value)})}
                        className="w-full px-3 py-2 border border-gray-300 rounded-md"
                        min="0"
                        max="1"
                      />
                    </div>
                  </div>

                  <div className="grid grid-cols-2 gap-4">
                    <div>
                      <label htmlFor="simulationSteps" className="block text-sm font-medium text-gray-700 mb-1">
                        Simulation Steps
                      </label>
                      <input
                        id="simulationSteps"
                        type="number"
                        value={formData.simulationSteps}
                        onChange={(e) => setFormData({...formData, simulationSteps: parseInt(e.target.value)})}
                        className="w-full px-3 py-2 border border-gray-300 rounded-md"
                        min="100"
                        max="10000"
                      />
                    </div>
                    <div>
                      <label htmlFor="networkId" className="block text-sm font-medium text-gray-700 mb-1">
                        Network ID
                      </label>
                      <input
                        id="networkId"
                        type="number"
                        value={formData.networkId}
                        onChange={(e) => setFormData({...formData, networkId: parseInt(e.target.value)})}
                        className="w-full px-3 py-2 border border-gray-300 rounded-md"
                        min="1"
                      />
                    </div>
                  </div>

                  <button
                    type="submit"
                    disabled={startingOptimization}
                    className="w-full bg-blue-600 text-white px-4 py-2 rounded-md hover:bg-blue-700 disabled:bg-gray-400 disabled:cursor-not-allowed transition-colors"
                  >
                    {startingOptimization ? 'Starting...' : 'Start Optimization'}
                  </button>
                </form>
              </div>
            </div>

            {/* Optimization History */}
            <div className="bg-white rounded-lg shadow">
              <div className="px-6 py-4 border-b border-gray-200">
                <h2 className="text-xl font-semibold text-gray-900">Optimization History</h2>
              </div>
              <div className="p-6">
                {runs.length === 0 ? (
                  <p className="text-gray-500">No optimization runs yet</p>
                ) : (
                  <div className="space-y-3">
                    {runs.map((run) => (
                      <div
                        key={run.id}
                        className={`border rounded-lg p-4 cursor-pointer transition-colors ${
                          selectedRun?.id === run.id
                            ? 'border-blue-500 bg-blue-50'
                            : 'border-gray-200 hover:border-gray-300'
                        }`}
                        onClick={() => handleViewResults(run)}
                      >
                        <div className="flex justify-between items-start mb-2">
                          <div className="flex items-center gap-2">
                            <span className="font-semibold text-gray-900">Run #{run.id}</span>
                            <span className={`text-xs px-2 py-1 rounded ${getStatusBadgeColor(run.status)}`}>
                              {run.status}
                            </span>
                          </div>
                          {run.status === 'running' && (
                            <button
                              onClick={(e) => {
                                e.stopPropagation();
                                handleStopOptimization(run.id);
                              }}
                              className="text-xs text-red-600 hover:text-red-700"
                            >
                              Stop
                            </button>
                          )}
                        </div>

                        {run.status === 'running' && (
                          <div className="mb-2">
                            <div className="flex justify-between text-xs text-gray-600 mb-1">
                              <span>Generation {run.progress.currentGeneration} / {run.progress.totalGenerations}</span>
                              <span>{run.progress.percentComplete.toFixed(1)}%</span>
                            </div>
                            <div className="w-full bg-gray-200 rounded-full h-2">
                              <div
                                className="bg-blue-600 h-2 rounded-full transition-all"
                                style={{ width: `${run.progress.percentComplete}%` }}
                              />
                            </div>
                          </div>
                        )}

                        {run.status === 'completed' && run.results && (
                          <div className="text-sm text-gray-600">
                            <div>Improvement: <span className="font-semibold text-green-600">
                              {run.results.improvementPercent.toFixed(1)}%
                            </span></div>
                            <div>Best Fitness: {run.results.bestFitness.toFixed(2)}</div>
                          </div>
                        )}

                        <div className="text-xs text-gray-500 mt-2">
                          Started: {new Date(run.startedAt * 1000).toLocaleString()}
                        </div>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            </div>
          </div>

          {/* Right Column: Results & Visualization */}
          <div>
            {selectedRun ? (
              <div className="bg-white rounded-lg shadow">
                <div className="px-6 py-4 border-b border-gray-200">
                  <div className="flex justify-between items-center">
                    <h2 className="text-xl font-semibold text-gray-900">
                      Run #{selectedRun.id} Details
                    </h2>
                    <span className={`text-sm font-semibold ${getStatusColor(selectedRun.status)}`}>
                      {selectedRun.status.toUpperCase()}
                    </span>
                  </div>
                </div>
                <div className="p-6 space-y-6">
                  {/* Parameters */}
                  <div>
                    <h3 className="text-sm font-semibold text-gray-700 mb-2">Parameters</h3>
                    <div className="grid grid-cols-2 gap-2 text-sm">
                      <div className="text-gray-600">Population: {selectedRun.parameters.populationSize}</div>
                      <div className="text-gray-600">Generations: {selectedRun.parameters.generations}</div>
                      <div className="text-gray-600">Mutation: {selectedRun.parameters.mutationRate}</div>
                      <div className="text-gray-600">Crossover: {selectedRun.parameters.crossoverRate}</div>
                      <div className="text-gray-600">Steps: {selectedRun.parameters.simulationSteps}</div>
                      <div className="text-gray-600">DT: {selectedRun.parameters.dt}</div>
                    </div>
                  </div>

                  {/* Results */}
                  {selectedRun.status === 'completed' && selectedRun.results && (
                    <>
                      <div>
                        <h3 className="text-sm font-semibold text-gray-700 mb-2">Results</h3>
                        <div className="space-y-2">
                          <div className="flex justify-between text-sm">
                            <span className="text-gray-600">Baseline Fitness:</span>
                            <span className="font-semibold">{selectedRun.results.baselineFitness.toFixed(2)}</span>
                          </div>
                          <div className="flex justify-between text-sm">
                            <span className="text-gray-600">Best Fitness:</span>
                            <span className="font-semibold text-green-600">{selectedRun.results.bestFitness.toFixed(2)}</span>
                          </div>
                          <div className="flex justify-between text-sm">
                            <span className="text-gray-600">Improvement:</span>
                            <span className="font-semibold text-green-600">
                              {selectedRun.results.improvementPercent.toFixed(1)}%
                            </span>
                          </div>
                          <div className="flex justify-between text-sm">
                            <span className="text-gray-600">Duration:</span>
                            <span className="font-semibold">{selectedRun.results.durationSeconds}s</span>
                          </div>
                        </div>
                      </div>

                      {/* GA Visualizer */}
                      <GAVisualizer run={selectedRun} />
                    </>
                  )}

                  {/* Running progress */}
                  {selectedRun.status === 'running' && (
                    <div>
                      <h3 className="text-sm font-semibold text-gray-700 mb-2">Progress</h3>
                      <div className="text-sm text-gray-600">
                        <div>Generation: {selectedRun.progress.currentGeneration} / {selectedRun.progress.totalGenerations}</div>
                        <div className="mt-2 w-full bg-gray-200 rounded-full h-3">
                          <div
                            className="bg-blue-600 h-3 rounded-full transition-all"
                            style={{ width: `${selectedRun.progress.percentComplete}%` }}
                          />
                        </div>
                      </div>
                    </div>
                  )}
                </div>
              </div>
            ) : (
              <div className="bg-white rounded-lg shadow p-12 text-center">
                <p className="text-gray-500">Select an optimization run to view details</p>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
