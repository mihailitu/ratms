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
