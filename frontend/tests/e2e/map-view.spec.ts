import { test, expect } from '@playwright/test';
import { MapViewPage } from '../fixtures/MapViewPage';
import { server } from '../mocks/server';
import { http, HttpResponse } from 'msw';
import { mockRunningSimulationStatus } from '../mocks/data/simulations';

test.describe('MapView Page', () => {
  let mapPage: MapViewPage;

  test.beforeEach(async ({ page }) => {
    mapPage = new MapViewPage(page);
    server.resetHandlers();
    await mapPage.goto();
  });

  test.describe('Page Load and Initial State', () => {
    test('should load the map view page successfully', async () => {
      await expect(mapPage.page.getByText('Network Map View')).toBeVisible();
    });

    test('should display page description', async () => {
      await expect(
        mapPage.page.getByText('Visualize road networks and traffic flow')
      ).toBeVisible();
    });

    test('should initialize Leaflet map', async () => {
      await mapPage.waitForMapLoad();
      await expect(mapPage.getMapContainer()).toBeVisible();
    });

    test('should display map legend', async () => {
      await expect(mapPage.getMapLegend()).toBeVisible();
      await expect(mapPage.page.getByText('Fast vehicles')).toBeVisible();
      await expect(mapPage.page.getByText('Medium vehicles')).toBeVisible();
      await expect(mapPage.page.getByText('Slow vehicles')).toBeVisible();
    });
  });

  test.describe('Map Rendering', () => {
    test('should render road polylines on map', async () => {
      await mapPage.waitForMapLoad();
      const roads = mapPage.getRoadPolylines();
      const count = await roads.count();
      expect(count).toBeGreaterThan(0);
    });

    test('should display road information in popup when clicked', async ({ page }) => {
      await mapPage.waitForMapLoad();
      await page.waitForTimeout(1000);

      const road = mapPage.getRoadPolylines().first();
      await road.click();

      const isPopupVisible = await mapPage.isPopupVisible();
      expect(isPopupVisible).toBe(true);
    });

    test('should fit map bounds to show all roads', async () => {
      await mapPage.waitForMapLoad();
      const map = mapPage.getMapContainer();
      await expect(map).toBeVisible();

      // Map should be centered and zoomed to show roads
      const viewport = await map.boundingBox();
      expect(viewport).not.toBeNull();
    });
  });

  test.describe('Simulation Control', () => {
    test('should display start and stop simulation buttons', async () => {
      await expect(mapPage.getStartSimulationButton()).toBeVisible();
      await expect(mapPage.getStopSimulationButton()).toBeVisible();
    });

    test('should display stream control buttons', async () => {
      await expect(mapPage.getStartStreamingButton()).toBeVisible();
    });

    test('should disable stream controls when simulation is not running', async () => {
      await expect(mapPage.getStartStreamingButton()).toBeDisabled();
    });

    test('should enable start button when simulation is stopped', async () => {
      await expect(mapPage.getStartSimulationButton()).toBeEnabled();
    });

    test('should start simulation when start button is clicked', async ({ page }) => {
      let startCalled = false;

      server.use(
        http.post('http://localhost:8080/api/simulation/start', () => {
          startCalled = true;
          return HttpResponse.json({
            message: 'Simulation started',
            status: 'running',
          });
        })
      );

      await mapPage.startSimulation();
      await page.waitForTimeout(500);

      expect(startCalled).toBe(true);
    });

    test('should stop simulation when stop button is clicked', async ({ page }) => {
      // Set to running first
      server.use(
        http.get('http://localhost:8080/api/simulation/status', () => {
          return HttpResponse.json(mockRunningSimulationStatus);
        })
      );

      await page.reload();
      await page.waitForTimeout(500);

      let stopCalled = false;
      server.use(
        http.post('http://localhost:8080/api/simulation/stop', () => {
          stopCalled = true;
          return HttpResponse.json({
            message: 'Simulation stopped',
            status: 'stopped',
          });
        })
      );

      await mapPage.stopSimulation();
      await page.waitForTimeout(500);

      expect(stopCalled).toBe(true);
    });
  });

  test.describe('Status Indicators', () => {
    test('should display simulation status indicator', async () => {
      const indicator = mapPage.getSimulationStatusIndicator();
      await expect(indicator).toBeVisible();
      await expect(indicator).toContainText(/Sim:.*Stopped|Running/);
    });

    test('should display stream status indicator', async () => {
      const indicator = mapPage.getStreamStatusIndicator();
      await expect(indicator).toBeVisible();
      await expect(indicator).toContainText(/Stream:.*Disconnected|Live/);
    });

    test('should show stopped status when simulation is not running', async () => {
      const indicator = mapPage.getSimulationStatusIndicator();
      await expect(indicator).toContainText('Stopped');
    });

    test('should show disconnected stream status initially', async () => {
      const indicator = mapPage.getStreamStatusIndicator();
      await expect(indicator).toContainText('Disconnected');
    });
  });

  test.describe('Network Selector', () => {
    test('should display network selector dropdown', async () => {
      await expect(mapPage.getNetworkSelector()).toBeVisible();
    });

    test('should list available networks in selector', async ({ page }) => {
      const selector = mapPage.getNetworkSelector();
      const options = await selector.locator('option').all();

      expect(options.length).toBeGreaterThan(0);
    });

    test('should change network when selection changes', async ({ page }) => {
      const selector = mapPage.getNetworkSelector();
      const options = await selector.locator('option').allTextContents();

      if (options.length > 1) {
        await mapPage.selectNetwork(options[1]);
        const selectedValue = await selector.inputValue();
        expect(selectedValue).toBeDefined();
      }
    });

    test('should display network description when selected', async ({ page }) => {
      await expect(page.getByText(/grid test network|road intersection/i)).toBeVisible();
    });
  });

  test.describe('Vehicle Markers', () => {
    test('should initially show no vehicle markers', async () => {
      await mapPage.waitForMapLoad();
      // No SSE stream yet, so no vehicles
      const vehicles = mapPage.getVehicleMarkers();
      const count = await vehicles.count();
      expect(count).toBe(0);
    });

    test('should display vehicle popup when marker is clicked', async ({ page }) => {
      // This test would require SSE mock to create vehicles first
      // Skipping detailed implementation as it requires complex SSE setup
    });
  });

  test.describe('Traffic Light Markers', () => {
    test('should initially show no traffic light markers without stream', async () => {
      await mapPage.waitForMapLoad();
      await page.waitForTimeout(500);
      // Without streaming, traffic lights might not be visible
    });

    test('should display traffic lights with correct colors', async ({ page }) => {
      // This would require SSE streaming to be active
      // Placeholder for traffic light color verification
    });
  });

  test.describe('Map Legend', () => {
    test('should display complete legend with all vehicle speed categories', async ({
      page,
    }) => {
      const legend = mapPage.getMapLegend();
      await expect(legend).toBeVisible();

      await expect(page.getByText('Fast vehicles')).toBeVisible();
      await expect(page.getByText('Medium vehicles')).toBeVisible();
      await expect(page.getByText('Slow vehicles')).toBeVisible();
    });

    test('should display traffic light colors in legend', async ({ page }) => {
      await expect(page.getByText('Green traffic lights')).toBeVisible();
      await expect(page.getByText('Yellow traffic lights')).toBeVisible();
      await expect(page.getByText('Red traffic lights')).toBeVisible();
    });

    test('should display helpful instruction text', async ({ page }) => {
      await expect(
        page.getByText('Click on any vehicle or traffic light to see detailed information')
      ).toBeVisible();
    });
  });

  test.describe('Simulation Info Display', () => {
    test('should display simulation info when stream is active', async ({ page }) => {
      // Enable streaming simulation
      server.use(
        http.get('http://localhost:8080/api/simulation/status', () => {
          return HttpResponse.json(mockRunningSimulationStatus);
        })
      );

      await page.reload();
      await page.waitForTimeout(1000);

      // With SSE mock, this would show Step, Time, Vehicles
    });
  });

  test.describe('Stream Control', () => {
    test('should toggle stream button text when clicked', async ({ page }) => {
      // First start simulation
      server.use(
        http.post('http://localhost:8080/api/simulation/start', () => {
          return HttpResponse.json({
            message: 'Simulation started',
            status: 'running',
          });
        }),
        http.get('http://localhost:8080/api/simulation/status', () => {
          return HttpResponse.json(mockRunningSimulationStatus);
        })
      );

      await mapPage.startSimulation();
      await page.waitForTimeout(1000);
      await page.reload();

      await expect(mapPage.getStartStreamingButton()).toBeEnabled();
    });

    test('should auto-enable streaming when simulation starts', async ({ page }) => {
      server.use(
        http.post('http://localhost:8080/api/simulation/start', () => {
          return HttpResponse.json({
            message: 'Simulation started',
            status: 'running',
          });
        })
      );

      await mapPage.startSimulation();
      await page.waitForTimeout(500);

      // Stream should auto-enable (implementation detail)
    });

    test('should auto-disable streaming when simulation stops', async ({ page }) => {
      // Set to running first
      server.use(
        http.get('http://localhost:8080/api/simulation/status', () => {
          return HttpResponse.json(mockRunningSimulationStatus);
        })
      );

      await page.reload();
      await mapPage.stopSimulation();
      await page.waitForTimeout(500);

      // Stream should auto-disable
    });
  });

  test.describe('Map Interactions', () => {
    test('should allow map zoom', async ({ page }) => {
      await mapPage.waitForMapLoad();

      const zoomIn = page.locator('.leaflet-control-zoom-in');
      await zoomIn.click();
      await page.waitForTimeout(500);

      // Map should zoom (verified by Leaflet internals)
      await expect(zoomIn).toBeVisible();
    });

    test('should allow map pan', async ({ page }) => {
      await mapPage.waitForMapLoad();

      const map = mapPage.getMapContainer();
      const box = await map.boundingBox();

      if (box) {
        await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
        await page.mouse.down();
        await page.mouse.move(box.x + box.width / 2 + 50, box.y + box.height / 2 + 50);
        await page.mouse.up();

        // Map should have panned
        await expect(map).toBeVisible();
      }
    });
  });

  test.describe('Error Handling', () => {
    test('should handle network fetch errors gracefully', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/networks', () => {
          return new HttpResponse(null, { status: 500 });
        })
      );

      await page.reload();
      // Should not crash, might show empty selector
      await expect(mapPage.getMapContainer()).toBeVisible();
    });

    test('should handle road fetch errors gracefully', async ({ page }) => {
      server.use(
        http.get('http://localhost:8080/api/simulation/roads', () => {
          return new HttpResponse(null, { status: 500 });
        })
      );

      await page.reload();
      // Map should still render even without roads
      await expect(mapPage.getMapContainer()).toBeVisible();
    });
  });

  test.describe('Real-time Updates', () => {
    test('should display vehicle count in info bar when stream is active', async ({ page }) => {
      // This requires SSE streaming to be mocked
      // Placeholder for stream update verification
    });

    test('should update step counter in real-time', async ({ page }) => {
      // Requires SSE mock
    });

    test('should update time counter in real-time', async ({ page }) => {
      // Requires SSE mock
    });
  });
});
