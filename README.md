# RATMS - Real-time Adaptive Traffic Management System

Production-ready traffic simulation with IDM physics, genetic algorithm optimization, and web-based monitoring.

## Features

- **IDM Physics**: Realistic vehicle acceleration, following, and lane changing
- **Traffic Light Control**: Per-lane signals with GA-optimized timing
- **Predictive Optimization**: Traffic prediction with auto-rollback validation
- **Real-time Dashboard**: React/TypeScript with live vehicle streaming (SSE)
- **Travel Time Tracking**: O-D pair metrics with percentile statistics
- **Traffic Profiles**: JSON-based profiles for spawn rates and light timings

## Quick Start

### Prerequisites

- C++20 compiler (g++ 10+), CMake 3.15+, SQLite3
- Node.js 18+, npm

### Build and Run

```bash
# Backend
cd simulator && mkdir -p build && cd build
cmake .. && make api_server
./api_server  # http://localhost:8080

# Frontend (new terminal)
cd frontend && npm install && npm run dev  # http://localhost:5173
```

### Using Real City Maps

```bash
# Run with included Munich map
./api_server --network ../../data/maps/munich_schwabing.json

# Import your own area (bbox: min_lon,min_lat,max_lon,max_lat)
./scripts/download_osm.sh berlin 13.36,52.50,13.42,52.54
./api_server --network ../../data/maps/berlin.json
```

## Architecture

```
ratms/
├── simulator/src/
│   ├── core/           # Simulator, Road, Vehicle, TrafficLight
│   ├── api/            # REST server, controllers
│   ├── optimization/   # Genetic Algorithm
│   ├── prediction/     # Traffic predictor
│   ├── validation/     # Timing validator
│   ├── metrics/        # Travel time collector
│   └── data/storage/   # SQLite persistence
├── frontend/src/
│   ├── pages/          # Dashboard, MapView, Optimization, Analytics
│   ├── components/     # Control panels, charts
│   └── services/       # API client
└── database/migrations/
```

## API Summary

| Category | Endpoints |
|----------|-----------|
| Simulation | `/api/simulation/*` - start, stop, status, stream (SSE), pause, resume |
| Optimization | `/api/optimization/*` - GA runs, continuous optimization, rollback |
| Traffic Control | `/api/traffic-lights`, `/api/spawn-rates` |
| Prediction | `/api/prediction/*` - forecast, config |
| Profiles | `/api/profiles/*` - CRUD, apply, capture, import/export |
| Travel Time | `/api/travel-time/*` - O-D pairs, stats, samples |

See [PROJECT_STATUS.md](PROJECT_STATUS.md) for complete endpoint list.

## Testing

```bash
# Backend unit tests
cd simulator/build && make unit_tests && ./unit_tests

# Frontend E2E tests (157 tests)
cd frontend && npm run test:e2e
```

## Performance

- API Response: <10ms
- GA Optimization: ~5s (30 gen, pop=20)
- Live Streaming: ~2 updates/sec via SSE
- Supports 1000+ vehicles

## Known Limitations

1. Road polylines are straight (no curves)
2. Lane selection at intersections needs refinement
3. No pedestrian simulation

## Documentation

- [PROJECT_STATUS.md](PROJECT_STATUS.md) - Feature status and API reference
- [CLAUDE.md](CLAUDE.md) - Development guidelines
- [docs/PRODUCTION_PLAN.md](docs/PRODUCTION_PLAN.md) - Original production plan

## Tech Stack

**Backend**: C++20, SQLite3, cpp-httplib, nlohmann/json, spdlog
**Frontend**: React 18, TypeScript, Vite, Tailwind CSS, Leaflet, Recharts

## License

See LICENSE file for details.
