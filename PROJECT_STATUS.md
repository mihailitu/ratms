# RATMS Project Status

**Last Updated:** December 2025
**Architecture:** Modular Monolith

## Overview

Real-time Adaptive Traffic Management System (RATMS) - A production-ready traffic simulation system implementing the Intelligent Driver Model (IDM) with genetic algorithm optimization and web-based monitoring.

## Completed Phases

| Phase | Description | Key Features |
|-------|-------------|--------------|
| 1 | Code Restructuring | Modular organization: core/, utils/, api/, optimization/, data/ |
| 2 | REST API Server | cpp-httplib server, CORS, multi-threaded, graceful shutdown |
| 3 | Database Integration | SQLite3, migrations, prepared statements, metrics persistence |
| 4 | Frontend Dashboard | React + TypeScript + Vite, Tailwind CSS, Recharts, Leaflet |
| 5 | Genetic Algorithm | GA engine for traffic light timing, fitness evaluation, CSV export |
| 6 | Frontend GA Integration | Real-time progress tracking, parameter config, visualization |
| 7 | Simulation Loop | Background thread execution, periodic metrics collection |
| 8 | Real-time Streaming | SSE for live vehicle/traffic light updates (~2 updates/sec) |
| 9 | Map Visualization | Road polylines, vehicle markers, speed-based coloring |
| 10 | GA Database Persistence | Optimization history survives restarts, 3-table schema |
| 11 | City Grid Network | 10x10 grid, 100 intersections, 360 roads, 1000 vehicles |
| 12 | Analytics Dashboard | Multi-simulation comparison, statistics, CSV export |
| 13 | E2E Testing | Playwright + MSW, 157 tests, Page Object Model |
| 14 | Logging System | spdlog with components, REQUEST_SCOPE, TIMED_SCOPE |
| 15 | Production Plan Stage 1 | Density-based map visualization, Speed/Density toggle |
| 16 | Build System | Dependency install script, BUILD.md documentation |
| 17 | Production Plan Stage 2 | Runtime traffic light timing modification API |
| 18 | Production Plan Stage 3 | Dynamic vehicle spawning API with configurable rates |
| 19 | Control Panel UI | Traffic light editor, spawn rate controls in MapView |
| 20 | Traffic Pattern Storage | Time-of-day pattern storage, snapshot recording, aggregation |
| 21 | Prediction Service | TrafficPredictor blends patterns with current state, 10-120min horizon |
| 22 | Predictive Optimizer | PredictiveOptimizer runs GA on predicted future traffic state |
| 23 | Validation & Rollout | TimingValidator, RolloutMonitor, auto-rollback on regression |

## Current Features

- IDM physics simulation with lane changing
- Traffic light control with GA optimization
- REST API (15+ endpoints) with SSE streaming
- SQLite persistence with migrations
- React dashboard with real-time updates
- Interactive map with vehicle tracking (speed & density modes)
- Traffic light timing editor with live updates
- Dynamic vehicle spawning with configurable rates
- Traffic pattern storage with time-of-day aggregation
- Traffic prediction service with configurable horizon (10-120min)
- Predictive optimization (GA on predicted future traffic state)
- Timing validation before applying optimizations
- Rollout monitoring with auto-rollback on regression
- Analytics with percentile statistics (P25, P50, P75, P95)
- Comprehensive E2E test coverage
- Build system with dependency installation script

## Quick Start

```bash
# Backend
cd simulator/build && cmake .. && make api_server
./api_server  # http://localhost:8080

# Frontend
cd frontend && npm install && npm run dev  # http://localhost:5173

# Tests
cd frontend && npm run test:e2e
```

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | /api/health | Health check |
| GET | /api/simulation/status | Current status |
| POST | /api/simulation/start | Start simulation |
| POST | /api/simulation/stop | Stop simulation |
| GET | /api/simulation/stream | SSE vehicle updates |
| GET | /api/simulation/roads | Road network data |
| GET | /api/simulations | Simulation history |
| POST | /api/optimization/start | Start GA optimization |
| GET | /api/optimization/status/:id | Optimization progress |
| GET | /api/analytics/simulations/:id/statistics | Metrics statistics |
| POST | /api/analytics/compare | Compare simulations |
| GET | /api/traffic-lights | Get all traffic light timings |
| POST | /api/traffic-lights | Bulk update traffic light timings |
| GET | /api/spawn-rates | Get vehicle spawn rates per road |
| POST | /api/spawn-rates | Set vehicle spawn rates per road |
| GET | /api/patterns | Get traffic patterns by day/time slot |
| GET | /api/snapshots | Get raw traffic snapshots |
| POST | /api/patterns/aggregate | Aggregate snapshots into patterns |
| POST | /api/patterns/prune | Prune old snapshots |
| GET | /api/prediction/current | Current time slot prediction |
| GET | /api/prediction/forecast | Prediction for T+N minutes |
| GET | /api/prediction/road/:id | Per-road prediction |
| GET | /api/prediction/config | Prediction configuration |
| POST | /api/prediction/config | Update prediction config |
| GET | /api/optimization/rollout/status | Rollout monitoring state |
| POST | /api/optimization/rollback | Manual rollback to previous timings |
| GET | /api/optimization/validation/config | Validation configuration |
| POST | /api/optimization/validation/config | Update validation config |

## File Structure

```
ratms/
├── simulator/src/
│   ├── core/           # Simulator, Road, Vehicle, TrafficLight
│   ├── api/            # REST server, OptimizationController, PredictiveOptimizer
│   ├── optimization/   # GeneticAlgorithm, Metrics
│   ├── prediction/     # TrafficPredictor
│   ├── validation/     # TimingValidator
│   ├── data/storage/   # DatabaseManager, TrafficPatternStorage
│   ├── utils/          # Logger, Config
│   └── tests/          # Test networks
├── frontend/src/
│   ├── pages/          # Dashboard, Map, Optimization, Analytics
│   ├── services/       # API client
│   └── types/          # TypeScript interfaces
├── database/migrations/
└── docs/
```

## Technical Stack

**Backend:** C++20, SQLite3, cpp-httplib, nlohmann/json, spdlog
**Frontend:** React 18, TypeScript, Vite, Tailwind CSS, Leaflet, Recharts
**Testing:** Playwright, MSW

## Performance

- Build time: ~5 seconds
- API response: <10ms
- GA optimization: <5s (30 gen, pop=20)
- Simulation: 1000 vehicles @ 10+ updates/sec

## Known Limitations

1. No pedestrian interactions
2. Lane selection logic needs refinement at intersections
3. Road polylines are straight (no curves)

## Next Steps (Production System Plan)

See [docs/PRODUCTION_PLAN.md](docs/PRODUCTION_PLAN.md) for full 8-stage plan.

| Stage | Description | Status |
|-------|-------------|--------|
| 1 | Density-based visualization | **Complete** |
| 2 | Runtime traffic light modification | **Complete** |
| 3 | Dynamic vehicle spawning | **Complete** |
| 4 | Continuous simulation mode | Pending |
| 5 | Traffic profiles (JSON-based) | Pending |
| 6 | Gradual timing blending | Pending |
| 7 | Continuous background optimization | Pending |
| 8 | Travel time metrics & dashboard | Pending |

## Predictive Optimization Plan

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Traffic Pattern Storage | **Complete** |
| 2 | Prediction Service | **Complete** |
| 3 | Predictive Optimizer | **Complete** |
| 4 | Validation & Rollout | **Complete** |
| 5 | Statistics Dashboard | Pending |
| 6 | Dashboard Enhancements | Pending |

## Logging

Component-based logging with spdlog:
- `LogComponent::API`, `Database`, `Simulation`, `Optimization`, `Core`, `General`
- `REQUEST_SCOPE()` for HTTP request tracing
- `TIMED_SCOPE()` for performance timing
- Output: console + rotating file + JSON

---

**Status:** Production-ready with 157 E2E tests. Implementing predictive optimization plan (Phases 1-4 complete).
