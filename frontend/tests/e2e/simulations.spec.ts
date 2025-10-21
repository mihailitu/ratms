import { test, expect } from '@playwright/test';
import { SimulationsPage } from '../fixtures/SimulationsPage';
import { server } from '../mocks/server';
import { http, HttpResponse } from 'msw';

test.describe('Simulations Page', () => {
  let simulationsPage: SimulationsPage;

  test.beforeEach(async ({ page }) => {
    simulationsPage = new SimulationsPage(page);
    server.resetHandlers();
    await simulationsPage.goto();
  });

  test.describe('Page Load', () => {
    test('should load the simulations page successfully', async () => {
      await expect(simulationsPage.page.getByText('Simulation History')).toBeVisible();
    });

    test('should display page description', async () => {
      await expect(
        simulationsPage.page.getByText('View and analyze past simulation runs')
      ).toBeVisible();
    });

    test('should not show loading state after data loads', async () => {
      await expect(simulationsPage.getLoadingIndicator()).not.toBeVisible();
    });
  });

  test.describe('Simulations Table', () => {
    test('should display the simulations table', async () => {
      await expect(simulationsPage.getTable()).toBeVisible();
    });

    test('should display table headers', async () => {
      const headers = simulationsPage.getHeaders();
      await expect(headers).toContainText(['ID', 'Name', 'Status', 'Started', 'Duration', 'Actions']);
    });

    test('should display all simulations from API', async () => {
      const count = await simulationsPage.getSimulationCount();
      expect(count).toBeGreaterThan(0);
    });

    test('should display simulation ID', async () => {
      await expect(simulationsPage.getSimulationRow(1)).toContainText('#1');
    });

    test('should display simulation name', async () => {
      const name = simulationsPage.getSimulationName(1);
      await expect(name).toContainText('Test Simulation 1');
    });

    test('should display simulation status badge', async () => {
      const badge = simulationsPage.getStatusBadge(1);
      await expect(badge).toBeVisible();
      await expect(badge).toContainText(/completed|running|failed/i);
    });

    test('should display formatted timestamp', async () => {
      const row = simulationsPage.getSimulationRow(1);
      await expect(row).toContainText(/\d{1,2}\/\d{1,2}\/\d{4}/); // Date format
    });

    test('should display formatted duration', async () => {
      const row = simulationsPage.getSimulationRow(1);
      await expect(row).toContainText(/\d+m \d+s|N\/A/);
    });

    test('should display View Details button for each simulation', async () => {
      const button = simulationsPage.getViewDetailsButton(1);
      await expect(button).toBeVisible();
      await expect(button).toHaveText('View Details');
    });
  });

  test.describe('Status Badges', () => {
    test('should display completed status with correct color', async () => {
      const badge = simulationsPage.getStatusBadge(1);
      await expect(badge).toContainText('completed');
      // Check for blue color class (bg-blue-100)
      const classes = await badge.getAttribute('class');
      expect(classes).toContain('bg-blue-100');
    });

    test('should display running status with correct color', async () => {
      const badge = simulationsPage.getStatusBadge(3);
      await expect(badge).toContainText('running');
      // Check for green color class (bg-green-100)
      const classes = await badge.getAttribute('class');
      expect(classes).toContain('bg-green-100');
    });
  });

  test.describe('Navigation', () => {
    test('should navigate to simulation detail page when View Details is clicked', async ({
      page,
    }) => {
      await simulationsPage.viewDetails(1);

      // Check URL changed
      expect(page.url()).toContain('/simulations/1');
    });

    test('should maintain simulation data after navigation', async ({ page }) => {
      await simulationsPage.viewDetails(1);
      await page.goBack();

      // Should still show the simulations list
      await expect(simulationsPage.getTable()).toBeVisible();
    });
  });

  test.describe('Empty State', () => {
    test('should display empty state when no simulations exist', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulations', () => {
          return HttpResponse.json([]);
        })
      );

      await page.reload();
      const isEmpty = await simulationsPage.isEmpty();
      expect(isEmpty).toBe(true);
    });

    test('should show "No simulations found" message in empty state', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulations', () => {
          return HttpResponse.json([]);
        })
      );

      await page.reload();
      await expect(page.getByText('No simulations found')).toBeVisible();
    });
  });

  test.describe('Error Handling', () => {
    test('should display error message when API fails', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulations', () => {
          return new HttpResponse(null, { status: 500 });
        })
      );

      await page.reload();
      await expect(page.getByText(/error/i)).toBeVisible();
    });

    test('should display error message with details', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulations', () => {
          return new HttpResponse(null, { status: 500 });
        })
      );

      await page.reload();
      await expect(page.getByText(/failed to fetch simulations/i)).toBeVisible();
    });
  });

  test.describe('Table Hover Effects', () => {
    test('should apply hover effect to table rows', async () => {
      const row = simulationsPage.getSimulationRow(1);
      await row.hover();

      const classes = await row.getAttribute('class');
      expect(classes).toContain('hover:bg-gray-50');
    });
  });

  test.describe('Row Interaction', () => {
    test('should display all expected columns for each row', async () => {
      const row = simulationsPage.getSimulationRow(1);
      const cells = await row.locator('td').all();

      expect(cells.length).toBeGreaterThanOrEqual(5); // ID, Name, Status, Started, Duration, Actions
    });

    test('should have clickable View Details button', async () => {
      const button = simulationsPage.getViewDetailsButton(1);
      await expect(button).toBeEnabled();
    });
  });
});
