# CLAUDE.md

AI assistant guidance for RATMS (Real-time Adaptive Traffic Management System).

## Project Overview

C++20 traffic simulation with React/TypeScript dashboard. Implements IDM physics, genetic algorithm optimization, and real-time visualization via SSE streaming.

## Build Commands

```bash
# Backend
cd simulator/build && cmake .. && make api_server
./api_server  # http://localhost:8080

# Frontend
cd frontend && npm install && npm run dev  # http://localhost:5173

# GA Optimizer (standalone)
cd simulator/build && make ga_optimizer
./ga_optimizer --pop 50 --gen 100
```

## Test Commands

```bash
# Backend unit tests
cd simulator/build && make unit_tests && ./unit_tests

# Backend integration tests
cd simulator/build && make integration_tests && ./integration_tests

# Run all backend tests via CTest
cd simulator/build && ctest --output-on-failure

# Run specific test pattern
./unit_tests --gtest_filter="SimulatorTest.*"
./unit_tests --gtest_filter="*Fitness*"

# Frontend unit tests
cd frontend && npm test -- --run

# Frontend E2E tests (Playwright)
cd frontend && npm run test:e2e

# Type check + lint
cd frontend && npm run lint && npx tsc --noEmit
```

## Directory Structure

```
simulator/src/
  core/           # Simulator, Road, Vehicle, TrafficLight
  api/            # HTTP server, controllers
  optimization/   # GeneticAlgorithm, Metrics
  prediction/     # TrafficPredictor
  validation/     # TimingValidator
  metrics/        # TravelTimeCollector
  data/storage/   # DatabaseManager, TrafficPatternStorage

frontend/src/
  pages/          # Dashboard, MapView, Optimization, Analytics, Statistics
  components/     # UI components
  services/       # apiClient.ts
  types/          # TypeScript interfaces
```

## Critical Concepts

### Two-Phase Simulation Update

**Phase 1**: Update all roads (IDM physics, lane changes, traffic lights, collect transitions)
**Phase 2**: Execute road transitions (move vehicles between roads)

This pattern prevents iterator invalidation and double-updates.

### Thread Safety

All simulator access must be mutex-protected:
```cpp
std::lock_guard<std::mutex> lock(sim_mutex_);
// Access simulator_ safely
```

Exception: Streaming snapshot uses separate `snapshot_mutex_` to reduce contention.

### IDM Physics

Intelligent Driver Model calculates acceleration based on:
- Free-road: `a * (1 - pow(v/v0, delta))`
- Following: Adds term `- pow(s_star/s, 2)` for safe distance

Parameters: `v0` (desired velocity), `T` (headway), `a/b` (accel/decel), `s0` (min gap)

### GA Optimization

- **Chromosome**: Vector of (greenTime, redTime) per traffic light
- **Selection**: Tournament (size=3)
- **Crossover**: Uniform (80% rate)
- **Mutation**: Gaussian (15% rate)
- **Elitism**: Top 10% preserved
- **Fitness**: Lower is better (minimizes queue + wait, maximizes throughput)

## Key Files

| Purpose | File |
|---------|------|
| Server routes | `simulator/src/api/server.cpp:setupRoutes()` |
| Simulation loop | `simulator/src/api/server.cpp:runSimulationLoop()` |
| IDM physics | `simulator/src/core/vehicle.cpp:getNewAcceleration()` |
| GA engine | `simulator/src/optimization/genetic_algorithm.cpp` |
| Predictive optimizer | `simulator/src/api/predictive_optimizer.cpp` |
| API client | `frontend/src/services/apiClient.ts` |
| Type definitions | `frontend/src/types/api.ts` |
| E2E test mocks | `frontend/tests/helpers/apiMocks.ts` |

## API Endpoints (Summary)

See [PROJECT_STATUS.md](PROJECT_STATUS.md) for complete list.

Core endpoints:
- `GET/POST /api/simulation/*` - Start, stop, status, stream (SSE)
- `GET/POST /api/optimization/*` - GA optimization runs
- `GET/POST /api/traffic-lights` - Traffic light timings
- `GET/POST /api/spawn-rates` - Vehicle spawn rates
- `GET /api/prediction/*` - Traffic prediction
- `GET/POST /api/profiles/*` - Traffic profiles
- `GET/POST /api/travel-time/*` - O-D pair tracking

## Database

SQLite with migrations in `database/migrations/`. Key tables:
- `simulations`, `metrics` - Simulation data
- `optimization_runs`, `optimization_generations`, `optimization_solutions` - GA results
- `traffic_snapshots`, `traffic_patterns` - Time-of-day patterns

## Dependencies

**C++**: C++20, SQLite3, cpp-httplib, nlohmann/json, spdlog (header-only in `src/external/`)
**Frontend**: React 18, TypeScript, Vite, Tailwind CSS, Leaflet, Recharts, Axios

## Performance Notes

- Simulation step: dt=0.1s
- Snapshot capture: every 5 steps (~2 updates/sec)
- Pattern recording: every 60 real seconds
- SSE polling: 50ms sleep between checks
- Batch database writes for efficiency

## Known Limitations

1. Road polylines are straight (no curves)
2. Lane selection at intersections needs refinement
3. No pedestrian simulation

## Code Patterns

### Error Handling
Backend returns `{"success": false, "error": "message"}` on errors.
Frontend wraps API calls in try-catch.

### JSON Serialization
Use `nlohmann/json` with `.dump(2)` for pretty-printed responses.

### Background Threads
Use `std::thread` with proper lifecycle (join in destructor, atomic flags for stop).

## Development Notes

- Keep simulation loop fast (avoid I/O in hot path)
- Use move semantics for large objects
- Reserve vector capacity when size is known
- Test both backend and frontend after changes
- Update PROJECT_STATUS.md for significant changes
