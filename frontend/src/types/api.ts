export interface HealthResponse {
  status: string;
  service: string;
  version: string;
  timestamp: number;
}

export interface SimulationStatus {
  status: 'running' | 'stopped';
  simulator_initialized: boolean;
  server_running: boolean;
  timestamp: number;
  road_count?: number;
}

export interface SimulationStartResponse {
  message: string;
  status: string;
  simulation_id: number;
  timestamp: number;
}

export interface SimulationStopResponse {
  message: string;
  status: string;
  timestamp: number;
}

export interface SimulationRecord {
  id: number;
  name: string;
  description: string;
  network_id: number;
  status: 'pending' | 'running' | 'completed' | 'failed';
  start_time: number;
  end_time: number;
  duration_seconds: number;
  config?: string;
}

export interface MetricRecord {
  id: number;
  simulation_id: number;
  timestamp: number;
  metric_type: string;
  road_id: number;
  value: number;
  unit: string;
}

export interface NetworkRecord {
  id: number;
  name: string;
  description: string;
  road_count: number;
  intersection_count: number;
  config?: string;
}

// Optimization API types
export interface GAParameters {
  populationSize: number;
  generations: number;
  mutationRate: number;
  mutationStdDev?: number;
  crossoverRate: number;
  tournamentSize?: number;
  elitismRate: number;
  minGreenTime?: number;
  maxGreenTime?: number;
  minRedTime?: number;
  maxRedTime?: number;
  simulationSteps: number;
  dt: number;
}

export interface TrafficLightGene {
  greenTime: number;
  redTime: number;
}

export interface OptimizationProgress {
  currentGeneration: number;
  totalGenerations: number;
  percentComplete: number;
}

export interface OptimizationResults {
  baselineFitness: number;
  bestFitness: number;
  improvementPercent: number;
  completedAt: number;
  durationSeconds: number;
  bestChromosome: TrafficLightGene[];
  fitnessHistory?: number[];
  fitnessHistorySample?: number[];
}

export interface OptimizationRun {
  id: number;
  status: 'pending' | 'running' | 'completed' | 'failed' | 'stopped';
  parameters: GAParameters;
  progress: OptimizationProgress;
  startedAt: number;
  results?: OptimizationResults;
}

export interface StartOptimizationRequest {
  populationSize?: number;
  generations?: number;
  mutationRate?: number;
  mutationStdDev?: number;
  crossoverRate?: number;
  tournamentSize?: number;
  elitismRate?: number;
  minGreenTime?: number;
  maxGreenTime?: number;
  minRedTime?: number;
  maxRedTime?: number;
  simulationSteps?: number;
  dt?: number;
  networkId?: number;
}

export interface StartOptimizationResponse {
  success: boolean;
  runId: number;
  message: string;
  run: OptimizationRun;
}

export interface OptimizationStatusResponse {
  success: boolean;
  run: OptimizationRun;
}

export interface OptimizationResultsResponse {
  success: boolean;
  run: OptimizationRun;
}

export interface OptimizationHistoryResponse {
  success: boolean;
  runs: OptimizationRun[];
  total: number;
}

export interface StopOptimizationResponse {
  success: boolean;
  message: string;
}
