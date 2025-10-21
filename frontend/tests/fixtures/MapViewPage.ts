import { Page, Locator } from '@playwright/test';
import { BasePage } from './BasePage';

/**
 * MapView Page Object
 */
export class MapViewPage extends BasePage {
  constructor(page: Page) {
    super(page);
  }

  /**
   * Navigate to map view
   */
  async goto() {
    await super.goto('/map');
  }

  /**
   * Get Leaflet map container
   */
  getMapContainer(): Locator {
    return this.page.locator('.leaflet-container');
  }

  /**
   * Get simulation start button
   */
  getStartSimulationButton(): Locator {
    return this.page.getByRole('button', { name: 'Start Simulation' });
  }

  /**
   * Get simulation stop button
   */
  getStopSimulationButton(): Locator {
    return this.page.getByRole('button', { name: 'Stop Simulation' });
  }

  /**
   * Get stream start button
   */
  getStartStreamingButton(): Locator {
    return this.page.getByRole('button', { name: 'Start Streaming' });
  }

  /**
   * Get stream stop button
   */
  getStopStreamingButton(): Locator {
    return this.page.getByRole('button', { name: 'Stop Streaming' });
  }

  /**
   * Start simulation
   */
  async startSimulation() {
    await this.getStartSimulationButton().click();
  }

  /**
   * Stop simulation
   */
  async stopSimulation() {
    await this.getStopSimulationButton().click();
  }

  /**
   * Start streaming
   */
  async startStreaming() {
    await this.getStartStreamingButton().click();
  }

  /**
   * Stop streaming
   */
  async stopStreaming() {
    await this.getStopStreamingButton().click();
  }

  /**
   * Get simulation status indicator
   */
  getSimulationStatusIndicator(): Locator {
    return this.page.locator('text=Sim:').locator('..');
  }

  /**
   * Get stream status indicator
   */
  getStreamStatusIndicator(): Locator {
    return this.page.locator('text=Stream:').locator('..');
  }

  /**
   * Get simulation info (step, time, vehicles)
   */
  getSimulationInfo(): Locator {
    return this.page.locator('text=/Step:|Time:|Vehicles:/');
  }

  /**
   * Get network selector
   */
  getNetworkSelector(): Locator {
    return this.page.locator('select');
  }

  /**
   * Select network
   */
  async selectNetwork(networkName: string) {
    await this.getNetworkSelector().selectOption({ label: networkName });
  }

  /**
   * Get vehicle markers
   */
  getVehicleMarkers(): Locator {
    return this.page.locator('.leaflet-marker-icon');
  }

  /**
   * Get traffic light markers
   */
  getTrafficLightMarkers(): Locator {
    return this.page.locator('.leaflet-interactive[fill]');
  }

  /**
   * Get road polylines
   */
  getRoadPolylines(): Locator {
    return this.page.locator('.leaflet-interactive[stroke]');
  }

  /**
   * Get map legend
   */
  getMapLegend(): Locator {
    return this.page.getByText('Map Legend:').locator('..');
  }

  /**
   * Wait for map to load
   */
  async waitForMapLoad() {
    await this.getMapContainer().waitFor({ state: 'visible', timeout: 10000 });
    await this.page.waitForSelector('.leaflet-tile-loaded', { timeout: 10000 });
  }

  /**
   * Click on map at coordinates
   */
  async clickOnMap(x: number, y: number) {
    const map = await this.getMapContainer().boundingBox();
    if (map) {
      await this.page.mouse.click(map.x + x, map.y + y);
    }
  }

  /**
   * Get popup content
   */
  getPopup(): Locator {
    return this.page.locator('.leaflet-popup-content');
  }

  /**
   * Check if popup is visible
   */
  async isPopupVisible(): Promise<boolean> {
    try {
      await this.getPopup().waitFor({ timeout: 2000 });
      return true;
    } catch {
      return false;
    }
  }
}
