# RATMS Frontend E2E Test Suite

Comprehensive Playwright end-to-end tests for the RATMS (Real-time Adaptive Traffic Management System) frontend application.

## Overview

This test suite provides complete coverage of all frontend pages and functionality using:
- **Playwright** for E2E testing
- **MSW (Mock Service Worker)** for API mocking
- **Page Object Pattern** for maintainable tests
- **Real Backend Integration Tests** for critical flows

## Test Structure

```
tests/
├── mocks/                    # MSW mock server and handlers
│   ├── data/                # Test data fixtures
│   │   ├── simulations.ts
│   │   ├── networks.ts
│   │   ├── optimization.ts
│   │   └── metrics.ts
│   ├── handlers.ts          # MSW request handlers
│   └── server.ts            # MSW server setup
├── fixtures/                # Page Object Models
│   ├── BasePage.ts
│   ├── DashboardPage.ts
│   ├── SimulationsPage.ts
│   ├── MapViewPage.ts
│   ├── OptimizationPage.ts
│   └── AnalyticsPage.ts
├── utils/                   # Test utilities
│   ├── test-helpers.ts     # Common test helpers
│   ├── sse-mock.ts         # SSE streaming mock
│   └── data-factories.ts   # Test data generators
├── e2e/                    # Main E2E tests (with MSW)
│   ├── dashboard.spec.ts
│   ├── simulations.spec.ts
│   ├── map-view.spec.ts    # Comprehensive map tests
│   ├── optimization.spec.ts
│   ├── analytics.spec.ts
│   ├── navigation.spec.ts
│   └── simulation-detail.spec.ts
└── e2e-integration/        # Integration tests (real backend)
    └── api-integration.spec.ts
```

## Running Tests

### All E2E Tests (with MSW mocking)
```bash
npm run test:e2e
```

### Interactive Mode (Playwright UI)
```bash
npm run test:e2e:ui
```

### Headed Mode (see browser)
```bash
npm run test:e2e:headed
```

### Debug Mode
```bash
npm run test:e2e:debug
```

### Integration Tests (requires real backend)
```bash
# 1. Start the C++ API server
cd ../simulator/build
./api_server

# 2. Run integration tests
cd ../../frontend
npm run test:e2e:integration
```

### View Test Report
```bash
npm run test:e2e:report
```

## Test Coverage

### Dashboard Tests (~50 tests)
- Page load and initial state
- Simulation status display (running/stopped)
- Simulation control (start/stop buttons)
- Networks section display
- Metrics cards
- Error handling
- Status polling (2s intervals)

### Simulations Tests (~20 tests)
- List rendering
- Table display with all columns
- Status badges (completed/running/failed)
- Navigation to detail page
- Empty states
- Error handling

### MapView Tests (~40 tests) - COMPREHENSIVE
- Leaflet map initialization
- Road polylines rendering
- Vehicle markers with speed-based colors
- Traffic light markers with state colors (R/Y/G)
- Real-time SSE updates
- Simulation start/stop controls
- Stream enable/disable
- Map interactions (zoom, pan, click)
- Marker popups
- Network selector
- Legend display
- Error handling

### Optimization Tests (~30 tests)
- Form rendering with default values
- Form validation
- Start optimization
- Run history display
- Progress tracking
- Stop running optimization
- Results display (fitness improvement, charts)
- GA Visualizer chart rendering
- Status badges
- Empty states

### Analytics Tests (~25 tests)
- Simulation selection (checkboxes)
- Metric type selection
- Compare button functionality
- Comparison chart rendering (Recharts)
- Statistics table display (min, max, mean, median, std dev, percentiles)
- CSV export
- Multiple simulation comparison
- Error handling

### Navigation Tests (~10 tests)
- All navigation links
- Route changes
- Browser back/forward
- Layout persistence
- Direct URL access

### Integration Tests (real backend)
- Health check
- Start/stop simulation flow
- SSE streaming with real updates
- Optimization workflow
- Database persistence

## Test Patterns

### Page Object Pattern
```typescript
const dashboardPage = new DashboardPage(page);
await dashboardPage.goto();
await dashboardPage.startSimulation();
await expect(dashboardPage.getSimulationStatus()).toContainText('Running');
```

### MSW Mocking
```typescript
server.use(
  http.get('http://localhost:8080/api/simulation/status', () => {
    return HttpResponse.json(mockRunningSimulationStatus);
  })
);
```

### Assertions
```typescript
await expect(page.getByText('RATMS Dashboard')).toBeVisible();
await expect(button).toBeEnabled();
await expect(status).toContainText('Running');
```

## Configuration

### Playwright Config (`playwright.config.ts`)
- **Browser**: Chromium only
- **Base URL**: http://localhost:5173
- **Timeout**: 30s per test
- **Retries**: 2 on CI, 0 locally
- **Web Server**: Auto-starts Vite dev server
- **Reporters**: HTML + list
- **Screenshots**: On failure
- **Videos**: On failure

### MSW Integration
- Global setup starts MSW server before all tests
- Global teardown closes MSW server after all tests
- All API endpoints mocked with realistic data
- Request handlers in `tests/mocks/handlers.ts`

## Writing New Tests

### 1. Create Page Object (if needed)
```typescript
// tests/fixtures/NewPage.ts
export class NewPage extends BasePage {
  async goto() {
    await super.goto('/new-route');
  }

  getButton(): Locator {
    return this.page.getByRole('button', { name: 'Click Me' });
  }
}
```

### 2. Create Test File
```typescript
// tests/e2e/new-feature.spec.ts
import { test, expect } from '@playwright/test';
import { NewPage } from '../fixtures/NewPage';

test.describe('New Feature', () => {
  let newPage: NewPage;

  test.beforeEach(async ({ page }) => {
    newPage = new NewPage(page);
    await newPage.goto();
  });

  test('should do something', async () => {
    await expect(newPage.getButton()).toBeVisible();
  });
});
```

### 3. Add Mock Data (if needed)
```typescript
// tests/mocks/data/new-data.ts
export const mockNewData = {
  id: 1,
  name: 'Test',
};
```

### 4. Add Handler (if needed)
```typescript
// tests/mocks/handlers.ts
http.get(`${BASE_URL}/api/new-endpoint`, () => {
  return HttpResponse.json(mockNewData);
}),
```

## Common Issues

### Test Timeout
If tests timeout, increase the timeout in playwright.config.ts or individual tests:
```typescript
test('long running test', async ({ page }) => {
  test.setTimeout(60000); // 60 seconds
  // ... test code
});
```

### MSW Not Mocking
Ensure the URL in handlers.ts matches exactly:
```typescript
http.get('http://localhost:8080/api/endpoint', ...)  // Correct
http.get('/api/endpoint', ...)  // Wrong - needs full URL
```

### Map Tests Failing
Leaflet requires tiles to load. Use `waitForMapLoad()` helper:
```typescript
await mapPage.waitForMapLoad();
```

### Integration Tests Failing
Ensure the C++ API server is running:
```bash
cd simulator/build && ./api_server
```

## Best Practices

1. **Use Page Objects** - Encapsulate page interactions
2. **Descriptive Test Names** - Use clear, specific test names
3. **One Assertion Per Test** - Keep tests focused
4. **Reset State** - Use beforeEach to reset between tests
5. **Wait for Elements** - Use Playwright's auto-waiting
6. **Avoid Hard Waits** - Use `waitFor` instead of `setTimeout`
7. **Mock Consistently** - Keep mock data realistic
8. **Test User Flows** - Not implementation details

## Debugging

### Run Single Test
```bash
npx playwright test -g "should load dashboard"
```

### Debug Mode
```bash
npm run test:e2e:debug
```

### Headed Mode
```bash
npm run test:e2e:headed
```

### Trace Viewer
```bash
npx playwright show-trace trace.zip
```

## CI/CD Integration

Tests are configured to run in CI with:
- Retries: 2
- Workers: 1 (sequential)
- Reporter: HTML

Example GitHub Actions:
```yaml
- name: Run E2E Tests
  run: npm run test:e2e

- name: Upload Test Report
  if: always()
  uses: actions/upload-artifact@v3
  with:
    name: playwright-report
    path: playwright-report/
```

## Test Statistics

- **Total Test Files**: 8
- **Estimated Test Count**: 120-150 tests
- **Average Test Duration**: 2-5 seconds
- **Full Suite Duration**: ~5-10 minutes (with parallelization)

## Maintenance

### Updating Mock Data
1. Edit files in `tests/mocks/data/`
2. Update handlers in `tests/mocks/handlers.ts` if endpoints change
3. Update page objects if UI changes

### Adding New Endpoints
1. Add mock data in `tests/mocks/data/`
2. Add handler in `tests/mocks/handlers.ts`
3. Update TypeScript types in `src/types/api.ts`

## Resources

- [Playwright Documentation](https://playwright.dev)
- [MSW Documentation](https://mswjs.io)
- [Testing Best Practices](https://playwright.dev/docs/best-practices)
