import { Page } from '@playwright/test';
import {
  mockOptimizationRuns,
  mockCompletedOptimizationRun,
  mockRunningOptimizationRun,
} from '../mocks/data/optimization';
import { mockSimulations, mockSimulationStatus } from '../mocks/data/simulations';
import { mockNetworks, mockRoads } from '../mocks/data/networks';
import { mockMetrics, mockMetricTypes, mockSimulationStatistics } from '../mocks/data/metrics';

const BASE_URL = 'http://localhost:8080';

/**
 * Setup all API mocks for a page using Playwright's route() API
 * This intercepts requests at the browser level, unlike MSW Node which only works in Node.js
 */
export async function setupApiMocks(page: Page) {
  // Health check
  await page.route(`${BASE_URL}/api/health`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({
        service: 'RATMS API Server',
        version: '1.0.0',
        status: 'healthy',
      }),
    });
  });

  // Simulation status
  await page.route(`${BASE_URL}/api/simulation/status`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(mockSimulationStatus),
    });
  });

  // Start simulation
  await page.route(`${BASE_URL}/api/simulation/start`, async (route) => {
    if (route.request().method() === 'POST') {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          message: 'Simulation started',
          status: 'running',
        }),
      });
    } else {
      await route.continue();
    }
  });

  // Stop simulation
  await page.route(`${BASE_URL}/api/simulation/stop`, async (route) => {
    if (route.request().method() === 'POST') {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          message: 'Simulation stopped',
          status: 'stopped',
        }),
      });
    } else {
      await route.continue();
    }
  });

  // Get all simulations
  await page.route(`${BASE_URL}/api/simulations`, async (route) => {
    // Only handle GET requests to /api/simulations (not /api/simulations/:id)
    if (route.request().url() === `${BASE_URL}/api/simulations`) {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify(mockSimulations),
      });
    } else {
      await route.continue();
    }
  });

  // Get single simulation
  await page.route(`${BASE_URL}/api/simulations/*`, async (route) => {
    const url = route.request().url();
    const match = url.match(/\/api\/simulations\/(\d+)/);
    if (match) {
      const id = parseInt(match[1]);
      const simulation = mockSimulations.find((s) => s.id === id);
      if (simulation) {
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify(simulation),
        });
      } else {
        await route.fulfill({ status: 404 });
      }
    } else {
      await route.continue();
    }
  });

  // Get simulation metrics
  await page.route(`${BASE_URL}/api/simulations/*/metrics`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(mockMetrics),
    });
  });

  // Get networks
  await page.route(`${BASE_URL}/api/networks`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(mockNetworks),
    });
  });

  // Get roads
  await page.route(`${BASE_URL}/api/simulation/roads*`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ roads: mockRoads }),
    });
  });

  // Get metric types
  await page.route(`${BASE_URL}/api/analytics/metric-types`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ metric_types: mockMetricTypes }),
    });
  });

  // Get simulation statistics
  await page.route(`${BASE_URL}/api/analytics/simulations/*/statistics*`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(mockSimulationStatistics),
    });
  });

  // Compare simulations
  await page.route(`${BASE_URL}/api/analytics/compare`, async (route) => {
    if (route.request().method() === 'POST') {
      const body = JSON.parse(route.request().postData() || '{}');
      const simulationIds = body.simulation_ids || [];
      const metricType = body.metric_type || 'avg_queue_length';

      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          metric_type: metricType,
          simulations: simulationIds.map((simId: number) => ({
            simulation_id: simId,
            simulation_name: `Simulation ${simId}`,
            metrics: mockMetrics.filter((m) => m.metric_type === metricType),
          })),
        }),
      });
    } else {
      await route.continue();
    }
  });

  // Export metrics (CSV)
  await page.route(`${BASE_URL}/api/analytics/simulations/*/export`, async (route) => {
    const csvContent = 'timestamp,metric_type,value\n' +
      mockMetrics.map((m) => `${m.timestamp},${m.metric_type},${m.value}`).join('\n');
    await route.fulfill({
      status: 200,
      contentType: 'text/csv',
      headers: {
        'Content-Disposition': 'attachment; filename="metrics.csv"',
      },
      body: csvContent,
    });
  });
}

/**
 * Setup optimization API mocks
 */
export async function setupOptimizationMocks(page: Page) {
  // Optimization history
  await page.route(`${BASE_URL}/api/optimization/history`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ runs: mockOptimizationRuns }),
    });
  });

  // Start optimization
  await page.route(`${BASE_URL}/api/optimization/start`, async (route) => {
    if (route.request().method() === 'POST') {
      const body = JSON.parse(route.request().postData() || '{}');
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          message: 'Optimization started',
          run: {
            id: mockOptimizationRuns.length + 1,
            startedAt: Math.floor(Date.now() / 1000),
            status: 'running',
            parameters: body,
            progress: {
              currentGeneration: 0,
              totalGenerations: body.generations || 50,
              percentComplete: 0,
            },
          },
        }),
      });
    } else {
      await route.continue();
    }
  });

  // Get optimization status
  await page.route(`${BASE_URL}/api/optimization/status/*`, async (route) => {
    const url = route.request().url();
    const match = url.match(/\/api\/optimization\/status\/(\d+)/);
    if (match) {
      const runId = parseInt(match[1]);
      const run = mockOptimizationRuns.find((r) => r.id === runId);
      if (run) {
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify({ run }),
        });
      } else {
        await route.fulfill({ status: 404 });
      }
    } else {
      await route.continue();
    }
  });

  // Get optimization results
  await page.route(`${BASE_URL}/api/optimization/results/*`, async (route) => {
    const url = route.request().url();
    const match = url.match(/\/api\/optimization\/results\/(\d+)/);
    if (match) {
      const runId = parseInt(match[1]);
      if (runId === mockCompletedOptimizationRun.id) {
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify({ run: mockCompletedOptimizationRun }),
        });
      } else {
        await route.fulfill({ status: 404 });
      }
    } else {
      await route.continue();
    }
  });

  // Stop optimization
  await page.route(`${BASE_URL}/api/optimization/stop/*`, async (route) => {
    if (route.request().method() === 'POST') {
      const url = route.request().url();
      const match = url.match(/\/api\/optimization\/stop\/(\d+)/);
      const runId = match ? parseInt(match[1]) : 0;
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          message: 'Optimization stopped',
          run_id: runId,
        }),
      });
    } else {
      await route.continue();
    }
  });
}

/**
 * Setup all mocks including optimization
 */
export async function setupAllMocks(page: Page) {
  await setupApiMocks(page);
  await setupOptimizationMocks(page);
}

/**
 * Override optimization history with empty runs
 */
export async function mockEmptyOptimizationHistory(page: Page) {
  await page.route(`${BASE_URL}/api/optimization/history`, async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ runs: [] }),
    });
  });
}

/**
 * Mock optimization start to fail
 */
export async function mockOptimizationStartError(page: Page) {
  await page.route(`${BASE_URL}/api/optimization/start`, async (route) => {
    if (route.request().method() === 'POST') {
      await route.fulfill({
        status: 500,
        contentType: 'application/json',
        body: JSON.stringify({ error: 'Internal server error' }),
      });
    } else {
      await route.continue();
    }
  });
}

// Export mock data for use in tests
export {
  mockOptimizationRuns,
  mockCompletedOptimizationRun,
  mockRunningOptimizationRun,
};