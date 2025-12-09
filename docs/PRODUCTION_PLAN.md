# Plan: RATMS Production Traffic Management System

## Goal
Transform RATMS from a simulation demo into a production traffic management system with continuous operation, real-time optimization, traffic data ingestion, and comprehensive monitoring.

## User Requirements (Confirmed)
- **Traffic Input**: Flow rates per road (vehicles/minute)
- **Time Profiles**: 2-3 profiles (morning rush, evening rush, off-peak)
- **Apply Strategy**: Gradual blend (transition old to new over 5 minutes)
- **Congestion Colors**: Density-based (vehicles/km)

---

## Architecture Overview

```
[Real Traffic Data] --> [Traffic Data Controller] --> [Vehicle Spawning]
                                |
                                v
[Time Profiles DB] --> [Profile Scheduler] --> [Active Profile]
                                |
                                v
[Continuous GA] --> [Gradual Timing Blender] --> [Live Traffic Lights]
                                |
                                v
[Sample Vehicles] --> [Travel Time Collector] --> [Analytics Dashboard]
```

---

## Phase 1: Core Infrastructure (Foundation)

### 1.1 Traffic Light Runtime Modification

**Files:**
- `simulator/src/core/trafficlight.h` - Add methods:
  ```cpp
  void setTimings(double greenTime, double yellowTime, double redTime);
  double getGreenTime() const;
  double getRedTime() const;
  ```
- `simulator/src/core/trafficlight.cpp` - Implement setters
- `simulator/src/core/road.h` - Add:
  ```cpp
  TrafficLight& getTrafficLight(unsigned lane);
  void setTrafficLightTimings(unsigned lane, double green, double yellow, double red);
  ```
- `simulator/src/core/road.cpp` - Implement

### 1.2 Dynamic Vehicle Spawning

**Files:**
- `simulator/src/core/road.h` - Add:
  ```cpp
  bool spawnVehicle(double position, double velocity, double aggressivity);
  int getVehicleCount() const;
  ```
- `simulator/src/core/road.cpp` - Implement (choose lane with fewest vehicles)

---

## Phase 2: Continuous Operation

### 2.1 Server Continuous Mode

**Files:**
- `simulator/src/api/server.h` - Add:
  ```cpp
  std::atomic<bool> continuous_mode_{false};
  std::atomic<int> restart_count_{0};
  void enableContinuousMode(bool enable);
  void reinitializeSimulator();
  ```
- `simulator/src/api/server.cpp` - Modify `runSimulationLoop()`:
  - Remove step limit in continuous mode
  - Add try/catch with auto-restart
  - Add `/api/simulation/continuous` endpoint

### 2.2 New API Endpoints
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/simulation/continuous` | POST | Enable/disable continuous mode |
| `/api/system/health` | GET | System health with restart count |

---

## Phase 3: Traffic Data Ingestion

### 3.1 Traffic Data Controller

**New Files:**
- `simulator/src/api/traffic_data_controller.h`
- `simulator/src/api/traffic_data_controller.cpp`

**Key Structures:**
```cpp
struct TrafficFlowRate {
    roadID roadId;
    double vehiclesPerMinute;
    double timestamp;
};

struct TrafficProfile {
    std::string name;  // "morning_rush", "evening_rush", "off_peak"
    int startHour, endHour;
    std::map<roadID, double> flowRates;
};
```

**Method:** `processVehicleSpawning(dt)` - Called every simulation step:
- Accumulate partial vehicles per road
- Spawn when accumulator >= 1.0

### 3.2 Database Schema

**New File:** `database/migrations/003_traffic_profiles.sql`
```sql
CREATE TABLE traffic_profiles (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE,
    start_hour INTEGER,
    end_hour INTEGER,
    is_active BOOLEAN DEFAULT 0
);

CREATE TABLE profile_flow_rates (
    profile_id INTEGER,
    road_id INTEGER,
    vehicles_per_minute REAL,
    FOREIGN KEY (profile_id) REFERENCES traffic_profiles(id)
);
```

### 3.3 API Endpoints
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/traffic/flow-rates` | POST/GET | Set/get current flow rates |
| `/api/traffic/profiles` | POST/GET | Create/list profiles |
| `/api/traffic/profiles/{name}/activate` | POST | Activate profile |

---

## Phase 4: Continuous Optimization

### 4.1 Background Optimization Loop

**Files:**
- `simulator/src/api/optimization_controller.h` - Add:
  ```cpp
  std::atomic<bool> continuousOptimizationEnabled_{false};
  int optimizationIntervalMinutes_ = 15;
  std::vector<TimingTransition> activeTransitions_;

  void startContinuousOptimization(int intervalMinutes);
  void stopContinuousOptimization();
  void applyOptimizedTimings(const Chromosome& chromosome, bool gradual);
  void updateTimingTransitions(double dt);
  ```

### 4.2 Gradual Timing Blending

**TimingTransition struct:**
```cpp
struct TimingTransition {
    roadID roadId;
    unsigned lane;
    double oldGreen, newGreen;
    double oldRed, newRed;
    double startTime;
    double durationSeconds;  // 300s = 5 minutes
    bool completed;
};
```

**Blending formula:** Linear interpolation over 5 minutes
```cpp
progress = elapsed / 300.0;
currentGreen = oldGreen + (newGreen - oldGreen) * progress;
```

### 4.3 API Endpoints
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/optimization/continuous/start` | POST | Start background GA |
| `/api/optimization/continuous/stop` | POST | Stop background GA |
| `/api/optimization/continuous/status` | GET | Status + active transitions |
| `/api/optimization/apply/{runId}` | POST | Manually apply results |

---

## Phase 5: Travel Time Metrics + Frontend

### 5.1 Sample Vehicle Tracking

**Files:**
- `simulator/src/core/vehicle.h` - Add:
  ```cpp
  bool isSampleVehicle_ = false;
  roadID originRoad_, destinationRoad_;
  double spawnTime_;
  void markAsSampleVehicle(roadID origin, roadID dest);
  ```

**New Files:**
- `simulator/src/optimization/travel_time_collector.h/cpp`
  - `recordTrip(vehicle, timestamp)`
  - `getODStats(origin, dest)` - avg, min, max, P50, P95

### 5.2 Database Schema

**New File:** `database/migrations/004_travel_times.sql`
```sql
CREATE TABLE travel_time_records (
    id INTEGER PRIMARY KEY,
    origin_road_id INTEGER,
    destination_road_id INTEGER,
    travel_time_seconds REAL,
    recorded_at INTEGER
);

CREATE TABLE od_pair_statistics (
    origin_road_id INTEGER,
    destination_road_id INTEGER,
    avg_travel_time REAL,
    sample_count INTEGER,
    UNIQUE(origin_road_id, destination_road_id)
);
```

### 5.3 Frontend: Density Coloring

**File:** `frontend/src/pages/MapView.tsx`

Replace speed-based coloring with density-based:
```typescript
const getDensityColor = (vehiclesPerKm: number): string => {
  if (vehiclesPerKm < 20) return '#22c55e';   // Green: free flow
  if (vehiclesPerKm < 50) return '#eab308';   // Yellow: moderate
  if (vehiclesPerKm < 100) return '#f97316';  // Orange: heavy
  return '#ef4444';                            // Red: congestion
};
```

### 5.4 Frontend: Optimization Dashboard

**New File:** `frontend/src/pages/OptimizationDashboard.tsx`
- Continuous optimization status panel
- Active profile indicator
- Timing transition progress bars
- Travel time trends chart
- Before/after fitness comparison

**File:** `frontend/src/types/api.ts` - Add types:
```typescript
interface TrafficProfile { name, startHour, endHour, flowRates[], isActive }
interface ContinuousOptimizationStatus { enabled, intervalMinutes, activeTransitions }
interface ODPairStats { origin, destination, avgTime, minTime, maxTime, count }
```

---

## File Summary

| Phase | File | Action |
|-------|------|--------|
| 1 | `simulator/src/core/trafficlight.h/cpp` | Modify - add setTimings() |
| 1 | `simulator/src/core/road.h/cpp` | Modify - add spawning, TL access |
| 2 | `simulator/src/api/server.h/cpp` | Modify - continuous mode |
| 3 | `simulator/src/api/traffic_data_controller.h/cpp` | **New** |
| 3 | `database/migrations/003_traffic_profiles.sql` | **New** |
| 4 | `simulator/src/api/optimization_controller.h/cpp` | Modify - continuous GA |
| 5 | `simulator/src/core/vehicle.h/cpp` | Modify - sample tracking |
| 5 | `simulator/src/optimization/travel_time_collector.h/cpp` | **New** |
| 5 | `database/migrations/004_travel_times.sql` | **New** |
| 5 | `frontend/src/pages/MapView.tsx` | Modify - density colors |
| 5 | `frontend/src/pages/OptimizationDashboard.tsx` | **New** |
| 5 | `frontend/src/types/api.ts` | Modify - new types |

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Timing changes during vehicle crossing | Apply at phase boundaries, 5-min blend |
| State corruption on crash | Database persistence, clean restart |
| Memory growth in long runs | Circular buffers, periodic cleanup |
| Frontend slowdown with many vehicles | Clustering, visibility culling |

---

## Implementation Order

1. **Phase 1** (Week 1-2): Traffic light mutation + vehicle spawning
2. **Phase 2** (Week 3-4): Continuous operation + crash recovery
3. **Phase 3** (Week 5-7): Traffic data API + time profiles
4. **Phase 4** (Week 8-10): Continuous optimization + gradual blending
5. **Phase 5** (Week 11-13): Travel times + frontend enhancements

---

## Success Criteria

- [ ] Traffic lights can be modified at runtime
- [ ] Simulation runs indefinitely without memory leaks
- [ ] Flow rates can be injected via API
- [ ] 3 traffic profiles work (morning, evening, off-peak)
- [ ] GA runs continuously, applies results gradually
- [ ] Map shows density-based congestion colors
- [ ] Travel time metrics available per OD pair
