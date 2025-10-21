import type { SimulationRecord, SimulationStatus } from '../../../src/types/api';

export const mockSimulationStatus: SimulationStatus = {
  status: 'stopped',
  server_running: true,
  road_count: 12,
};

export const mockRunningSimulationStatus: SimulationStatus = {
  status: 'running',
  server_running: true,
  road_count: 12,
};

export const mockSimulations: SimulationRecord[] = [
  {
    id: 1,
    name: 'Test Simulation 1',
    description: 'First test simulation',
    network_id: 1,
    start_time: 1704067200, // 2024-01-01 00:00:00
    end_time: 1704070800,   // 2024-01-01 01:00:00
    duration_seconds: 3600,
    status: 'completed',
    config: '{"steps": 1000, "dt": 0.1}',
  },
  {
    id: 2,
    name: 'Test Simulation 2',
    description: 'Second test simulation',
    network_id: 1,
    start_time: 1704153600, // 2024-01-02 00:00:00
    end_time: 1704157200,   // 2024-01-02 01:00:00
    duration_seconds: 3600,
    status: 'completed',
    config: '{"steps": 2000, "dt": 0.1}',
  },
  {
    id: 3,
    name: 'Running Simulation',
    description: 'Currently running simulation',
    network_id: 1,
    start_time: Math.floor(Date.now() / 1000) - 600, // Started 10 mins ago
    end_time: 0,
    duration_seconds: 0,
    status: 'running',
    config: '{"steps": 5000, "dt": 0.1}',
  },
];
