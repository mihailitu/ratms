import { http, HttpResponse } from 'msw';
import {
  mockSimulationStatus,
  mockRunningSimulationStatus,
  mockSimulations,
} from './data/simulations';
import { mockNetworks, mockRoads } from './data/networks';
import {
  mockOptimizationRuns,
  mockCompletedOptimizationRun,
  mockRunningOptimizationRun,
} from './data/optimization';
import { mockMetrics, mockSimulationStatistics, mockMetricTypes } from './data/metrics';

const BASE_URL = 'http://localhost:8080';

export const handlers = [
  // Health check
  http.get(`${BASE_URL}/api/health`, () => {
    return HttpResponse.json({
      service: 'RATMS API Server',
      version: '1.0.0',
      status: 'healthy',
    });
  }),

  // Simulation status
  http.get(`${BASE_URL}/api/simulation/status`, () => {
    return HttpResponse.json(mockSimulationStatus);
  }),

  // Start simulation
  http.post(`${BASE_URL}/api/simulation/start`, () => {
    return HttpResponse.json({
      message: 'Simulation started',
      status: 'running',
    });
  }),

  // Stop simulation
  http.post(`${BASE_URL}/api/simulation/stop`, () => {
    return HttpResponse.json({
      message: 'Simulation stopped',
      status: 'stopped',
    });
  }),

  // Get all simulations
  http.get(`${BASE_URL}/api/simulations`, () => {
    return HttpResponse.json(mockSimulations);
  }),

  // Get single simulation
  http.get(`${BASE_URL}/api/simulations/:id`, ({ params }) => {
    const { id } = params;
    const simulation = mockSimulations.find((s) => s.id === Number(id));
    if (!simulation) {
      return new HttpResponse(null, { status: 404 });
    }
    return HttpResponse.json(simulation);
  }),

  // Get simulation metrics
  http.get(`${BASE_URL}/api/simulations/:id/metrics`, () => {
    return HttpResponse.json(mockMetrics);
  }),

  // Get networks
  http.get(`${BASE_URL}/api/networks`, () => {
    return HttpResponse.json(mockNetworks);
  }),

  // Get roads
  http.get(`${BASE_URL}/api/simulation/roads`, () => {
    return HttpResponse.json({ roads: mockRoads });
  }),

  // Start optimization
  http.post(`${BASE_URL}/api/optimization/start`, async ({ request }) => {
    const body = await request.json();
    return HttpResponse.json({
      message: 'Optimization started',
      run: {
        id: mockOptimizationRuns.length + 1,
        startedAt: Math.floor(Date.now() / 1000),
        status: 'running',
        parameters: body,
        progress: {
          currentGeneration: 0,
          totalGenerations: (body as any).generations,
          percentComplete: 0,
        },
      },
    });
  }),

  // Get optimization status
  http.get(`${BASE_URL}/api/optimization/status/:runId`, ({ params }) => {
    const { runId } = params;
    const run = mockOptimizationRuns.find((r) => r.id === Number(runId));
    if (!run) {
      return new HttpResponse(null, { status: 404 });
    }
    return HttpResponse.json({ run });
  }),

  // Get optimization results
  http.get(`${BASE_URL}/api/optimization/results/:runId`, ({ params }) => {
    const { runId } = params;
    if (Number(runId) === mockCompletedOptimizationRun.id) {
      return HttpResponse.json({ run: mockCompletedOptimizationRun });
    }
    return new HttpResponse(null, { status: 404 });
  }),

  // Get optimization history
  http.get(`${BASE_URL}/api/optimization/history`, () => {
    return HttpResponse.json({ runs: mockOptimizationRuns });
  }),

  // Stop optimization
  http.post(`${BASE_URL}/api/optimization/stop/:runId`, ({ params }) => {
    const { runId } = params;
    return HttpResponse.json({
      message: 'Optimization stopped',
      run_id: Number(runId),
    });
  }),

  // Get simulation statistics
  http.get(`${BASE_URL}/api/analytics/simulations/:id/statistics/:metricType?`, () => {
    return HttpResponse.json(mockSimulationStatistics);
  }),

  // Compare simulations
  http.post(`${BASE_URL}/api/analytics/compare`, async ({ request }) => {
    const body = await request.json() as any;
    const simulationIds = body.simulation_ids;
    const metricType = body.metric_type;

    return HttpResponse.json({
      metric_type: metricType,
      simulations: simulationIds.map((simId: number) => ({
        simulation_id: simId,
        simulation_name: `Simulation ${simId}`,
        metrics: mockMetrics.filter((m) => m.metric_type === metricType),
      })),
    });
  }),

  // Export metrics (CSV)
  http.get(`${BASE_URL}/api/analytics/simulations/:id/export`, () => {
    const csvContent = 'timestamp,metric_type,value\n' +
      mockMetrics.map((m) => `${m.timestamp},${m.metric_type},${m.value}`).join('\n');
    return new HttpResponse(csvContent, {
      headers: {
        'Content-Type': 'text/csv',
        'Content-Disposition': 'attachment; filename="metrics.csv"',
      },
    });
  }),

  // Get metric types
  http.get(`${BASE_URL}/api/analytics/metric-types`, () => {
    return HttpResponse.json({ metric_types: mockMetricTypes });
  }),
];
