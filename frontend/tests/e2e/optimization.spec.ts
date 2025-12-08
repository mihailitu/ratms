import { test, expect } from '@playwright/test';
import { OptimizationPage } from '../fixtures/OptimizationPage';
import {
  setupOptimizationMocks,
  mockEmptyOptimizationHistory,
  mockOptimizationStartError,
  mockRunningOptimizationRun,
} from '../helpers/apiMocks';

test.describe('Optimization Page', () => {
  let optimizationPage: OptimizationPage;

  test.beforeEach(async ({ page }) => {
    optimizationPage = new OptimizationPage(page);
    // Setup Playwright route-based mocking (intercepts at browser level)
    await setupOptimizationMocks(page);
    await optimizationPage.goto();
    // Wait for page to load (loading state to disappear)
    await page.waitForSelector('text=Traffic Light Optimization', { timeout: 10000 });
  });

  test.describe('Page Load', () => {
    test('should load the optimization page successfully', async () => {
      await expect(optimizationPage.page.getByText('Traffic Light Optimization')).toBeVisible();
    });

    test('should display page description', async () => {
      await expect(
        optimizationPage.page.getByText('Genetic Algorithm-based optimization for traffic signal timing')
      ).toBeVisible();
    });
  });

  test.describe('Optimization Form', () => {
    test('should display all form fields', async () => {
      await expect(optimizationPage.getPopulationSizeInput()).toBeVisible();
      await expect(optimizationPage.getGenerationsInput()).toBeVisible();
      await expect(optimizationPage.getMutationRateInput()).toBeVisible();
      await expect(optimizationPage.getCrossoverRateInput()).toBeVisible();
      await expect(optimizationPage.getSimulationStepsInput()).toBeVisible();
      await expect(optimizationPage.getNetworkIdInput()).toBeVisible();
    });

    test('should have default values in form fields', async () => {
      await expect(optimizationPage.getPopulationSizeInput()).toHaveValue('30');
      await expect(optimizationPage.getGenerationsInput()).toHaveValue('50');
      await expect(optimizationPage.getMutationRateInput()).toHaveValue('0.15');
      await expect(optimizationPage.getCrossoverRateInput()).toHaveValue('0.8');
    });

    test('should allow editing form fields', async () => {
      await optimizationPage.fillForm({ populationSize: 50, generations: 100 });

      await expect(optimizationPage.getPopulationSizeInput()).toHaveValue('50');
      await expect(optimizationPage.getGenerationsInput()).toHaveValue('100');
    });

    test('should display Start Optimization button', async () => {
      await expect(optimizationPage.getStartButton()).toBeVisible();
      await expect(optimizationPage.getStartButton()).toHaveText('Start Optimization');
    });
  });

  test.describe('Start Optimization', () => {
    test('should submit form when Start Optimization is clicked', async ({ page }) => {
      let optimizationStarted = false;

      // Override the start endpoint to track the call
      await page.route('http://localhost:8080/api/optimization/start', async (route) => {
        if (route.request().method() === 'POST') {
          optimizationStarted = true;
          const body = JSON.parse(route.request().postData() || '{}');
          await route.fulfill({
            status: 200,
            contentType: 'application/json',
            body: JSON.stringify({
              message: 'Optimization started',
              run: {
                id: 99,
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

      await optimizationPage.startOptimization();
      await page.waitForTimeout(500);

      expect(optimizationStarted).toBe(true);
    });

    test('should show loading state while starting', async ({ page }) => {
      // Set up a delayed response
      await page.route('http://localhost:8080/api/optimization/start', async (route) => {
        if (route.request().method() === 'POST') {
          await new Promise(resolve => setTimeout(resolve, 1000));
          await route.fulfill({
            status: 200,
            contentType: 'application/json',
            body: JSON.stringify({
              message: 'Optimization started',
              run: mockRunningOptimizationRun,
            }),
          });
        } else {
          await route.continue();
        }
      });

      const startPromise = optimizationPage.startOptimization();
      await page.waitForTimeout(200);

      await expect(optimizationPage.getStartButton()).toContainText('Starting');
      await startPromise;
    });
  });

  test.describe('Optimization History', () => {
    test('should display optimization history section', async () => {
      await expect(optimizationPage.getHistorySection()).toBeVisible();
      await expect(optimizationPage.page.getByText('Optimization History')).toBeVisible();
    });

    test('should display all optimization runs', async () => {
      await expect(optimizationPage.getRun(1)).toBeVisible();
      await expect(optimizationPage.getRun(2)).toBeVisible();
    });

    test('should display run status badges', async () => {
      await expect(optimizationPage.getRunStatus(1)).toContainText('completed');
      await expect(optimizationPage.getRunStatus(2)).toContainText('running');
    });

    test('should display progress bar for running optimizations', async () => {
      const progressBar = optimizationPage.getProgressBar(2);
      await expect(progressBar).toBeVisible();
    });

    test('should display stop button for running optimizations', async () => {
      await expect(optimizationPage.getStopButton(2)).toBeVisible();
    });

    test('should not display stop button for completed optimizations', async () => {
      await expect(optimizationPage.getStopButton(1)).not.toBeVisible();
    });
  });

  test.describe('Run Selection', () => {
    test('should select run when clicked', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(300);

      const isSelected = await optimizationPage.isRunSelected(1);
      expect(isSelected).toBe(true);
    });

    test('should display run details when selected', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      // Wait for API call and state update
      await page.waitForTimeout(500);

      await expect(optimizationPage.getResultsSection()).toBeVisible();
      await expect(optimizationPage.page.getByText('Run #1 Details')).toBeVisible();
    });

    test('should display parameters section for selected run', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(300);

      await expect(optimizationPage.getParametersSection()).toBeVisible();
      await expect(optimizationPage.page.getByText('Population:')).toBeVisible();
      await expect(optimizationPage.page.getByText('Generations:')).toBeVisible();
    });
  });

  test.describe('Completed Run Results', () => {
    test('should display results for completed runs', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(500);

      // Check that results section is visible with specific labels
      await expect(page.getByText('Results').first()).toBeVisible();
      await expect(page.getByText('Baseline Fitness:', { exact: true })).toBeVisible();
      await expect(page.getByText('Best Fitness:', { exact: true })).toBeVisible();
    });

    test('should display fitness improvement percentage', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(500);

      // Check for improvement percentage in results section
      await expect(page.getByText(/Improvement:/).first()).toBeVisible();
      await expect(page.getByText(/37\.8%/).first()).toBeVisible();
    });

    test('should display GA visualizer chart for completed runs', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(500);

      // GAVisualizer renders an SVG chart
      await expect(page.locator('svg').first()).toBeVisible();
      await expect(page.getByText('Fitness Evolution')).toBeVisible();
    });

    test('should display duration in results', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(500);

      await expect(page.getByText('Duration:', { exact: true })).toBeVisible();
      await expect(page.getByText('125s')).toBeVisible();
    });
  });

  test.describe('Running Run Progress', () => {
    test('should display progress section for running runs', async ({ page }) => {
      await optimizationPage.viewRunDetails(2);
      await page.waitForTimeout(500);

      // Running run should show Progress heading in details panel
      await expect(page.getByText('Progress').first()).toBeVisible();
      // And generation progress text
      await expect(page.getByText(/Generation:.*45.*\/.*100/)).toBeVisible();
    });

    test('should display progress bar for running run', async ({ page }) => {
      await optimizationPage.viewRunDetails(2);
      await page.waitForTimeout(500);

      // Progress bar with h-3 height in details section
      const progressBar = page.locator('.bg-blue-600.h-3');
      await expect(progressBar).toBeVisible();
    });
  });

  test.describe('Stop Optimization', () => {
    test('should stop optimization when stop button is clicked', async ({ page }) => {
      let stopCalled = false;

      await page.route('http://localhost:8080/api/optimization/stop/2', async (route) => {
        if (route.request().method() === 'POST') {
          stopCalled = true;
          await route.fulfill({
            status: 200,
            contentType: 'application/json',
            body: JSON.stringify({
              message: 'Optimization stopped',
              run_id: 2,
            }),
          });
        } else {
          await route.continue();
        }
      });

      await optimizationPage.stopRun(2);
      await page.waitForTimeout(500);

      expect(stopCalled).toBe(true);
    });
  });

  test.describe('Empty State', () => {
    test('should display empty state when no runs exist', async ({ page }) => {
      // Override with empty history before reload
      await mockEmptyOptimizationHistory(page);
      await page.reload();
      await page.waitForSelector('text=Traffic Light Optimization', { timeout: 10000 });
      await expect(optimizationPage.getEmptyState()).toBeVisible();
    });
  });

  test.describe('Error Handling', () => {
    test('should display error when optimization start fails', async ({ page }) => {
      await mockOptimizationStartError(page);

      await optimizationPage.startOptimization();
      await page.waitForTimeout(500);

      // Error should be shown - could be "Failed to start optimization" or similar
      await expect(page.locator('.bg-red-50, .text-red-700').first()).toBeVisible();
    });
  });
});