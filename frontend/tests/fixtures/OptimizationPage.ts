import { Page, Locator } from '@playwright/test';
import { BasePage } from './BasePage';

/**
 * Optimization Page Object
 */
export class OptimizationPage extends BasePage {
  constructor(page: Page) {
    super(page);
  }

  /**
   * Navigate to optimization page
   */
  async goto() {
    await super.goto('/optimization');
  }

  /**
   * Get form input by label
   */
  getFormInput(label: string): Locator {
    return this.page.getByLabel(label);
  }

  /**
   * Get population size input
   */
  getPopulationSizeInput(): Locator {
    return this.getFormInput('Population Size');
  }

  /**
   * Get generations input
   */
  getGenerationsInput(): Locator {
    return this.getFormInput('Generations');
  }

  /**
   * Get mutation rate input
   */
  getMutationRateInput(): Locator {
    return this.getFormInput('Mutation Rate');
  }

  /**
   * Get crossover rate input
   */
  getCrossoverRateInput(): Locator {
    return this.getFormInput('Crossover Rate');
  }

  /**
   * Get simulation steps input
   */
  getSimulationStepsInput(): Locator {
    return this.getFormInput('Simulation Steps');
  }

  /**
   * Get network ID input
   */
  getNetworkIdInput(): Locator {
    return this.getFormInput('Network ID');
  }

  /**
   * Get start optimization button
   */
  getStartButton(): Locator {
    return this.page.getByRole('button', { name: /Start Optimization|Starting\.\.\./ });
  }

  /**
   * Fill optimization form
   */
  async fillForm(params: {
    populationSize?: number;
    generations?: number;
    mutationRate?: number;
    crossoverRate?: number;
    simulationSteps?: number;
    networkId?: number;
  }) {
    if (params.populationSize !== undefined) {
      await this.getPopulationSizeInput().fill(params.populationSize.toString());
    }
    if (params.generations !== undefined) {
      await this.getGenerationsInput().fill(params.generations.toString());
    }
    if (params.mutationRate !== undefined) {
      await this.getMutationRateInput().fill(params.mutationRate.toString());
    }
    if (params.crossoverRate !== undefined) {
      await this.getCrossoverRateInput().fill(params.crossoverRate.toString());
    }
    if (params.simulationSteps !== undefined) {
      await this.getSimulationStepsInput().fill(params.simulationSteps.toString());
    }
    if (params.networkId !== undefined) {
      await this.getNetworkIdInput().fill(params.networkId.toString());
    }
  }

  /**
   * Start optimization
   */
  async startOptimization() {
    await this.getStartButton().click();
  }

  /**
   * Get optimization history section
   */
  getHistorySection(): Locator {
    return this.page.getByText('Optimization History').locator('..');
  }

  /**
   * Get run by ID - returns the full run card container
   */
  getRun(runId: number): Locator {
    // Find the run card by looking for a container that has "Run #X" text
    return this.page.locator('.border.rounded-lg.p-4').filter({ hasText: `Run #${runId}` });
  }

  /**
   * Click on run to view details
   */
  async viewRunDetails(runId: number) {
    await this.getRun(runId).click();
  }

  /**
   * Get stop button for a run
   */
  getStopButton(runId: number): Locator {
    return this.getRun(runId).getByRole('button', { name: 'Stop' });
  }

  /**
   * Stop optimization run
   */
  async stopRun(runId: number) {
    await this.getStopButton(runId).click();
  }

  /**
   * Get run status badge
   */
  getRunStatus(runId: number): Locator {
    return this.getRun(runId).locator('.text-xs.px-2.py-1.rounded');
  }

  /**
   * Get progress bar for running optimization
   */
  getProgressBar(runId: number): Locator {
    return this.getRun(runId).locator('.bg-blue-600');
  }

  /**
   * Get results section
   */
  getResultsSection(): Locator {
    return this.page.getByText(/Run #\d+ Details/).locator('..');
  }

  /**
   * Get fitness improvement value
   */
  getImprovementValue(): Locator {
    return this.page.locator('text=Improvement:').locator('..').locator('.text-green-600');
  }

  /**
   * Get GA visualizer chart
   */
  getGAVisualizerChart(): Locator {
    return this.page.locator('.recharts-wrapper');
  }

  /**
   * Get parameters section
   */
  getParametersSection(): Locator {
    return this.page.getByText('Parameters').locator('..');
  }

  /**
   * Check if run is selected (has the blue border selection style)
   */
  async isRunSelected(runId: number): Promise<boolean> {
    // Use a more specific selector that looks for the selection state
    const selectedRun = this.page.locator('.border-blue-500.bg-blue-50').filter({ hasText: `Run #${runId}` });
    return await selectedRun.isVisible();
  }

  /**
   * Get empty state message
   */
  getEmptyState(): Locator {
    return this.page.getByText('No optimization runs yet');
  }
}
