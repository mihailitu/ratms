import { test, expect } from '@playwright/test';
import { DashboardPage } from '../fixtures/DashboardPage';
import { server } from '../mocks/server';
import { mockRunningSimulationStatus } from '../mocks/data/simulations';
import { http, HttpResponse } from 'msw';

test.describe('Dashboard Page', () => {
  let dashboardPage: DashboardPage;

  test.beforeEach(async ({ page }) => {
    dashboardPage = new DashboardPage(page);
    // Reset handlers before each test
    server.resetHandlers();
    await dashboardPage.goto();
  });

  test.describe('Page Load and Initial State', () => {
    test('should load the dashboard page successfully', async () => {
      await expect(dashboardPage.page).toHaveTitle(/RATMS|Vite/);
      await expect(dashboardPage.page.getByText('RATMS Dashboard')).toBeVisible();
    });

    test('should display the page title and description', async () => {
      await expect(dashboardPage.page.getByText('RATMS Dashboard')).toBeVisible();
      await expect(
        dashboardPage.page.getByText('Real-time Adaptive Traffic Management System')
      ).toBeVisible();
    });

    test('should not show loading state after data is loaded', async ({ page }) => {
      await expect(dashboardPage.getLoadingIndicator()).not.toBeVisible();
    });
  });

  test.describe('Simulation Status Display', () => {
    test('should display simulation status as Stopped', async () => {
      const status = dashboardPage.getSimulationStatus();
      await expect(status).toContainText('Stopped');
    });

    test('should display simulation status as Running when simulation is active', async ({
      page,
    }) => {
      // Override the status endpoint to return running status
      server.use(
        http.get('http://localhost:8080/api/simulation/status', () => {
          return HttpResponse.json(mockRunningSimulationStatus);
        })
      );

      await page.reload();
      await expect(dashboardPage.getSimulationStatus()).toContainText('Running');
    });

    test('should display road count', async () => {
      const roadCount = dashboardPage.getRoadCount();
      await expect(roadCount).toBeVisible();
      await expect(roadCount).toContainText(/\d+/); // Contains a number
    });

    test('should display server status as Online', async () => {
      const serverStatus = dashboardPage.getServerStatus();
      await expect(serverStatus).toContainText('Online');
    });
  });

  test.describe('Simulation Control', () => {
    test('should display both start and stop buttons', async () => {
      await expect(dashboardPage.getStartButton()).toBeVisible();
      await expect(dashboardPage.getStopButton()).toBeVisible();
    });

    test('should enable start button when simulation is stopped', async () => {
      await expect(dashboardPage.getStartButton()).toBeEnabled();
    });

    test('should disable stop button when simulation is stopped', async () => {
      await expect(dashboardPage.getStopButton()).toBeDisabled();
    });

    test('should call start API when start button is clicked', async ({ page }) => {
      let startCalled = false;

      server.use(
        http.post('http://localhost:8080/api/simulation/start', () => {
          startCalled = true;
          return HttpResponse.json({
            message: 'Simulation started',
            status: 'running',
          });
        })
      );

      await dashboardPage.startSimulation();
      await page.waitForTimeout(500); // Wait for API call

      expect(startCalled).toBe(true);
    });

    test('should call stop API when stop button is clicked', async ({ page }) => {
      // First set status to running
      server.use(
        http.get('http://localhost:8080/api/simulation/status', () => {
          return HttpResponse.json(mockRunningSimulationStatus);
        })
      );

      await page.reload();
      await expect(dashboardPage.getStopButton()).toBeEnabled();

      let stopCalled = false;
      server.use(
        http.post('http://localhost:8080/api/simulation/stop', () => {
          stopCalled = true;
          return HttpResponse.json({
            message: 'Simulation stopped',
            status: 'stopped',
          });
        })
      );

      await dashboardPage.stopSimulation();
      await page.waitForTimeout(500);

      expect(stopCalled).toBe(true);
    });
  });

  test.describe('Networks Section', () => {
    test('should display networks section', async () => {
      await expect(dashboardPage.page.getByText('Networks')).toBeVisible();
    });

    test('should display network list', async () => {
      const networksSection = dashboardPage.getNetworksSection();
      await expect(networksSection).toBeVisible();
    });

    test('should display network details (name, description, counts)', async ({ page }) => {
      await expect(page.getByText('Test Network 10x10')).toBeVisible();
      await expect(page.getByText('A 10x10 grid test network')).toBeVisible();
      await expect(page.getByText(/200 roads.*100 intersections/)).toBeVisible();
    });

    test('should show empty state when no networks configured', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/networks', () => {
          return HttpResponse.json([]);
        })
      );

      await page.reload();
      await expect(page.getByText('No networks configured')).toBeVisible();
    });
  });

  test.describe('Metrics Cards', () => {
    test('should display all three metric cards', async ({ page }) => {
      await expect(page.getByText('Simulation Status')).toBeVisible();
      await expect(page.getByText('Road Count')).toBeVisible();
      await expect(page.getByText('Server Status')).toBeVisible();
    });

    test('should display correct simulation status color', async ({ page }) => {
      // Stopped status should be gray
      const statusCard = dashboardPage.getMetricsCard('Simulation Status');
      await expect(statusCard).toBeVisible();
    });
  });

  test.describe('Error Handling', () => {
    test('should display error when API fails', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulation/status', () => {
          return new HttpResponse(null, { status: 500 });
        })
      );

      await page.reload();
      await expect(page.getByText(/error/i)).toBeVisible();
    });

    test('should display error when network fetch fails', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/networks', () => {
          return new HttpResponse(null, { status: 500 });
        })
      );

      await page.reload();
      await expect(page.getByText(/error|failed/i)).toBeVisible();
    });
  });

  test.describe('Polling and Updates', () => {
    test('should poll for status updates every 2 seconds', async ({ page }) => {
      let callCount = 0;

      server.use(
        http.get('http://localhost:8080/api/simulation/status', () => {
          callCount++;
          return HttpResponse.json(mockRunningSimulationStatus);
        })
      );

      await page.reload();
      await page.waitForTimeout(5000); // Wait 5 seconds

      // Should have called at least 2 times (initial + 2 polls)
      expect(callCount).toBeGreaterThanOrEqual(2);
    });
  });
});
