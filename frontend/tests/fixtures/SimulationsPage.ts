import { Page, Locator } from '@playwright/test';
import { BasePage } from './BasePage';

/**
 * Simulations Page Object
 */
export class SimulationsPage extends BasePage {
  constructor(page: Page) {
    super(page);
  }

  /**
   * Navigate to simulations page
   */
  async goto() {
    await super.goto('/simulations');
  }

  /**
   * Get the simulations table
   */
  getTable(): Locator {
    return this.page.locator('table');
  }

  /**
   * Get all simulation rows
   */
  getSimulationRows(): Locator {
    return this.page.locator('table tbody tr');
  }

  /**
   * Get simulation row by ID
   */
  getSimulationRow(id: number): Locator {
    return this.page.locator(`tr:has-text("#${id}")`);
  }

  /**
   * Get view details button for a simulation
   */
  getViewDetailsButton(id: number): Locator {
    return this.getSimulationRow(id).getByRole('button', { name: 'View Details' });
  }

  /**
   * Click view details for a simulation
   */
  async viewDetails(id: number) {
    await this.getViewDetailsButton(id).click();
    await this.page.waitForLoadState('networkidle');
  }

  /**
   * Get status badge for a simulation
   */
  getStatusBadge(id: number): Locator {
    return this.getSimulationRow(id).locator('.inline-flex');
  }

  /**
   * Get simulation count
   */
  async getSimulationCount(): Promise<number> {
    return await this.getSimulationRows().count();
  }

  /**
   * Check if table is empty
   */
  async isEmpty(): Promise<boolean> {
    const emptyMessage = this.page.getByText('No simulations found');
    try {
      await emptyMessage.waitFor({ timeout: 2000 });
      return true;
    } catch {
      return false;
    }
  }

  /**
   * Get table headers
   */
  getHeaders(): Locator {
    return this.page.locator('table thead th');
  }

  /**
   * Get simulation name
   */
  getSimulationName(id: number): Locator {
    return this.getSimulationRow(id).locator('td').nth(1);
  }
}
