import { Page, Locator } from '@playwright/test';
import { BasePage } from './BasePage';

/**
 * Analytics Page Object
 */
export class AnalyticsPage extends BasePage {
  constructor(page: Page) {
    super(page);
  }

  /**
   * Navigate to analytics page
   */
  async goto() {
    await super.goto('/analytics');
  }

  /**
   * Get simulation checkbox
   */
  getSimulationCheckbox(simulationId: number): Locator {
    return this.page.locator(`input[type="checkbox"]`).filter({
      has: this.page.locator(`text=/Sim ${simulationId}|Simulation ${simulationId}/`),
    });
  }

  /**
   * Select simulation
   */
  async selectSimulation(simulationId: number) {
    await this.getSimulationCheckbox(simulationId).check();
  }

  /**
   * Deselect simulation
   */
  async deselectSimulation(simulationId: number) {
    await this.getSimulationCheckbox(simulationId).uncheck();
  }

  /**
   * Get metric type selector
   */
  getMetricTypeSelector(): Locator {
    return this.page.getByLabel('Metric Type');
  }

  /**
   * Select metric type
   */
  async selectMetricType(metricType: string) {
    await this.getMetricTypeSelector().selectOption(metricType);
  }

  /**
   * Get compare button
   */
  getCompareButton(): Locator {
    return this.page.getByRole('button', { name: /Compare Simulations|Loading\.\.\./ });
  }

  /**
   * Click compare button
   */
  async clickCompare() {
    await this.getCompareButton().click();
  }

  /**
   * Get comparison chart
   */
  getComparisonChart(): Locator {
    return this.page.locator('.recharts-wrapper');
  }

  /**
   * Get statistics table
   */
  getStatisticsTable(): Locator {
    return this.page.locator('table');
  }

  /**
   * Get statistics table row
   */
  getStatisticsRow(simulationName: string): Locator {
    return this.page.locator(`tr:has-text("${simulationName}")`);
  }

  /**
   * Get export button for simulation
   */
  getExportButton(simulationName: string): Locator {
    return this.getStatisticsRow(simulationName).getByRole('button', { name: 'Export CSV' });
  }

  /**
   * Export metrics for simulation
   */
  async exportMetrics(simulationName: string) {
    const downloadPromise = this.page.waitForEvent('download');
    await this.getExportButton(simulationName).click();
    const download = await downloadPromise;
    return download;
  }

  /**
   * Get empty state message
   */
  getEmptyState(): Locator {
    return this.page.getByText('No completed simulations available');
  }

  /**
   * Get chart title
   */
  getChartTitle(): Locator {
    return this.page.locator('h2:has-text("Comparison Chart")');
  }

  /**
   * Get statistics table headers
   */
  getTableHeaders(): Locator {
    return this.page.locator('table thead th');
  }

  /**
   * Wait for chart to render
   */
  async waitForChartRender() {
    await this.getComparisonChart().waitFor({ timeout: 5000 });
  }

  /**
   * Wait for table to render
   */
  async waitForTableRender() {
    await this.getStatisticsTable().waitFor({ timeout: 5000 });
  }

  /**
   * Get all checkboxes
   */
  getAllCheckboxes(): Locator {
    return this.page.locator('input[type="checkbox"]');
  }

  /**
   * Get count of selected simulations
   */
  async getSelectedCount(): Promise<number> {
    const checkboxes = await this.getAllCheckboxes().all();
    let count = 0;
    for (const checkbox of checkboxes) {
      if (await checkbox.isChecked()) {
        count++;
      }
    }
    return count;
  }
}
