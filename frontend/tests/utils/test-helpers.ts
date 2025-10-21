import { Page, expect } from '@playwright/test';

/**
 * Wait for a specific text to appear on the page
 */
export async function waitForText(page: Page, text: string, timeout = 5000) {
  await expect(page.getByText(text, { exact: false })).toBeVisible({ timeout });
}

/**
 * Wait for an element to disappear
 */
export async function waitForElementToDisappear(page: Page, selector: string, timeout = 5000) {
  await page.waitForSelector(selector, { state: 'hidden', timeout });
}

/**
 * Wait for network to be idle (no pending requests)
 */
export async function waitForNetworkIdle(page: Page, timeout = 5000) {
  await page.waitForLoadState('networkidle', { timeout });
}

/**
 * Take a screenshot and attach it to the test report
 */
export async function takeScreenshot(page: Page, name: string) {
  const screenshot = await page.screenshot();
  return screenshot;
}

/**
 * Fill form field and wait for it to update
 */
export async function fillAndWait(page: Page, selector: string, value: string) {
  await page.fill(selector, value);
  await page.waitForTimeout(100); // Small delay for React state updates
}

/**
 * Click and wait for navigation
 */
export async function clickAndNavigate(page: Page, selector: string) {
  await Promise.all([
    page.waitForNavigation(),
    page.click(selector),
  ]);
}

/**
 * Wait for a chart to render (Recharts)
 */
export async function waitForChartToRender(page: Page, timeout = 5000) {
  await page.waitForSelector('.recharts-wrapper', { timeout });
  await page.waitForSelector('svg', { timeout });
}

/**
 * Wait for map to be initialized (Leaflet)
 */
export async function waitForMapToRender(page: Page, timeout = 10000) {
  await page.waitForSelector('.leaflet-container', { timeout });
  await page.waitForSelector('.leaflet-tile-loaded', { timeout });
}

/**
 * Simulate a delay (useful for testing loading states)
 */
export async function delay(ms: number) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * Get all table rows as an array of objects
 */
export async function getTableData(page: Page, tableSelector: string = 'table') {
  const rows = await page.locator(`${tableSelector} tbody tr`).all();
  const data = [];

  for (const row of rows) {
    const cells = await row.locator('td').allTextContents();
    data.push(cells);
  }

  return data;
}

/**
 * Check if element has a specific class
 */
export async function hasClass(page: Page, selector: string, className: string) {
  const element = page.locator(selector);
  const classes = await element.getAttribute('class');
  return classes?.includes(className) ?? false;
}
