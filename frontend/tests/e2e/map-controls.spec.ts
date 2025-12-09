import { test, expect } from '@playwright/test';
import { setupApiMocks, setupControlMocks } from '../helpers/apiMocks';

test.describe('Map View Control Panels', () => {
  test.beforeEach(async ({ page }) => {
    await setupApiMocks(page);
    await setupControlMocks(page);
    await page.goto('/map');
    await page.waitForSelector('text=Network Map View', { timeout: 10000 });
  });

  test.describe('Traffic Light Panel', () => {
    test('should display Traffic Lights toggle button', async ({ page }) => {
      await expect(page.getByRole('button', { name: 'Traffic Lights' })).toBeVisible();
    });

    test('should open traffic light panel when button is clicked', async ({ page }) => {
      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await expect(page.getByText('Traffic Lights').first()).toBeVisible();
    });

    test('should display traffic lights grouped by road', async ({ page }) => {
      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await page.waitForTimeout(500);

      // Check for road groupings
      await expect(page.getByText('Road 1')).toBeVisible();
      await expect(page.getByText('Road 2')).toBeVisible();
    });

    test('should display traffic light timing values', async ({ page }) => {
      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await page.waitForTimeout(500);

      // Should show timing format (green/yellow/red)
      await expect(page.getByText(/\d+s.*\/.*\d+s.*\/.*\d+s/).first()).toBeVisible();
    });

    test('should display current state indicator', async ({ page }) => {
      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await page.waitForTimeout(500);

      // Should show Green, Yellow, or Red state
      const stateIndicator = page.locator('.bg-green-500, .bg-yellow-500, .bg-red-500').first();
      await expect(stateIndicator).toBeVisible();
    });

    test('should show edit controls when Edit is clicked', async ({ page }) => {
      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await page.waitForTimeout(500);

      // Click first Edit button
      await page.getByRole('button', { name: 'Edit' }).first().click();

      // Should show input fields
      await expect(page.getByLabel('Green')).toBeVisible();
      await expect(page.getByLabel('Yellow')).toBeVisible();
      await expect(page.getByLabel('Red')).toBeVisible();
    });

    test('should save traffic light changes', async ({ page }) => {
      let saveRequested = false;

      await page.route('http://localhost:8080/api/traffic-lights', async (route) => {
        if (route.request().method() === 'POST') {
          saveRequested = true;
          await route.fulfill({
            status: 200,
            contentType: 'application/json',
            body: JSON.stringify({ success: true, updated: 1 }),
          });
        } else {
          await route.continue();
        }
      });

      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await page.waitForTimeout(500);

      await page.getByRole('button', { name: 'Edit' }).first().click();
      await page.getByRole('button', { name: 'Save' }).click();
      await page.waitForTimeout(300);

      expect(saveRequested).toBe(true);
    });

    test('should close panel when toggle is clicked again', async ({ page }) => {
      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await page.waitForTimeout(300);

      // Panel should be visible
      const panel = page.locator('.fixed.right-0');
      await expect(panel).toBeVisible();

      // Click toggle again
      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await page.waitForTimeout(300);

      // Panel content should be hidden (w-0)
      await expect(panel).toHaveClass(/w-0/);
    });
  });

  test.describe('Spawn Rate Panel', () => {
    test('should display Spawn Rates toggle button', async ({ page }) => {
      await expect(page.getByRole('button', { name: 'Spawn Rates' })).toBeVisible();
    });

    test('should open spawn rate panel when button is clicked', async ({ page }) => {
      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await expect(page.getByText('Spawn Rates').first()).toBeVisible();
    });

    test('should display add spawn point section', async ({ page }) => {
      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await page.waitForTimeout(500);

      await expect(page.getByText('Add Spawn Point')).toBeVisible();
      await expect(page.getByText('Select a road...')).toBeVisible();
    });

    test('should display active spawn points', async ({ page }) => {
      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await page.waitForTimeout(500);

      await expect(page.getByText('Active Spawn Points')).toBeVisible();
      // Should show vehicles/min for active spawn points
      await expect(page.getByText(/vehicles\/min/).first()).toBeVisible();
    });

    test('should allow adding new spawn point', async ({ page }) => {
      let addRequested = false;

      await page.route('http://localhost:8080/api/spawn-rates', async (route) => {
        if (route.request().method() === 'POST') {
          addRequested = true;
          const body = JSON.parse(route.request().postData() || '{}');
          await route.fulfill({
            status: 200,
            contentType: 'application/json',
            body: JSON.stringify({ success: true, updated: body.rates?.length || 0 }),
          });
        } else {
          await route.continue();
        }
      });

      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await page.waitForTimeout(500);

      // Select a road from dropdown
      await page.selectOption('select', { index: 1 });

      // Click Add button
      await page.getByRole('button', { name: 'Add Spawn Point' }).click();
      await page.waitForTimeout(300);

      expect(addRequested).toBe(true);
    });

    test('should allow editing spawn rate', async ({ page }) => {
      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await page.waitForTimeout(500);

      // Click edit on first spawn point
      const editButton = page.getByRole('button', { name: 'Edit' }).first();
      if (await editButton.isVisible()) {
        await editButton.click();

        // Should show input and save/cancel buttons
        await expect(page.getByRole('button', { name: 'Save' })).toBeVisible();
        await expect(page.getByRole('button', { name: 'Cancel' })).toBeVisible();
      }
    });

    test('should allow removing spawn point', async ({ page }) => {
      let removeRequested = false;

      await page.route('http://localhost:8080/api/spawn-rates', async (route) => {
        if (route.request().method() === 'POST') {
          const body = JSON.parse(route.request().postData() || '{}');
          // Check if it's a remove request (rate set to 0)
          if (body.rates?.some((r: { vehiclesPerMinute: number }) => r.vehiclesPerMinute === 0)) {
            removeRequested = true;
          }
          await route.fulfill({
            status: 200,
            contentType: 'application/json',
            body: JSON.stringify({ success: true, updated: 1 }),
          });
        } else {
          await route.continue();
        }
      });

      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await page.waitForTimeout(500);

      // Click remove on first spawn point
      const removeButton = page.getByRole('button', { name: 'Remove' }).first();
      if (await removeButton.isVisible()) {
        await removeButton.click();
        await page.waitForTimeout(300);
        expect(removeRequested).toBe(true);
      }
    });

    test('should close panel when toggle is clicked again', async ({ page }) => {
      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await page.waitForTimeout(300);

      // Panel should be visible
      const panel = page.locator('.fixed.left-0');
      await expect(panel).toBeVisible();

      // Click toggle again
      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await page.waitForTimeout(300);

      // Panel content should be hidden (w-0)
      await expect(panel).toHaveClass(/w-0/);
    });
  });

  test.describe('Panel Interactions', () => {
    test('should be able to open both panels simultaneously', async ({ page }) => {
      await page.getByRole('button', { name: 'Traffic Lights' }).click();
      await page.getByRole('button', { name: 'Spawn Rates' }).click();
      await page.waitForTimeout(300);

      // Both panels should be visible
      await expect(page.locator('.fixed.right-0.w-80')).toBeVisible();
      await expect(page.locator('.fixed.left-0.w-80')).toBeVisible();
    });

    test('should highlight toggle buttons when panels are open', async ({ page }) => {
      // Initially buttons should have gray background
      const trafficBtn = page.getByRole('button', { name: 'Traffic Lights' });
      const spawnBtn = page.getByRole('button', { name: 'Spawn Rates' });

      await expect(trafficBtn).toHaveClass(/bg-gray-200/);
      await expect(spawnBtn).toHaveClass(/bg-gray-200/);

      // Open panels
      await trafficBtn.click();
      await spawnBtn.click();

      // Buttons should now have colored backgrounds
      await expect(trafficBtn).toHaveClass(/bg-amber-600/);
      await expect(spawnBtn).toHaveClass(/bg-purple-600/);
    });
  });
});
