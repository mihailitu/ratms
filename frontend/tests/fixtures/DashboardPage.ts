import { Page, Locator } from '@playwright/test';
import { BasePage } from './BasePage';

/**
 * Dashboard Page Object
 */
export class DashboardPage extends BasePage {
  constructor(page: Page) {
    super(page);
  }

  /**
   * Navigate to dashboard
   */
  async goto() {
    await super.goto('/');
  }

  /**
   * Get simulation status text
   */
  getSimulationStatus(): Locator {
    return this.page.locator('text=/Running|Stopped/').first();
  }

  /**
   * Get road count metric
   */
  getRoadCount(): Locator {
    return this.page.getByText('Road Count').locator('..').locator('div').nth(1);
  }

  /**
   * Get server status
   */
  getServerStatus(): Locator {
    return this.page.locator('text=/Online|Offline/').first();
  }

  /**
   * Get start simulation button
   */
  getStartButton(): Locator {
    return this.page.getByRole('button', { name: 'Start Simulation' });
  }

  /**
   * Get stop simulation button
   */
  getStopButton(): Locator {
    return this.page.getByRole('button', { name: 'Stop Simulation' });
  }

  /**
   * Start simulation
   */
  async startSimulation() {
    await this.getStartButton().click();
  }

  /**
   * Stop simulation
   */
  async stopSimulation() {
    await this.getStopButton().click();
  }

  /**
   * Get networks section
   */
  getNetworksSection(): Locator {
    return this.page.getByText('Networks').locator('..');
  }

  /**
   * Get network by name
   */
  getNetwork(name: string): Locator {
    return this.page.getByText(name);
  }

  /**
   * Check if network exists
   */
  async hasNetwork(name: string): Promise<boolean> {
    try {
      await this.getNetwork(name).waitFor({ timeout: 2000 });
      return true;
    } catch {
      return false;
    }
  }

  /**
   * Get metrics card
   */
  getMetricsCard(title: string): Locator {
    return this.page.getByText(title).locator('..');
  }

  /**
   * Wait for status to update
   */
  async waitForStatus(status: 'Running' | 'Stopped', timeout = 5000) {
    await this.page.waitForSelector(`text=${status}`, { timeout });
  }
}
