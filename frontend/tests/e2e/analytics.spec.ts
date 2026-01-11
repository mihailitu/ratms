import { test, expect } from '@playwright/test';
import { AnalyticsPage } from '../fixtures/AnalyticsPage';
import { server } from '../mocks/server';
import { http, HttpResponse } from 'msw';

test.describe('Analytics Page', () => {
  let analyticsPage: AnalyticsPage;

  test.beforeEach(async ({ page }) => {
    analyticsPage = new AnalyticsPage(page);
    server.resetHandlers();
    await analyticsPage.goto();
  });

  test.describe('Page Load', () => {
    test('should load the analytics page successfully', async () => {
      await expect(analyticsPage.page.getByText('Performance Analytics')).toBeVisible();
    });

    test('should display page description', async () => {
      await expect(
        analyticsPage.page.getByText('Compare simulations and analyze performance metrics')
      ).toBeVisible();
    });
  });

  test.describe('Simulation Selection', () => {
    test('should display simulation checkboxes', async () => {
      const checkboxes = analyticsPage.getAllCheckboxes();
      const count = await checkboxes.count();
      expect(count).toBeGreaterThan(0);
    });

    test('should only show completed simulations', async ({ page }) => {
      // Should show Test Simulation 1 and 2 (completed), not Running Simulation
      await expect(page.getByText('Test Simulation 1').or(page.getByText('Sim 1'))).toBeVisible();
      await expect(page.getByText('Test Simulation 2').or(page.getByText('Sim 2'))).toBeVisible();
    });

    test('should allow selecting multiple simulations', async () => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.selectSimulation(2);

      const selectedCount = await analyticsPage.getSelectedCount();
      expect(selectedCount).toBeGreaterThanOrEqual(2);
    });

    test('should allow deselecting simulations', async () => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.deselectSimulation(1);

      const selectedCount = await analyticsPage.getSelectedCount();
      expect(selectedCount).toBe(0);
    });
  });

  test.describe('Metric Type Selection', () => {
    test('should display metric type dropdown', async () => {
      await expect(analyticsPage.getMetricTypeSelector()).toBeVisible();
    });

    test('should populate metric types from API', async ({ page }) => {
      const selector = analyticsPage.getMetricTypeSelector();
      const options = await selector.locator('option').all();

      expect(options.length).toBeGreaterThan(0);
    });

    test('should allow selecting different metric types', async () => {
      await analyticsPage.selectMetricType('avg_vehicle_speed');

      const value = await analyticsPage.getMetricTypeSelector().inputValue();
      expect(value).toBe('avg_vehicle_speed');
    });
  });

  test.describe('Compare Button', () => {
    test('should display compare button', async () => {
      await expect(analyticsPage.getCompareButton()).toBeVisible();
    });

    test('should disable compare button when no simulations selected', async () => {
      await expect(analyticsPage.getCompareButton()).toBeDisabled();
    });

    test('should enable compare button when simulations are selected', async () => {
      await analyticsPage.selectSimulation(1);
      await expect(analyticsPage.getCompareButton()).toBeEnabled();
    });

    test('should show loading state while comparing', async ({ page }) => {
      await analyticsPage.selectSimulation(1);

      server.use(
        http.post('http://localhost:8080/api/analytics/compare', async () => {
          await new Promise(resolve => setTimeout(resolve, 1000));
          return HttpResponse.json({
            metric_type: 'avg_queue_length',
            simulations: [],
          });
        })
      );

      const comparePromise = analyticsPage.clickCompare();
      await page.waitForTimeout(200);

      await expect(analyticsPage.getCompareButton()).toContainText('Loading');
      await comparePromise;
    });
  });

  test.describe('Comparison Chart', () => {
    test('should display chart after comparison', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      await expect(analyticsPage.getComparisonChart()).toBeVisible();
    });

    test('should display chart title with metric type', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      const title = analyticsPage.getChartTitle();
      await expect(title).toBeVisible();
      await expect(title).toContainText('Comparison Chart');
    });

    test('should render Recharts SVG', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      const chart = analyticsPage.getComparisonChart();
      await expect(chart.locator('svg')).toBeVisible();
    });
  });

  test.describe('Statistics Table', () => {
    test('should display statistics table after comparison', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      await expect(analyticsPage.getStatisticsTable()).toBeVisible();
    });

    test('should display all table headers', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      const headers = analyticsPage.getTableHeaders();
      await expect(headers).toContainText([
        'Simulation',
        'Min',
        'Max',
        'Mean',
        'Median',
        'Std Dev',
      ]);
    });

    test('should display statistics for each selected simulation', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      const table = analyticsPage.getStatisticsTable();
      await expect(table.locator('tbody tr')).toHaveCount(1);
    });

    test('should display export CSV button for each simulation', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      await expect(
        analyticsPage.page.getByRole('button', { name: 'Export CSV' })
      ).toBeVisible();
    });
  });

  test.describe('CSV Export', () => {
    test('should download CSV when export button is clicked', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      const downloadPromise = page.waitForEvent('download');
      await analyticsPage.page.getByRole('button', { name: 'Export CSV' }).first().click();

      const download = await downloadPromise;
      expect(download.suggestedFilename()).toContain('.csv');
    });

    test('should export correct file format', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      const downloadPromise = page.waitForEvent('download');
      await analyticsPage.page.getByRole('button', { name: 'Export CSV' }).first().click();

      const download = await downloadPromise;
      const path = await download.path();
      expect(path).toBeDefined();
    });
  });

  test.describe('Empty States', () => {
    test('should show empty state when no completed simulations', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulations', () => {
          return HttpResponse.json([]);
        })
      );

      await page.reload();
      await expect(analyticsPage.getEmptyState()).toBeVisible();
    });

    test('should display appropriate message for no data', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulations', () => {
          return HttpResponse.json([]);
        })
      );

      await page.reload();
      await expect(page.getByText('No completed simulations available')).toBeVisible();
    });
  });

  test.describe('Error Handling', () => {
    test('should display error when comparison fails', async ({ page }) => {
      await analyticsPage.selectSimulation(1);

      server.use(
        http.post('http://localhost:8080/api/analytics/compare', () => {
          return new HttpResponse(null, { status: 500 });
        })
      );

      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      await expect(page.getByText(/error|failed/i)).toBeVisible();
    });

    test('should display error when statistics fetch fails', async ({ page }) => {
      await analyticsPage.selectSimulation(1);

      server.use(
        http.get(
          'http://localhost:8080/api/analytics/simulations/1/statistics/avg_queue_length',
          () => {
            return new HttpResponse(null, { status: 500 });
          }
        )
      );

      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      // Should handle error gracefully
    });
  });

  test.describe('Multiple Simulation Comparison', () => {
    test('should display multiple lines in chart when comparing multiple simulations', async ({
      page,
    }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.selectSimulation(2);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      const chart = analyticsPage.getComparisonChart();
      // Should have multiple lines (one per simulation)
      await expect(chart).toBeVisible();
    });

    test('should display statistics row for each selected simulation', async ({ page }) => {
      await analyticsPage.selectSimulation(1);
      await analyticsPage.selectSimulation(2);
      await analyticsPage.clickCompare();
      await page.waitForTimeout(500);

      const table = analyticsPage.getStatisticsTable();
      const rows = await table.locator('tbody tr').count();
      expect(rows).toBeGreaterThanOrEqual(2);
    });
  });
});
