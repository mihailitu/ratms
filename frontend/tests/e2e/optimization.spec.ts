import { test, expect } from '@playwright/test';
import { OptimizationPage } from '../fixtures/OptimizationPage';
import { server } from '../mocks/server';
import { http, HttpResponse } from 'msw';
import { mockRunningOptimizationRun, mockCompletedOptimizationRun } from '../mocks/data/optimization';

test.describe('Optimization Page', () => {
  let optimizationPage: OptimizationPage;

  test.beforeEach(async ({ page }) => {
    optimizationPage = new OptimizationPage(page);
    server.resetHandlers();
    await optimizationPage.goto();
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

      server.use(
        http.post('http://localhost:8080/api/optimization/start', async () => {
          optimizationStarted = true;
          return HttpResponse.json({
            message: 'Optimization started',
            run: mockRunningOptimizationRun,
          });
        })
      );

      await optimizationPage.startOptimization();
      await page.waitForTimeout(500);

      expect(optimizationStarted).toBe(true);
    });

    test('should show loading state while starting', async ({ page }) => {
      server.use(
        http.post('http://localhost:8080/api/optimization/start', async () => {
          await new Promise(resolve => setTimeout(resolve, 1000));
          return HttpResponse.json({
            message: 'Optimization started',
            run: mockRunningOptimizationRun,
          });
        })
      );

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

    test('should display all optimization runs', async ({ page }) => {
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
      await page.waitForTimeout(300);

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
      await page.waitForTimeout(300);

      await expect(optimizationPage.page.getByText('Results')).toBeVisible();
      await expect(optimizationPage.page.getByText('Baseline Fitness:')).toBeVisible();
      await expect(optimizationPage.page.getByText('Best Fitness:')).toBeVisible();
      await expect(optimizationPage.getImprovementValue()).toBeVisible();
    });

    test('should display fitness improvement percentage', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(300);

      const improvement = optimizationPage.getImprovementValue();
      await expect(improvement).toContainText('%');
    });

    test('should display GA visualizer chart for completed runs', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(500);

      await expect(optimizationPage.getGAVisualizerChart()).toBeVisible();
    });

    test('should display duration in results', async ({ page }) => {
      await optimizationPage.viewRunDetails(1);
      await page.waitForTimeout(300);

      await expect(optimizationPage.page.getByText('Duration:')).toBeVisible();
      await expect(optimizationPage.page.getByText(/\d+s/)).toBeVisible();
    });
  });

  test.describe('Running Run Progress', () => {
    test('should display progress section for running runs', async ({ page }) => {
      await optimizationPage.viewRunDetails(2);
      await page.waitForTimeout(300);

      await expect(optimizationPage.page.getByText('Progress')).toBeVisible();
      await expect(optimizationPage.page.getByText(/Generation.*\//)).toBeVisible();
    });

    test('should display progress bar for running run', async ({ page }) => {
      await optimizationPage.viewRunDetails(2);
      await page.waitForTimeout(300);

      const progressBar = optimizationPage.page.locator('.bg-blue-600.h-3');
      await expect(progressBar).toBeVisible();
    });
  });

  test.describe('Stop Optimization', () => {
    test('should stop optimization when stop button is clicked', async ({ page }) => {
      let stopCalled = false;

      server.use(
        http.post('http://localhost:8080/api/optimization/stop/2', () => {
          stopCalled = true;
          return HttpResponse.json({
            message: 'Optimization stopped',
            run_id: 2,
          });
        })
      );

      await optimizationPage.stopRun(2);
      await page.waitForTimeout(500);

      expect(stopCalled).toBe(true);
    });
  });

  test.describe('Empty State', () => {
    test('should display empty state when no runs exist', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/optimization/history', () => {
          return HttpResponse.json({ runs: [] });
        })
      );

      await page.reload();
      await expect(optimizationPage.getEmptyState()).toBeVisible();
    });
  });

  test.describe('Error Handling', () => {
    test('should display error when optimization start fails', async ({ page }) => {
      server.use(
        http.post('http://localhost:8080/api/optimization/start', () => {
          return new HttpResponse(null, { status: 500 });
        })
      );

      await optimizationPage.startOptimization();
      await page.waitForTimeout(500);

      await expect(optimizationPage.page.getByText(/error|failed/i)).toBeVisible();
    });
  });
});
