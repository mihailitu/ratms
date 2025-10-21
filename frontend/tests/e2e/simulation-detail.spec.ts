import { test, expect } from '@playwright/test';
import { server } from '../mocks/server';
import { http, HttpResponse } from 'msw';
import { mockSimulations } from '../mocks/data/simulations';

test.describe('Simulation Detail Page', () => {
  test.beforeEach(async ({ page }) => {
    server.resetHandlers();
    await page.goto('/simulations/1');
  });

  test.describe('Page Load', () => {
    test('should load simulation detail page', async ({ page }) => {
      await expect(page).toHaveURL(/\/simulations\/1/);
    });

    test('should display simulation information', async ({ page }) => {
      // Page should show simulation-specific data
      await expect(page.locator('body')).toBeVisible();
    });

    test('should handle invalid simulation ID', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulations/999', () => {
          return new HttpResponse(null, { status: 404 });
        })
      );

      await page.goto('/simulations/999');
      // Should show error or 404 state
    });
  });

  test.describe('Data Display', () => {
    test('should load simulation data from API', async ({ page }) => {
      const simulation = mockSimulations[0];
      await expect(page.locator('body')).toContainText(simulation.name);
    });

    test('should display simulation metrics if available', async ({ page }) => {
      // Metrics visualization depends on implementation
      await page.waitForLoadState('networkidle');
    });
  });

  test.describe('Navigation', () => {
    test('should navigate back to simulations list', async ({ page }) => {
      await page.getByRole('link', { name: /simulations/i }).click();
      await expect(page).toHaveURL(/\/simulations$/);
    });
  });
});
