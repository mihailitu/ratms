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

// Real-time streaming types
export interface VehiclePosition {
  id: number;
  roadId: number;
  lane: number;
  position: number;
  velocity: number;
  acceleration: number;
}

export interface TrafficLightState {
  roadId: number;
  lane: number;
  state: 'R' | 'Y' | 'G';
  lat: number;
  lon: number;
}

export interface SimulationUpdate {
  step: number;
  time: number;
  vehicles: VehiclePosition[];
  trafficLights: TrafficLightState[];
}

// Road geometry types
export interface RoadGeometry {
  id: number;
  length: number;
  maxSpeed: number;
  lanes: number;
  startLat: number;
  startLon: number;
  endLat: number;
  endLon: number;
}

export interface RoadsResponse {
  roads: RoadGeometry[];
  count: number;
}

// Analytics types
export interface MetricStatistics {
  metric_type: string;
  min_value: number;
  max_value: number;
  mean_value: number;
  median_value: number;
  stddev_value: number;
  p25_value: number;
  p75_value: number;
  p95_value: number;
  sample_count: number;
}

export interface SimulationStatistics {
  simulation_id: number;
  simulation_name: string;
  statistics: Record<string, MetricStatistics>;
}

export interface ComparativeMetric {
  simulation_id: number;
  simulation_name: string;
  metrics: Array<{
    timestamp: number;
    value: number;
  }>;
}

export interface ComparisonResponse {
  metric_type: string;
  simulations: ComparativeMetric[];
}

export interface MetricTypesResponse {
  metric_types: string[];
}

// Traffic Light Configuration (Stage 2)
export interface TrafficLightConfig {
  roadId: number;
  lane: number;
  greenTime: number;
  yellowTime: number;
  redTime: number;
  currentState: 'G' | 'Y' | 'R';
}

export interface TrafficLightsResponse {
  trafficLights: TrafficLightConfig[];
  count: number;
}

export interface TrafficLightUpdate {
  roadId: number;
  lane: number;
  greenTime: number;
  yellowTime: number;
  redTime: number;
}

export interface SetTrafficLightsRequest {
  updates: TrafficLightUpdate[];
}

export interface SetTrafficLightsResponse {
  success: boolean;
  updated: number;
}

// Spawn Rate Configuration (Stage 3)
export interface SpawnRateConfig {
  roadId: number;
  vehiclesPerMinute: number;
}

export interface SpawnRatesResponse {
  rates: SpawnRateConfig[];
  count: number;
}

export interface SetSpawnRatesRequest {
  rates: SpawnRateConfig[];
}

export interface SetSpawnRatesResponse {
  success: boolean;
  updated: number;
}

// Visualization types
export type MapViewMode = 'speed' | 'density';
