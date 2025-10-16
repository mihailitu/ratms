# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

RATMS (Real-time Adaptive Traffic Management System) is a production-ready traffic simulation system built in C++20 with a React/TypeScript web dashboard. It implements the Intelligent Driver Model (IDM) for realistic vehicle behavior, genetic algorithm optimization for traffic light timing, and real-time visualization with live vehicle streaming.

## Architecture

### Modular Monolith (Approach #1)

The project uses a modular monolith architecture with clear separation of concerns:

```
simulator/src/
├── core/              # Simulation engine
│   ├── simulator.h/cpp       # Main simulation loop
│   ├── road.h/cpp           # Road segments with lanes
│   ├── vehicle.h/cpp        # Vehicle entities with IDM
│   ├── trafficlight.h/cpp   # Traffic signal control
│   └── defs.h               # Type definitions
├── utils/             # Utilities
│   ├── config.h/cpp         # Configuration management
│   ├── logger.h/cpp         # Logging system
│   └── utils.h/cpp          # Helper functions
├── mapping/           # Map loading
│   ├── newmap.hpp           # JSON map loader
│   └── newroad.hpp          # Road definitions
├── optimization/      # Genetic Algorithm
│   ├── genetic_algorithm.h/cpp  # GA engine
│   └── metrics.h/cpp            # Fitness evaluation
├── api/               # REST API Server
│   ├── server.h/cpp             # HTTP/SSE server
│   └── optimization_controller.h/cpp  # GA endpoints
├── data/storage/      # Database Layer
│   └── database.h/cpp       # SQLite manager
└── tests/             # Test Scenarios
    └── testmap.cpp          # Pre-built test networks
```

## Build System

The project uses CMake with multiple executable targets:

### CMake Build (Primary)
```bash
cd simulator
mkdir -p build
cd build
cmake ..
make api_server    # API server (primary executable)
make ga_optimizer  # GA optimization tool
```

### Executables
- **api_server**: Main HTTP server with simulation engine
- **ga_optimizer**: Standalone GA optimization tool for batch processing

### Build Configuration
- C++20 standard required
- Links: pthread, sqlite3
- Header-only libraries: cpp-httplib, nlohmann/json
- Build output: `simulator/build/`

## Core Components

### 1. Simulator (`core/simulator.h/cpp`)

Main simulation engine managing the complete traffic system.

**Key Members:**
- `CityMap cityMap`: Map of roadID → Road objects
- Thread management: simulation runs in background thread
- Two-phase update cycle:
  1. Update all roads (IDM physics, lane changes)
  2. Execute road transitions (vehicles moving between roads)

**Important Methods:**
- `run()`: Main simulation loop (called by API server in background thread)
- `step()`: Single time-step update
- `cityMap`: Direct access to road network

**Usage Pattern:**
```cpp
simulator::Simulator sim;
std::vector<simulator::Road> network = simulator::manyRandomVehicleTestMap(10);
for (auto& road : network) {
    sim.cityMap[road.getId()] = std::move(road);
}
// Update in loop
for (int i = 0; i < steps; i++) {
    std::vector<simulator::RoadTransition> transitions;
    for (auto& [id, road] : sim.cityMap) {
        road.update(dt, sim.cityMap, transitions);
    }
    // Execute transitions...
}
```

### 2. Road (`core/road.h/cpp`)

Represents one-way road segments with multiple lanes.

**Key Concepts:**
- Roads are unidirectional (bidirectional roads = 2 Road objects)
- Right-to-left lane numbering: lane 0 is rightmost/slowest
- Each lane has independent traffic light and destination connections
- Vehicles stored as `std::list<Vehicle>` per lane
- Geographic coordinates: `roadPosGeo` (lon, lat) and Cartesian (x, y)

**Important Members:**
- `std::vector<std::list<Vehicle>> lanesVehicles`: Vehicles per lane
- `std::vector<TrafficLight> trafficLights`: One per lane
- `std::map<int, std::map<unsigned, double>> connections`: roadID → lane → probability
- Geographic: `startPosGeo`, `endPosGeo` (lat/lon pairs)
- Cartesian: `startPosCartesian`, `endPosCartesian` (meters)

**Key Methods:**
- `update(dt, cityMap, transitions)`: Main update with two-phase pattern
- `getStartPosGeo()`, `getEndPosGeo()`: Geographic coordinate getters
- `getLanesNo()`: Number of lanes
- `getId()`, `getLength()`, `getMaxSpeed()`: Basic properties

**Lane Change Logic:**
- Implemented in `update()` method
- Safety checks: gap with new leader and follower
- Acceleration incentive: only change if beneficial
- Distance thresholds: `maxChangeLaneDist`, `minChangeLaneDist`

### 3. Vehicle (`core/vehicle.h/cpp`)

Individual vehicle entities implementing IDM physics.

**IDM Parameters:**
- `v0`: Desired velocity (affected by road max speed and aggressivity)
- `T`: Safe time headway (typically 1.5s)
- `a`: Maximum acceleration (typically 1.5 m/s²)
- `b`: Desired deceleration (typically 2.0 m/s²)
- `s0`: Minimum safe distance (typically 2.0m)
- `delta`: Acceleration exponent (typically 4)
- `aggressivity`: Driver behavior modifier (0.5 = normal, <0.5 = cautious, >0.5 = aggressive)

**Key Methods:**
- `getNewAcceleration(leader, maxSpeed)`: IDM calculation
- `update(dt, maxSpeed)`: Position and velocity update
- `getId()`: Unique identifier (auto-generated)

**IDM Implementation:**
Free-road acceleration when no leader:
```cpp
a_free = a * (1 - pow(v/v0, delta))
```

Following acceleration with leader:
```cpp
s_star = s0 + v*T + (v*dv)/(2*sqrt(a*b))
a_follow = a * (1 - pow(v/v0, delta) - pow(s_star/s, 2))
```

### 4. TrafficLight (`core/trafficlight.h/cpp`)

Traffic signal management with state machine.

**States:**
- 'R': Red (stop)
- 'Y': Yellow (prepare to stop)
- 'G': Green (go)

**Key Methods:**
- `update(dt)`: State transition logic
- `getState()`: Current state
- `setTimings(greenTime, yellowTime, redTime)`: Configure cycle

**Usage:**
One traffic light per lane per road. Vehicles check traffic light state before entering intersection.

### 5. GeneticAlgorithm (`optimization/genetic_algorithm.h/cpp`)

Traffic light timing optimization using genetic algorithms.

**Algorithm Components:**
- **Chromosome**: Vector of (greenTime, redTime) pairs for each traffic light
- **Selection**: Tournament selection (size=3)
- **Crossover**: Uniform crossover (80% rate)
- **Mutation**: Gaussian mutation with bounds clamping (15% rate, 5s stddev)
- **Elitism**: Top 10% preserved unchanged
- **Replacement**: Generational (entire population replaced each generation)

**Key Classes:**
```cpp
struct TrafficLightGene {
    double greenTime;
    double redTime;
};

struct Chromosome {
    std::vector<TrafficLightGene> genes;
    double fitness;
};

class GeneticAlgorithm {
    void initializePopulation(size_t numTrafficLights);
    Chromosome evolve();  // Main evolution loop
    std::vector<double> getFitnessHistory();
};
```

**Parameters Struct:**
```cpp
struct Parameters {
    size_t populationSize;
    size_t generations;
    double mutationRate;
    double mutationStdDev;
    double crossoverRate;
    size_t tournamentSize;
    double elitismRate;
    double minGreenTime;
    double maxGreenTime;
    double minRedTime;
    double maxRedTime;
};
```

### 6. FitnessEvaluator (`optimization/metrics.h/cpp`)

Simulation-based fitness function for GA optimization.

**Metrics Collected:**
- Average queue length at traffic lights
- Average vehicle speed
- Vehicles exited (completed routes)
- Maximum queue length observed

**Fitness Function:**
```cpp
fitness = avgQueueLength + (1.0 / (avgSpeed + 0.1)) - (vehiclesExited * 0.1)
```

Lower fitness is better. Minimizes queue length and wait time, maximizes throughput.

**Fallback Fitness:**
For short simulations where no vehicles exit, uses queue length + active vehicle penalty to prevent invalid fitness values.

### 7. Server (`api/server.h/cpp`)

HTTP/SSE server using cpp-httplib.

**Architecture:**
- Single-threaded HTTP server with chunked content for SSE
- Background simulation thread with atomic synchronization
- Mutex-protected simulator access
- Separate snapshot for streaming data

**Key Members:**
```cpp
class Server {
    httplib::Server http_server_;
    std::unique_ptr<simulator::Simulator> simulator_;
    std::mutex sim_mutex_;

    // Simulation thread
    std::unique_ptr<std::thread> simulation_thread_;
    std::atomic<bool> simulation_should_stop_{false};
    std::atomic<int> simulation_steps_{0};
    std::atomic<double> simulation_time_{0.0};

    // Streaming
    SimulationSnapshot latest_snapshot_;
    std::mutex snapshot_mutex_;
    std::atomic<bool> has_new_snapshot_{false};
};
```

**Thread Safety Pattern:**
```cpp
void Server::handleGetStatus(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(sim_mutex_);
    // Access simulator_ safely
}
```

**SSE Streaming Pattern:**
```cpp
res.set_chunked_content_provider(
    "text/event-stream",
    [this](size_t offset, httplib::DataSink &sink) {
        if (has_new_snapshot_) {
            SimulationSnapshot snapshot;
            {
                std::lock_guard<std::mutex> lock(snapshot_mutex_);
                snapshot = latest_snapshot_;
                has_new_snapshot_ = false;
            }
            json data = convertSnapshotToJson(snapshot);
            std::string msg = "event: update\ndata: " + data.dump() + "\n\n";
            return sink.write(msg.c_str(), msg.size());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return true;
    }
);
```

**Snapshot Capture:**
Called every 5 simulation steps (~2 updates/sec):
```cpp
void Server::captureSimulationSnapshot() {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    latest_snapshot_.step = simulation_steps_;
    latest_snapshot_.time = simulation_time_;
    latest_snapshot_.vehicles.clear();
    latest_snapshot_.trafficLights.clear();

    for (const auto& [roadId, road] : simulator_->cityMap) {
        // Capture vehicle positions
        for (unsigned lane = 0; lane < road.getLanesNo(); lane++) {
            for (const auto& vehicle : road.lanesVehicles[lane]) {
                latest_snapshot_.vehicles.push_back({...});
            }
        }
        // Capture traffic light states
        for (unsigned lane = 0; lane < road.getLanesNo(); lane++) {
            latest_snapshot_.trafficLights.push_back({
                roadId, lane, road.trafficLights[lane].getState()
            });
        }
    }

    has_new_snapshot_ = true;
}
```

### 8. DatabaseManager (`data/storage/database.h/cpp`)

SQLite persistence layer with prepared statements.

**Schema (see database/migrations/):**
- `simulations`: Simulation metadata and lifecycle
- `networks`: Road network configurations
- `metrics`: Time-series simulation metrics
- `roads`: Individual road data
- `traffic_lights`: Traffic light configurations
- `optimization_runs`: GA optimization results
- `optimization_generations`: Fitness per generation
- `optimization_solutions`: Chromosome data

**Key Methods:**
```cpp
int64_t createSimulation(name, description, networkId, config);
void updateSimulationStatus(id, status);
void insertMetrics(simulationId, timestamp, metrics);
std::vector<MetricRecord> getMetrics(simulationId, startTime, endTime);
```

**Prepared Statement Pattern:**
All queries use prepared statements to prevent SQL injection.

## Frontend Architecture

### React + TypeScript Structure

```
frontend/src/
├── pages/
│   ├── Dashboard.tsx        # Main overview page
│   ├── Simulations.tsx      # Simulation history list
│   ├── SimulationDetail.tsx # Single simulation analysis
│   ├── MapView.tsx          # Live vehicle map
│   └── Optimization.tsx     # GA configuration and results
├── components/
│   ├── Layout.tsx           # Navigation wrapper
│   ├── SimulationControl.tsx # Start/stop controls
│   └── GAVisualizer.tsx     # Fitness charts
├── services/
│   └── apiClient.ts         # Type-safe API client
├── hooks/
│   └── useSimulationStream.ts # SSE hook
└── types/
    └── api.ts               # TypeScript interfaces
```

### API Client Pattern

All API calls go through type-safe client:

```typescript
class ApiClient {
  private client: AxiosInstance;

  async getSimulationStatus(): Promise<SimulationStatus> {
    const response = await this.client.get<SimulationStatus>('/api/simulation/status');
    return response.data;
  }
}

export const apiClient = new ApiClient();
```

### SSE Hook Pattern

Custom React hook for Server-Sent Events:

```typescript
export function useSimulationStream(enabled: boolean = true) {
  const [latestUpdate, setLatestUpdate] = useState<SimulationUpdate | null>(null);
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    if (!enabled) return;

    const eventSource = new EventSource('http://localhost:8080/api/simulation/stream');

    eventSource.addEventListener('update', (event) => {
      const data: SimulationUpdate = JSON.parse(event.data);
      setLatestUpdate(data);
    });

    eventSource.onopen = () => setIsConnected(true);
    eventSource.onerror = () => setIsConnected(false);

    return () => eventSource.close();
  }, [enabled]);

  return { latestUpdate, isConnected, error };
}
```

### Map Visualization Pattern

Vehicle position interpolation along roads:

```typescript
// Find road for vehicle
const road = roads.find(r => r.id === vehicle.roadId);

// Interpolate position
const ratio = Math.min(vehicle.position / road.length, 1.0);
const lat = road.startLat + (road.endLat - road.startLat) * ratio;
const lng = road.startLon + (road.endLon - road.startLon) * ratio;

// Lane offset (perpendicular to road)
if (vehicle.lane > 0) {
  const perpOffset = 0.00001 * vehicle.lane;
  lat += perpOffset;
}

// Speed-based coloring
const speedRatio = Math.min(vehicle.velocity / 20, 1.0);
const color = speedRatio > 0.7 ? '#22c55e' : speedRatio > 0.3 ? '#3b82f6' : '#ef4444';
```

## Development Workflow

### Making Code Changes

1. **Backend (C++):**
   - Edit files in `simulator/src/`
   - Rebuild: `cd simulator/build && make api_server`
   - Restart server: `./api_server`

2. **Frontend (TypeScript):**
   - Edit files in `frontend/src/`
   - Vite hot-reload handles updates automatically
   - Type check: `npx tsc --noEmit`

### Adding New API Endpoints

1. **Backend** (`simulator/src/api/server.cpp`):
   ```cpp
   void Server::setupRoutes() {
       http_server_.Get("/api/new-endpoint",
           [this](const httplib::Request& req, httplib::Response& res) {
               handleNewEndpoint(req, res);
           });
   }

   void Server::handleNewEndpoint(const httplib::Request& req, httplib::Response& res) {
       std::lock_guard<std::mutex> lock(sim_mutex_);
       json response = {{"status", "ok"}};
       res.set_content(response.dump(2), "application/json");
   }
   ```

2. **Frontend Types** (`frontend/src/types/api.ts`):
   ```typescript
   export interface NewResponse {
     status: string;
     data: any;
   }
   ```

3. **Frontend Client** (`frontend/src/services/apiClient.ts`):
   ```typescript
   async getNewData(): Promise<NewResponse> {
     const response = await this.client.get<NewResponse>('/api/new-endpoint');
     return response.data;
   }
   ```

### Creating Test Maps

Add test scenarios to `simulator/src/tests/testmap.cpp`:

```cpp
std::vector<simulator::Road> customTestMap() {
    std::vector<simulator::Road> roads;

    // Create road with geographic coordinates
    simulator::Road road1(
        0,                          // id
        {11.5820, 48.1351},        // startGeo (lon, lat)
        {11.5830, 48.1361},        // endGeo
        500.0,                     // length (meters)
        2,                         // lanes
        15.0,                      // maxSpeed (m/s)
        true                       // hasTrafficLight
    );

    // Add vehicle
    simulator::Vehicle vehicle(
        0.0,      // position
        10.0,     // velocity
        0.5       // aggressivity
    );
    road1.lanesVehicles[0].push_back(vehicle);

    // Set connections (road 0, lane 0 → road 1 with 100% probability)
    road1.connections[1][0] = 1.0;

    roads.push_back(std::move(road1));
    return roads;
}
```

### Database Migrations

Create new migration in `database/migrations/`:

```sql
-- 003_new_feature.sql
CREATE TABLE IF NOT EXISTS new_table (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    simulation_id INTEGER NOT NULL,
    data TEXT,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (simulation_id) REFERENCES simulations(id) ON DELETE CASCADE
);

CREATE INDEX idx_new_table_simulation ON new_table(simulation_id);
```

Migrations run automatically on server startup in alphanumeric order.

## Key Concepts

### Intelligent Driver Model (IDM)

Two-component acceleration model:

1. **Free-road acceleration** (no leader):
   - Accelerate toward desired velocity v0
   - Smooth approach using power function

2. **Following behavior** (with leader):
   - Maintain safe distance based on current speed
   - Consider relative velocity (approaching/receding)
   - Smooth deceleration when too close

Reference: https://en.wikipedia.org/wiki/Intelligent_driver_model

### Two-Phase Simulation Update

**Phase 1**: Update all roads
- IDM physics calculations
- Lane change decisions
- Traffic light updates
- Collect road transitions (vehicles exiting roads)

**Phase 2**: Execute transitions
- Move vehicles between roads
- Ensures no vehicle is updated twice
- Prevents iterator invalidation

This pattern is critical for correctness.

### Thread Safety

**Rule**: All simulator access must be mutex-protected

```cpp
// Reading simulator state
{
    std::lock_guard<std::mutex> lock(sim_mutex_);
    auto roadCount = simulator_->cityMap.size();
}

// Writing simulator state
{
    std::lock_guard<std::mutex> lock(sim_mutex_);
    simulator_->cityMap[roadId].lanesVehicles[lane].push_back(vehicle);
}
```

**Exception**: Streaming snapshot uses separate mutex to minimize contention.

### Geographic Coordinates

Roads have both coordinate systems:

- **Geographic**: `roadPosGeo` = `std::pair<double, double>` (lon, lat)
- **Cartesian**: `roadPosCartesian` = `std::pair<double, double>` (x, y in meters)

For map visualization, use geographic coordinates (lat/lon).

## Common Patterns

### Error Handling

**Backend (C++):**
```cpp
try {
    // Operation
} catch (const std::exception& e) {
    json error = {{"error", e.what()}, {"status", "error"}};
    res.status = 500;
    res.set_content(error.dump(2), "application/json");
}
```

**Frontend (TypeScript):**
```typescript
try {
  const data = await apiClient.getSimulationStatus();
  setStatus(data);
} catch (err) {
  setError(err instanceof Error ? err.message : 'Unknown error');
}
```

### Background Thread Management

```cpp
// Start background task
run->optimizationThread = std::make_unique<std::thread>(
    &OptimizationController::runOptimizationBackground, this, run, networkId);

// Stop and wait
if (run->optimizationThread && run->optimizationThread->joinable()) {
    run->isRunning = false;
    run->optimizationThread->join();
}
```

### JSON Serialization

**Backend:**
```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

json data = {
    {"id", vehicle.getId()},
    {"position", vehicle.getPosition()},
    {"velocity", vehicle.getVelocity()}
};
std::string jsonStr = data.dump(2);  // Indent with 2 spaces
```

**Frontend:**
```typescript
interface VehicleData {
  id: number;
  position: number;
  velocity: number;
}

const data: VehicleData = JSON.parse(jsonStr);
```

## Testing

### Backend Testing

Run api_server with test network:
```bash
cd simulator/build
./api_server
# Uses manyRandomVehicleTestMap(10) by default
```

Test API endpoints:
```bash
curl http://localhost:8080/api/health
curl http://localhost:8080/api/simulation/status
curl -X POST http://localhost:8080/api/simulation/start
```

### Frontend Testing

Type checking:
```bash
cd frontend
npx tsc --noEmit
```

Build check:
```bash
cd frontend
npm run build
```

## Performance Considerations

1. **Simulation Step**: 0.1s time step (dt=0.1)
2. **Metrics Collection**: Every 10 steps
3. **Database Writes**: Every 100 steps (batch insertion)
4. **Snapshot Capture**: Every 5 steps (~2 updates/sec)
5. **SSE Polling**: 50ms sleep between checks

### Optimization Tips

- Keep simulation loop fast (avoid I/O in hot path)
- Use mutex only when necessary
- Batch database writes
- Use move semantics for large objects
- Reserve vector capacity when size is known

## Known Issues and Limitations

1. **GA Persistence**: Optimization results stored in-memory only
2. **Road Geometry**: Polylines are straight (no curves)
3. **Traffic Lights on Map**: Not visualized yet
4. **Lane Selection**: Logic needs refinement at intersections
5. **Pedestrians**: Not implemented

## File Locations

**C++ Backend:**
- Core: `simulator/src/core/`
- API: `simulator/src/api/`
- GA: `simulator/src/optimization/`
- Database: `simulator/src/data/storage/`
- Tests: `simulator/src/tests/`

**Frontend:**
- Pages: `frontend/src/pages/`
- Components: `frontend/src/components/`
- Services: `frontend/src/services/`
- Types: `frontend/src/types/`

**Configuration:**
- CMake: `simulator/CMakeLists.txt`
- Database schema: `database/migrations/*.sql`
- Frontend build: `frontend/vite.config.ts`

**Documentation:**
- User docs: `README.md`
- Project status: `PROJECT_STATUS.md`
- Architecture: `ARCHITECTURE.md`
- AI guidance: `CLAUDE.md` (this file)

## Dependencies

**C++ (Backend):**
- C++20 compiler (g++ 10+)
- SQLite3 (system library)
- cpp-httplib (header-only, in `simulator/src/external/`)
- nlohmann/json (header-only, in `simulator/src/external/`)

**JavaScript (Frontend):**
- Node.js 18+
- React 18
- TypeScript 5+
- Vite (dev server)
- Tailwind CSS
- Axios (HTTP client)
- Leaflet (maps)
- Recharts (charts)

## Quick Reference

### Build Commands
```bash
# Backend
cd simulator/build
cmake .. && make api_server
./api_server

# Frontend
cd frontend
npm install
npm run dev

# GA Optimizer
cd simulator/build
make ga_optimizer
./ga_optimizer --pop 50 --gen 100
```

### Important Files to Check
- Server routes: `simulator/src/api/server.cpp:setupRoutes()`
- Simulation loop: `simulator/src/api/server.cpp:runSimulationLoop()`
- IDM physics: `simulator/src/core/vehicle.cpp:getNewAcceleration()`
- API client: `frontend/src/services/apiClient.ts`
- Type definitions: `frontend/src/types/api.ts`

### Debugging Tips
- Server logs to stdout (no file logging yet)
- Frontend logs to browser console (F12)
- Database: `database/ratms.db` (use sqlite3 CLI tool)
- Check `PROJECT_STATUS.md` for known issues
- Thread issues: Add more logging with mutex-protected std::cout

## Notes for AI Assistants

- Always use mutex when accessing `simulator_` in Server class
- Follow two-phase update pattern for simulation
- Use prepared statements for all database queries
- Maintain type safety between C++ and TypeScript
- Test both backend and frontend after changes
- Update PROJECT_STATUS.md for significant changes
- Follow existing code patterns and naming conventions
- Consider thread safety for all shared state
- Use move semantics for performance-critical code
- Batch database operations when possible
