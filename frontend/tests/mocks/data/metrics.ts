import type { MetricRecord, SimulationStatistics } from '../../../src/types/api';

export const mockMetrics: MetricRecord[] = Array.from({ length: 100 }, (_, i) => ({
  id: i + 1,
  simulation_id: 1,
  timestamp: i * 10, // Every 10 seconds
  metric_type: 'avg_queue_length',
  value: 2.5 + Math.sin(i / 10) * 1.5 + Math.random() * 0.5,
}));

export const mockSimulationStatistics: SimulationStatistics = {
  simulation_id: 1,
  simulation_name: 'Test Simulation 1',
  statistics: {
    avg_queue_length: {
      metric_type: 'avg_queue_length',
      min_value: 0.5,
      max_value: 4.8,
      mean_value: 2.6,
      median_value: 2.5,
      stddev_value: 0.8,
      p25_value: 2.0,
      p75_value: 3.2,
      p95_value: 4.2,
      sample_count: 100,
    },
    avg_vehicle_speed: {
      metric_type: 'avg_vehicle_speed',
      min_value: 8.2,
      max_value: 14.8,
      mean_value: 12.3,
      median_value: 12.5,
      stddev_value: 1.2,
      p25_value: 11.5,
      p75_value: 13.2,
      p95_value: 14.2,
      sample_count: 100,
    },
  },
};

export const mockMetricTypes = [
  'avg_queue_length',
  'avg_vehicle_speed',
  'vehicles_exited',
  'max_queue_length',
];
