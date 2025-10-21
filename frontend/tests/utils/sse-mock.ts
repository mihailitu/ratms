import { Page } from '@playwright/test';

/**
 * Mock SSE (Server-Sent Events) stream for testing
 */
export class SSEMock {
  private page: Page;
  private intervalId: NodeJS.Timeout | null = null;

  constructor(page: Page) {
    this.page = page;
  }

  /**
   * Start sending mock SSE updates
   */
  async start(updateInterval = 100) {
    // Intercept SSE endpoint and send mock updates
    await this.page.route('**/api/simulation/stream', async (route) => {
      const body = this.generateSSEStream();
      await route.fulfill({
        status: 200,
        contentType: 'text/event-stream',
        body,
      });
    });
  }

  /**
   * Generate a mock SSE stream
   */
  private generateSSEStream(): string {
    const updates = [];

    // Generate 10 updates
    for (let i = 0; i < 10; i++) {
      const update = {
        step: i * 5,
        time: i * 0.5,
        vehicles: this.generateMockVehicles(5 + i),
        trafficLights: this.generateMockTrafficLights(),
      };

      updates.push(`event: update\ndata: ${JSON.stringify(update)}\n\n`);
    }

    return updates.join('');
  }

  /**
   * Generate mock vehicle data
   */
  private generateMockVehicles(count: number) {
    return Array.from({ length: count }, (_, i) => ({
      id: i,
      roadId: Math.floor(i / 3),
      lane: i % 2,
      position: (i * 50) % 500,
      velocity: 10 + Math.random() * 5,
      acceleration: -0.5 + Math.random() * 1,
    }));
  }

  /**
   * Generate mock traffic light data
   */
  private generateMockTrafficLights() {
    const states = ['R', 'Y', 'G'];
    return Array.from({ length: 4 }, (_, i) => ({
      roadId: i,
      lane: 0,
      state: states[i % 3],
      lat: 48.1351 + (i * 0.001),
      lon: 11.5820 + (i * 0.001),
    }));
  }

  /**
   * Stop the SSE mock
   */
  stop() {
    if (this.intervalId) {
      clearInterval(this.intervalId);
      this.intervalId = null;
    }
  }
}

/**
 * Helper function to create and start SSE mock
 */
export async function mockSSEStream(page: Page, updateInterval = 100) {
  const sseMock = new SSEMock(page);
  await sseMock.start(updateInterval);
  return sseMock;
}
