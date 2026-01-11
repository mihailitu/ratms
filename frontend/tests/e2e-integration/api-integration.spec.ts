import { test, expect } from '@playwright/test';

/**
 * Integration tests with REAL backend API server
 *
 * IMPORTANT: These tests require the C++ api_server to be running:
 * 1. cd simulator/build
 * 2. ./api_server
 *
 * Run these tests separately with: npm run test:e2e:integration
 */

test.describe('API Integration Tests (Real Backend)', () => {
  const API_BASE = 'http://localhost:8080';

  test.beforeAll(async () => {
    // Verify backend is running before tests
    try {
      const response = await fetch(`${API_BASE}/api/health`);
      if (!response.ok) {
        throw new Error('Backend API is not healthy');
      }
    } catch (error) {
      throw new Error(
        'Backend API server is not running. Please start ./api_server before running integration tests.'
      );
    }
  });

  test.describe('Health Check', () => {
    test('should return healthy status from real API', async ({ request }) => {
      const response = await request.get(`${API_BASE}/api/health`);
      expect(response.ok()).toBeTruthy();

      const data = await response.json();
      expect(data.service).toBe('RATMS API Server');
      expect(data.status).toBe('healthy');
    });
  });

  test.describe('Simulation Control Integration', () => {
    test('should get simulation status from real API', async ({ request }) => {
      const response = await request.get(`${API_BASE}/api/simulation/status`);
      expect(response.ok()).toBeTruthy();

      const data = await response.json();
      expect(data).toHaveProperty('status');
      expect(data).toHaveProperty('server_running');
      expect(data).toHaveProperty('road_count');
    });

    test('should start and stop simulation via real API', async ({ request }) => {
      // Start simulation
      const startResponse = await request.post(`${API_BASE}/api/simulation/start`);
      expect(startResponse.ok()).toBeTruthy();

      const startData = await startResponse.json();
      expect(startData.status).toBe('running');

      // Wait a moment for simulation to start
      await new Promise(resolve => setTimeout(resolve, 1000));

      // Check status is running
      const statusResponse = await request.get(`${API_BASE}/api/simulation/status`);
      const statusData = await statusResponse.json();
      expect(statusData.status).toBe('running');

      // Stop simulation
      const stopResponse = await request.post(`${API_BASE}/api/simulation/stop`);
      expect(stopResponse.ok()).toBeTruthy();

      const stopData = await stopResponse.json();
      expect(stopData.status).toBe('stopped');
    });
  });

  test.describe('Networks Integration', () => {
    test('should fetch networks from real API', async ({ request }) => {
      const response = await request.get(`${API_BASE}/api/networks`);
      expect(response.ok()).toBeTruthy();

      const data = await response.json();
      expect(Array.isArray(data)).toBeTruthy();
    });

    test('should fetch roads from real API', async ({ request }) => {
      const response = await request.get(`${API_BASE}/api/simulation/roads`);
      expect(response.ok()).toBeTruthy();

      const data = await response.json();
      expect(data).toHaveProperty('roads');
      expect(Array.isArray(data.roads)).toBeTruthy();
    });
  });

  test.describe('SSE Streaming Integration', () => {
    test('should receive SSE updates from real simulation', async ({ page }) => {
      // Start simulation first
      await fetch(`${API_BASE}/api/simulation/start`, { method: 'POST' });

      // Navigate to map view
      await page.goto('http://localhost:5173/map');
      await page.waitForLoadState('networkidle');

      // Wait for map to load
      await page.waitForSelector('.leaflet-container', { timeout: 10000 });

      // Start streaming
      await page.getByRole('button', { name: /Start Streaming/i }).click();

      // Wait for stream info to appear
      await page.waitForSelector('text=/Step:|Time:|Vehicles:/', { timeout: 5000 });

      // Verify stream is active
      const streamStatus = page.locator('text=Stream:').locator('..');
      await expect(streamStatus).toContainText('Live');

      // Clean up: stop simulation
      await fetch(`${API_BASE}/api/simulation/stop`, { method: 'POST' });
    });
  });

  test.describe('Optimization Integration', () => {
    test('should start optimization via real API', async ({ request }) => {
      const params = {
        populationSize: 20,
        generations: 10,
        mutationRate: 0.15,
        mutationStdDev: 5.0,
        crossoverRate: 0.8,
        tournamentSize: 3,
        elitismRate: 0.1,
        minGreenTime: 10.0,
        maxGreenTime: 60.0,
        minRedTime: 10.0,
        maxRedTime: 60.0,
        simulationSteps: 100,
        dt: 0.1,
        networkId: 1,
      };

      const response = await request.post(`${API_BASE}/api/optimization/start`, {
        data: params,
      });

      expect(response.ok()).toBeTruthy();

      const data = await response.json();
      expect(data).toHaveProperty('run');
      expect(data.run).toHaveProperty('id');
      expect(data.run.status).toBe('running');

      // Get optimization history
      const historyResponse = await request.get(`${API_BASE}/api/optimization/history`);
      const historyData = await historyResponse.json();

      expect(historyData).toHaveProperty('runs');
      expect(historyData.runs.length).toBeGreaterThan(0);
    });
  });

  test.describe('Analytics Integration', () => {
    test('should fetch simulations from real database', async ({ request }) => {
      const response = await request.get(`${API_BASE}/api/simulations`);
      expect(response.ok()).toBeTruthy();

      const data = await response.json();
      expect(Array.isArray(data)).toBeTruthy();
    });

    test('should fetch metric types from real API', async ({ request }) => {
      const response = await request.get(`${API_BASE}/api/analytics/metric-types`);
      expect(response.ok()).toBeTruthy();

      const data = await response.json();
      expect(data).toHaveProperty('metric_types');
      expect(Array.isArray(data.metric_types)).toBeTruthy();
    });
  });

  test.describe('End-to-End User Flow', () => {
    test('should complete full workflow: start sim -> view map -> stop sim', async ({
      page,
    }) => {
      // Navigate to dashboard
      await page.goto('http://localhost:5173/');
      await page.waitForLoadState('networkidle');

      // Start simulation
      await page.getByRole('button', { name: 'Start Simulation' }).click();
      await page.waitForTimeout(1000);

      // Verify status changed to running
      await expect(page.locator('text=/Running/')).toBeVisible();

      // Navigate to map view
      await page.getByRole('link', { name: /map/i }).click();
      await page.waitForLoadState('networkidle');

      // Verify map loaded
      await page.waitForSelector('.leaflet-container', { timeout: 10000 });

      // Stop simulation
      await page.getByRole('button', { name: 'Stop Simulation' }).click();
      await page.waitForTimeout(1000);

      // Verify status changed to stopped
      await expect(page.locator('text=/Stopped/')).toBeVisible();
    });
  });
});
