import type {
  SimulationRecord,
  NetworkRecord,
  OptimizationRun,
  MetricRecord,
} from '../../src/types/api';

/**
 * Create a mock simulation record
 */
export function createMockSimulation(overrides?: Partial<SimulationRecord>): SimulationRecord {
  return {
    id: Math.floor(Math.random() * 1000),
    name: `Test Simulation ${Date.now()}`,
    description: 'Auto-generated test simulation',
    network_id: 1,
    start_time: Math.floor(Date.now() / 1000) - 3600,
    end_time: Math.floor(Date.now() / 1000),
    duration_seconds: 3600,
    status: 'completed',
    config: '{"steps": 1000, "dt": 0.1}',
    ...overrides,
  };
}

/**
 * Create a mock network record
 */
export function createMockNetwork(overrides?: Partial<NetworkRecord>): NetworkRecord {
  return {
    id: Math.floor(Math.random() * 1000),
    name: `Test Network ${Date.now()}`,
    description: 'Auto-generated test network',
    road_count: 10,
    intersection_count: 5,
    created_at: Math.floor(Date.now() / 1000),
    ...overrides,
  };
}

/**
 * Create a mock optimization run
 */
export function createMockOptimizationRun(
  overrides?: Partial<OptimizationRun>
): OptimizationRun {
  return {
    id: Math.floor(Math.random() * 1000),
    startedAt: Math.floor(Date.now() / 1000),
    status: 'completed',
    parameters: {
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
    },
    progress: {
      currentGeneration: 50,
      totalGenerations: 50,
      percentComplete: 100,
    },
    ...overrides,
  };
}

/**
 * Create mock metric records
 */
export function createMockMetrics(
  count: number,
  metricType: string = 'avg_queue_length'
): MetricRecord[] {
  return Array.from({ length: count }, (_, i) => ({
    id: i + 1,
    simulation_id: 1,
    timestamp: i * 10,
    metric_type: metricType,
    value: Math.random() * 10,
  }));
}

/**
 * Create a batch of mock simulations
 */
export function createMockSimulationBatch(count: number): SimulationRecord[] {
  return Array.from({ length: count }, (_, i) =>
    createMockSimulation({
      id: i + 1,
      name: `Simulation ${i + 1}`,
    })
  );
}
