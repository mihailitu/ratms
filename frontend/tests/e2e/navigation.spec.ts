import { test, expect } from '@playwright/test';
import { BasePage } from '../fixtures/BasePage';

test.describe('Navigation', () => {
  let basePage: BasePage;

  test.beforeEach(async ({ page }) => {
    basePage = new BasePage(page);
    await basePage.goto('/');
  });

  test.describe('Navigation Links', () => {
    test('should display all navigation links', async ({ page }) => {
      await expect(page.getByRole('link', { name: /dashboard|home/i })).toBeVisible();
      await expect(page.getByRole('link', { name: /simulations/i })).toBeVisible();
      await expect(page.getByRole('link', { name: /map/i })).toBeVisible();
      await expect(page.getByRole('link', { name: /optimization/i })).toBeVisible();
      await expect(page.getByRole('link', { name: /analytics/i })).toBeVisible();
    });

    test('should navigate to Simulations page', async ({ page }) => {
      await page.getByRole('link', { name: /simulations/i }).click();
      await expect(page).toHaveURL(/\/simulations/);
      await expect(page.getByText('Simulation History')).toBeVisible();
    });

    test('should navigate to Map page', async ({ page }) => {
      await page.getByRole('link', { name: /map/i }).click();
      await expect(page).toHaveURL(/\/map/);
      await expect(page.getByText('Network Map View')).toBeVisible();
    });

    test('should navigate to Optimization page', async ({ page }) => {
      await page.getByRole('link', { name: /optimization/i }).click();
      await expect(page).toHaveURL(/\/optimization/);
      await expect(page.getByText('Traffic Light Optimization')).toBeVisible();
    });

    test('should navigate to Analytics page', async ({ page }) => {
      await page.getByRole('link', { name: /analytics/i }).click();
      await expect(page).toHaveURL(/\/analytics/);
      await expect(page.getByText('Performance Analytics')).toBeVisible();
    });
  });

  test.describe('Browser Navigation', () => {
    test('should support back navigation', async ({ page }) => {
      await page.getByRole('link', { name: /simulations/i }).click();
      await page.goBack();
      await expect(page).toHaveURL('/');
    });

    test('should support forward navigation', async ({ page }) => {
      await page.getByRole('link', { name: /simulations/i }).click();
      await page.goBack();
      await page.goForward();
      await expect(page).toHaveURL(/\/simulations/);
    });

    test('should maintain navigation state across route changes', async ({ page }) => {
      await page.getByRole('link', { name: /map/i }).click();
      await page.getByRole('link', { name: /optimization/i }).click();
      await page.goBack();
      await expect(page).toHaveURL(/\/map/);
    });
  });

  test.describe('Layout Persistence', () => {
    test('should maintain layout across route changes', async ({ page }) => {
      const nav = page.locator('nav');
      await expect(nav).toBeVisible();

      await page.getByRole('link', { name: /simulations/i }).click();
      await expect(nav).toBeVisible();

      await page.getByRole('link', { name: /map/i }).click();
      await expect(nav).toBeVisible();
    });
  });

  test.describe('Direct URL Access', () => {
    test('should load Dashboard at root path', async ({ page }) => {
      await page.goto('/');
      await expect(page.getByText('RATMS Dashboard')).toBeVisible();
    });

    test('should load Simulations at /simulations', async ({ page }) => {
      await page.goto('/simulations');
      await expect(page.getByText('Simulation History')).toBeVisible();
    });

    test('should load Map at /map', async ({ page }) => {
      await page.goto('/map');
      await expect(page.getByText('Network Map View')).toBeVisible();
    });

    test('should load Optimization at /optimization', async ({ page }) => {
      await page.goto('/optimization');
      await expect(page.getByText('Traffic Light Optimization')).toBeVisible();
    });

    test('should load Analytics at /analytics', async ({ page }) => {
      await page.goto('/analytics');
      await expect(page.getByText('Performance Analytics')).toBeVisible();
    });
  });
});
