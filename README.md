# RATMS - Real-time Adaptive Traffic Management System

A production-ready traffic simulation system implementing the Intelligent Driver Model (IDM) with web-based monitoring, control, and genetic algorithm optimization.

## Features

### Core Simulation
- **Intelligent Driver Model (IDM)**: Realistic vehicle acceleration and following behavior
- **Multi-lane Roads**: Lane changing with safety checks and MOBIL model
- **Traffic Light Control**: Independent per-lane traffic signal management
- **Road Networks**: Complex intersection and road network support
- **Geographic Coordinates**: Real-world map integration with lat/lon positioning

### Web Dashboard
- **Real-time Monitoring**: Live simulation status and metrics
- **Interactive Map**: Leaflet-based visualization with vehicle tracking
- **Live Streaming**: Server-Sent Events for real-time vehicle positions
- **Simulation History**: Browse and analyze past simulation runs
- **Network Management**: View and manage road network configurations

### Optimization
- **Genetic Algorithm**: Traffic light timing optimization
- **Web Interface**: Configure and monitor GA optimization runs
- **Real-time Progress**: Live fitness evolution tracking
- **Results Visualization**: Charts and statistics for optimization results
- **Standalone Tool**: Command-line GA optimizer for batch processing

### Technical Stack
- **Backend**: C++20, SQLite3, cpp-httplib
- **Frontend**: React 18, TypeScript, Vite, Tailwind CSS
- **Visualization**: Leaflet (maps), Recharts (charts)
- **Data**: REST API, Server-Sent Events (SSE)

## Quick Start

### Prerequisites

**C++ Backend:**
- C++20 compiler (g++ 10+)
- CMake 3.15+
- SQLite3
- Make
- libexpat-dev (for OSM import)

**Frontend:**
- Node.js 18+
- npm

### Build and Run

#### 1. Build Backend
```bash
cd simulator
mkdir -p build
cd build
cmake ..
make api_server
```

#### 2. Start API Server
```bash
cd simulator/build
./api_server
# Server runs on http://localhost:8080
```

#### 3. Start Frontend (in new terminal)
```bash
cd frontend
npm install
npm run dev
# Dashboard runs on http://localhost:5173
```

#### 4. Access Dashboard
Open browser to [http://localhost:5173](http://localhost:5173)

### Using Real City Maps (OpenStreetMap)

RATMS supports importing real road networks from OpenStreetMap. A pre-imported Munich Schwabing map is included.

#### Run with Munich Map
```bash
cd simulator/build
./api_server --network ../../data/maps/munich_schwabing.json
```

#### Import Your Own City
```bash
# Download and convert any area
./scripts/download_osm.sh berlin 13.36,52.50,13.42,52.54

# Run with the new map
cd simulator/build
./api_server --network ../../data/maps/berlin.json
```

See [OSM Import Guide](#openstreetmap-import) for details.

## Usage

### Web Dashboard

1. **Dashboard Page** (`/`): Overview with status, metrics, and network list
2. **Simulations Page** (`/simulations`): History of simulation runs
3. **Map View** (`/map`): Live vehicle tracking on interactive map
4. **Optimization** (`/optimization`): Configure and run GA optimization

### Starting a Simulation

1. Navigate to Dashboard
2. Click "Start Simulation" button
3. Simulation runs in background with real-time metrics
4. View live vehicles on Map View page
5. Click "Stop Simulation" when complete

### Running GA Optimization

**Via Web Interface:**
1. Navigate to Optimization page (`/optimization`)
2. Configure GA parameters (population size, generations, mutation rate, etc.)
3. Click "Start Optimization"
4. Monitor real-time progress and fitness evolution
5. View results and traffic light timings

**Via Command Line:**
```bash
cd simulator/build
./ga_optimizer --pop 50 --gen 100 --steps 2000
# Results exported to evolution_history.csv and best_solution.csv
```

### API Endpoints

#### Simulation Control
- `GET /api/health` - Health check
- `GET /api/simulation/status` - Current status
- `POST /api/simulation/start` - Start simulation
- `POST /api/simulation/stop` - Stop simulation
- `GET /api/simulation/stream` - SSE live updates
- `GET /api/simulation/roads` - Road network geometry

#### Data Management
- `GET /api/simulations` - List all simulations
- `GET /api/simulations/:id` - Simulation details
- `GET /api/simulations/:id/metrics` - Simulation metrics
- `GET /api/networks` - List networks

#### Optimization
- `POST /api/optimization/start` - Start GA optimization
- `GET /api/optimization/status/:id` - Progress status
- `GET /api/optimization/results/:id` - Full results
- `GET /api/optimization/history` - All optimization runs
- `POST /api/optimization/stop/:id` - Stop optimization

## Architecture

### Modular Monolith Structure

```
ratms/
├── simulator/              # C++ Backend
│   ├── src/
│   │   ├── core/          # Simulator, Road, Vehicle, TrafficLight
│   │   ├── utils/         # Logger, Config, Utils
│   │   ├── mapping/       # Map loading, OSM import, network loader
│   │   ├── optimization/  # Genetic Algorithm engine
│   │   ├── api/           # REST API server
│   │   ├── data/storage/  # Database layer
│   │   ├── tools/         # Standalone tools (osm_import_main.cpp)
│   │   └── tests/         # Test scenarios
│   └── build/
│       ├── api_server     # Main HTTP server
│       ├── ga_optimizer   # GA optimization tool
│       └── osm_import     # OSM to JSON converter
├── frontend/              # React Dashboard
│   ├── src/
│   │   ├── pages/        # Dashboard, Simulations, Map, Optimization
│   │   ├── components/   # Reusable UI components
│   │   ├── services/     # API client
│   │   └── types/        # TypeScript types
│   └── dist/             # Built assets
├── data/                  # Data files
│   ├── osm/              # Raw OSM downloads
│   └── maps/             # Converted JSON maps
├── scripts/               # Utility scripts
│   └── download_osm.sh   # OSM download helper
├── database/
│   └── migrations/       # SQL schema definitions
└── config/               # YAML configuration files
```

### Key Components

**C++ Backend:**
- **Simulator**: Main simulation engine with IDM physics
- **Road**: Multi-lane road segments with geographic coordinates
- **Vehicle**: Individual vehicles with configurable aggressivity
- **TrafficLight**: Per-lane traffic signal control
- **GeneticAlgorithm**: Optimization engine with tournament selection
- **FitnessEvaluator**: Simulation-based fitness function
- **Server**: HTTP/SSE server with background threads
- **DatabaseManager**: SQLite persistence layer

**Frontend:**
- **Dashboard**: Real-time status monitoring
- **MapView**: Interactive Leaflet map with live vehicle tracking
- **Optimization**: GA configuration and results visualization
- **ApiClient**: Type-safe REST API client

## Development

### Building from Source

```bash
# Backend
cd simulator/build
cmake .. && make

# Frontend
cd frontend
npm install
npm run build
```

### Running Tests

```bash
# Backend builds with test scenarios
cd simulator/build
./api_server  # Uses manyRandomVehicleTestMap by default

# Frontend type checking
cd frontend
npx tsc --noEmit
```

### Configuration

Edit `config/default.yaml` for simulation parameters:
- Time step (dt)
- Simulation duration
- Vehicle spawn rates
- Traffic light timings
- Network configurations

### Database Migrations

Migrations run automatically on server startup. Manual migration:
```bash
cd simulator/build
./api_server  # Applies all pending migrations
```

## Performance

- **Simulation**: 10,000 steps @ 0.1s dt = 1000s simulated time
- **API Response**: <10ms (local)
- **Build Time**: ~5-6 seconds (C++), <1 second (TypeScript)
- **GA Optimization**: ~5 seconds (30 generations, population=20)
- **Live Streaming**: ~2 updates/second via SSE

## Project Status

✅ **Phase 1-9 Complete** (see PROJECT_STATUS.md for details)

Current capabilities:
- Real simulation execution with background threads
- Live vehicle streaming via Server-Sent Events
- Interactive map with road network visualization
- Genetic algorithm traffic light optimization
- Database persistence for simulations and metrics
- Web dashboard with real-time monitoring

## Known Limitations

1. GA optimization results stored in-memory only (database persistence TODO)
2. No pedestrian interactions
3. Road polylines are straight (curved segments possible)
4. Traffic light indicators not shown on map
5. Lane selection logic needs refinement

## Future Enhancements

- Database persistence for GA optimization results
- Traffic light indicators on map
- Advanced network editing interface
- Performance analytics dashboard
- Curved road rendering
- Multi-network comparison tools

## OpenStreetMap Import

RATMS can import real-world road networks from OpenStreetMap (OSM).

### How It Works

1. **Download**: OSM XML data is fetched from the Overpass API
2. **Parse**: The `osm_import` tool extracts roads and intersections
3. **Convert**: Road segments are converted to simulator-compatible JSON
4. **Load**: The `api_server` loads the JSON map at startup

### Supported Road Types

The importer handles these OSM highway types:
- motorway, trunk, primary, secondary, tertiary
- residential, living_street, unclassified
- motorway_link, trunk_link, primary_link, etc.

### Speed and Lane Defaults

When OSM data lacks specific tags, the importer uses sensible defaults:

| Road Type   | Default Speed | Default Lanes |
|-------------|---------------|---------------|
| motorway    | 120 km/h      | 3             |
| trunk       | 100 km/h      | 2             |
| primary     | 50 km/h       | 2             |
| secondary   | 50 km/h       | 2             |
| tertiary    | 40 km/h       | 1             |
| residential | 30 km/h       | 1             |

### Manual Import

```bash
# Build the import tool
cd simulator/build
cmake .. && make osm_import

# Download OSM data manually
wget -O data/osm/myarea.osm "https://overpass-api.de/api/map?bbox=LON1,LAT1,LON2,LAT2"

# Convert to JSON
./osm_import data/osm/myarea.osm data/maps/myarea.json "My Area"

# Verify the output
cat data/maps/myarea.json | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'Roads: {len(d[\"roads\"])}')"
```

### Bounding Box Format

The bbox format is: `min_lon,min_lat,max_lon,max_lat`

Example areas:
- Munich Schwabing: `11.56,48.15,11.60,48.17`
- Berlin Mitte: `13.36,52.50,13.42,52.54`
- Paris center: `2.32,48.85,2.36,48.87`

### Area Size Recommendations

| Area Size | Approximate Roads | File Size | Performance |
|-----------|-------------------|-----------|-------------|
| 2x2 km    | 500-1000          | 1-3 MB    | Excellent   |
| 5x5 km    | 3000-8000         | 5-20 MB   | Good        |
| 10x10 km  | 10000+            | 50+ MB    | May lag     |

Start small and increase area as needed.

## Documentation

- **PROJECT_STATUS.md**: Detailed phase-by-phase project status
- **ARCHITECTURE.md**: Architecture decision records
- **CLAUDE.md**: AI assistant development guidelines
- **database/migrations/*.sql**: Database schema documentation

## Credits

Built with C++20, React, TypeScript, and modern web technologies.

Implements the Intelligent Driver Model (IDM) for realistic vehicle behavior:
- [IDM Wikipedia](https://en.wikipedia.org/wiki/Intelligent_driver_model)

## License

See LICENSE file for details.
