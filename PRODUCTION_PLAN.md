# Production-Ready Predictive Traffic Light Optimization

## Overview

Build a production-ready traffic management system that:
1. Collects traffic data (simulated for now)
2. Predicts future traffic using time-of-day patterns
3. Runs GA optimization on predicted future state (30-60 min ahead)
4. Validates optimized timings via simulation
5. Gradually rolls out with monitoring and automatic rollback
6. **Provides a real-time dashboard with live map and statistics**

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              FRONTEND DASHBOARD                              │
├─────────────────────────────────────────────────────────────────────────────┤
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐                 │
│  │   Live Map     │  │  Statistics    │  │  Optimization  │                 │
│  │   - Vehicles   │  │  - Flow rates  │  │   Dashboard    │                 │
│  │   - Traffic    │  │  - Queue lens  │  │  - GA status   │                 │
│  │     lights     │  │  - Avg speeds  │  │  - Rollout     │                 │
│  │   - Congestion │  │  - Throughput  │  │  - Predictions │                 │
│  └───────┬────────┘  └───────┬────────┘  └───────┬────────┘                 │
│          │                   │                   │                          │
│          └───────────────────┼───────────────────┘                          │
│                              │ WebSocket / SSE                              │
└──────────────────────────────┼──────────────────────────────────────────────┘
                               │
┌──────────────────────────────┼──────────────────────────────────────────────┐
│                         API SERVER                                           │
├──────────────────────────────┼──────────────────────────────────────────────┤
│                              │                                               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                   │
│  │   Traffic    │───▶│  Historical  │───▶│  Prediction  │                   │
│  │   Metrics    │    │   Patterns   │    │   Service    │                   │
│  │  Collector   │    │   Storage    │    │              │                   │
│  └──────────────┘    └──────────────┘    └──────┬───────┘                   │
│         │                                        │                          │
│         │ Current State                          │ Predicted State          │
│         │                                        │ (T+30min to T+60min)     │
│         ▼                                        ▼                          │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                   │
│  │   Running    │    │     GA       │◀───│  Predicted   │                   │
│  │  Simulation  │    │  Optimizer   │    │   Network    │                   │
│  └──────────────┘    └──────┬───────┘    └──────────────┘                   │
│                              │                                              │
│                              │ Optimized Timings                            │
│                              ▼                                              │
│                       ┌──────────────┐                                      │
│                       │  Validation  │                                      │
│                       │  Simulation  │                                      │
│                       └──────┬───────┘                                      │
│                              │                                              │
│                              │ If improvement > threshold                   │
│                              ▼                                              │
│                       ┌──────────────┐    ┌──────────────┐                  │
│                       │   Gradual    │───▶│  Monitoring  │                  │
│                       │   Rollout    │◀───│  & Rollback  │                  │
│                       └──────────────┘    └──────────────┘                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Part A: Frontend Dashboard

### A1. Live Map View

**Goal**: Real-time visualization of the entire city traffic state.

#### Features
- **Vehicle Layer**: Show all vehicles as moving dots, color-coded by speed
  - Green: > 70% of speed limit
  - Yellow: 30-70% of speed limit
  - Red: < 30% of speed limit (congested)
- **Traffic Light Layer**: Show all traffic lights with current state (R/Y/G)
- **Road Layer**: Base map with road network
- **Congestion Heatmap**: Overlay showing congestion levels per road segment
- **Click-to-inspect**: Click road/intersection for detailed stats

#### Technical Implementation
```typescript
// frontend/src/pages/LiveMap.tsx
interface LiveMapProps {
  // Real-time data via SSE
  vehicles: VehiclePosition[];
  trafficLights: TrafficLightState[];
  roadMetrics: RoadMetrics[];
}

// Update frequency: 200ms (5 FPS) for smooth animation
// Use Leaflet.js with custom layers
// WebGL rendering for large vehicle counts (>10k vehicles)
```

#### New Components
| Component | Description |
|-----------|-------------|
| `VehicleLayer.tsx` | Renders vehicle dots with interpolation |
| `TrafficLightLayer.tsx` | Renders traffic light markers |
| `CongestionHeatmap.tsx` | Road segment color overlay |
| `RoadInfoPopup.tsx` | Click-to-inspect popup |
| `MapControls.tsx` | Layer toggles, zoom controls |

### A2. Statistics Dashboard

**Goal**: Real-time and historical traffic statistics.

#### Panels
1. **Live Metrics Panel**
   - Current vehicle count
   - Average network speed
   - Total queue length
   - Vehicles entering/exiting per minute
   - Congestion index (0-100)

2. **Time-Series Charts**
   - Vehicle count over time (last 1h/6h/24h)
   - Average speed over time
   - Queue length trends
   - Throughput (vehicles/min)

3. **Per-Road Statistics**
   - Sortable table of all roads
   - Columns: Road ID, Vehicles, Avg Speed, Queue, Congestion %
   - Click to highlight on map

4. **Comparison View**
   - Compare current vs predicted metrics
   - Compare before/after optimization

#### Technical Implementation
```typescript
// frontend/src/pages/Statistics.tsx
interface StatisticsProps {
  liveMetrics: NetworkMetrics;
  historicalData: TimeSeriesData[];
  roadStats: RoadStatistics[];
}

// Use Recharts for time-series
// Update live metrics every 1 second
// Historical data polling every 30 seconds
```

#### New Components
| Component | Description |
|-----------|-------------|
| `LiveMetricsPanel.tsx` | Real-time KPI cards |
| `TimeSeriesChart.tsx` | Historical trends |
| `RoadStatsTable.tsx` | Per-road statistics |
| `ComparisonChart.tsx` | Before/after comparison |
| `CongestionGauge.tsx` | Visual congestion indicator |

### A3. Optimization Dashboard

**Goal**: Monitor and control the GA optimization pipeline.

#### Panels
1. **Optimization Status**
   - Current state: IDLE / PREDICTING / OPTIMIZING / VALIDATING / ROLLING_OUT
   - Progress bar for current operation
   - Time until next optimization cycle

2. **Prediction View**
   - Current time vs predicted time
   - Predicted traffic levels (heatmap preview)
   - Confidence score

3. **GA Progress**
   - Generation progress (e.g., 45/100)
   - Fitness curve (real-time chart)
   - Best fitness found
   - Population diversity

4. **Validation Results**
   - Predicted improvement %
   - Simulated improvement %
   - Confidence score
   - Pass/Fail status

5. **Rollout Monitor**
   - Transition progress (0-100%)
   - Active timing transitions list
   - Pre-rollout vs current metrics
   - Rollback button (manual override)

6. **History**
   - Past optimization runs
   - Success/failure rate
   - Average improvement achieved
   - Rollback history

#### Technical Implementation
```typescript
// frontend/src/pages/OptimizationDashboard.tsx
interface OptimizationDashboardProps {
  pipelineStatus: PipelineStatus;
  prediction: PredictionResult;
  gaProgress: GAProgress;
  validation: ValidationResult;
  rollout: RolloutStatus;
  history: OptimizationHistory[];
}
```

#### New Components
| Component | Description |
|-----------|-------------|
| `PipelineStatusBar.tsx` | Visual pipeline progress |
| `PredictionPreview.tsx` | Mini-map with predicted state |
| `FitnessCurve.tsx` | Real-time GA fitness chart |
| `ValidationCard.tsx` | Validation results display |
| `RolloutProgress.tsx` | Transition progress bars |
| `RollbackButton.tsx` | Emergency rollback control |
| `OptimizationHistory.tsx` | Historical runs table |

### A4. New API Endpoints for Frontend

```typescript
// Real-time streaming (SSE)
GET /api/stream/vehicles          // Vehicle positions (200ms updates)
GET /api/stream/traffic-lights    // Traffic light states (1s updates)
GET /api/stream/metrics           // Network metrics (1s updates)
GET /api/stream/optimization      // Optimization progress (1s updates)

// REST endpoints
GET /api/statistics/live          // Current network statistics
GET /api/statistics/history       // Historical time-series data
GET /api/statistics/roads         // Per-road statistics
GET /api/statistics/comparison    // Before/after comparison

GET /api/prediction/current       // Current prediction state
GET /api/prediction/forecast      // Predicted metrics for T+N

GET /api/optimization/pipeline    // Full pipeline status
GET /api/optimization/ga/progress // GA-specific progress
GET /api/optimization/validation  // Current validation results
GET /api/optimization/rollout     // Rollout status

POST /api/optimization/rollback   // Manual rollback trigger
POST /api/optimization/abort      // Abort current operation
```

---

## Part B: Backend - Traffic Pattern Storage

### B1. Database Schema

**File**: `database/migrations/004_traffic_patterns.sql`

```sql
-- Historical traffic patterns aggregated by time slot
CREATE TABLE IF NOT EXISTS traffic_patterns (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    road_id INTEGER NOT NULL,
    day_of_week INTEGER NOT NULL,        -- 0=Sunday, 1=Monday, ..., 6=Saturday
    time_slot INTEGER NOT NULL,          -- 0-47 (30-min slots)
    avg_vehicle_count REAL DEFAULT 0,
    avg_queue_length REAL DEFAULT 0,
    avg_speed REAL DEFAULT 0,
    sample_count INTEGER DEFAULT 0,
    last_updated INTEGER NOT NULL,
    UNIQUE(road_id, day_of_week, time_slot)
);

CREATE INDEX idx_traffic_patterns_lookup ON traffic_patterns(day_of_week, time_slot);

-- Raw traffic snapshots (kept for N days)
CREATE TABLE IF NOT EXISTS traffic_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    road_id INTEGER NOT NULL,
    vehicle_count INTEGER NOT NULL,
    queue_length REAL NOT NULL,
    avg_speed REAL NOT NULL
);

CREATE INDEX idx_traffic_snapshots_time ON traffic_snapshots(timestamp);

-- Optimization history
CREATE TABLE IF NOT EXISTS optimization_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    started_at INTEGER NOT NULL,
    completed_at INTEGER,
    status TEXT NOT NULL,              -- 'running', 'completed', 'failed', 'rolled_back'
    prediction_horizon_minutes INTEGER,
    baseline_fitness REAL,
    optimized_fitness REAL,
    improvement_percent REAL,
    validation_passed INTEGER,
    rollback_reason TEXT
);

-- Rollback events for analysis
CREATE TABLE IF NOT EXISTS rollback_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    optimization_id INTEGER NOT NULL,
    timestamp INTEGER NOT NULL,
    reason TEXT NOT NULL,
    metrics_before TEXT,               -- JSON
    metrics_after TEXT,                -- JSON
    FOREIGN KEY (optimization_id) REFERENCES optimization_history(id)
);
```

### B2. Traffic Pattern Storage Class

**File**: `simulator/src/data/storage/traffic_pattern_storage.h`

```cpp
struct TrafficPattern {
    int roadId;
    int dayOfWeek;
    int timeSlot;
    double avgVehicleCount;
    double avgQueueLength;
    double avgSpeed;
    int sampleCount;
};

struct TrafficSnapshot {
    int64_t timestamp;
    int roadId;
    int vehicleCount;
    double queueLength;
    double avgSpeed;
};

class TrafficPatternStorage {
public:
    TrafficPatternStorage(std::shared_ptr<DatabaseManager> db);

    // Record current state
    void recordSnapshot(const std::map<int, RoadMetrics>& metrics);
    void recordSnapshotBatch(const std::vector<TrafficSnapshot>& snapshots);

    // Query patterns
    std::vector<TrafficPattern> getPatterns(int dayOfWeek, int timeSlot);
    TrafficPattern getPattern(int roadId, int dayOfWeek, int timeSlot);
    std::vector<TrafficPattern> getPatternsForTime(
        std::chrono::system_clock::time_point time);

    // Maintenance
    void aggregateSnapshots();          // Aggregate raw → patterns
    void pruneOldSnapshots(int days);   // Cleanup old data

private:
    std::shared_ptr<DatabaseManager> db_;
    static int getTimeSlot(std::chrono::system_clock::time_point time);
    static int getDayOfWeek(std::chrono::system_clock::time_point time);
};
```

---

## Part C: Backend - Prediction Service

**File**: `simulator/src/prediction/traffic_predictor.h`

```cpp
struct PredictionConfig {
    int horizonMinutes = 30;
    double patternWeight = 0.7;
    double currentWeight = 0.3;
    double trendFactor = 0.1;
};

struct PredictedMetrics {
    int roadId;
    double vehicleCount;
    double queueLength;
    double avgSpeed;
    double confidence;      // 0-1, based on sample count
};

class TrafficPredictor {
public:
    TrafficPredictor(
        std::shared_ptr<TrafficPatternStorage> patterns,
        std::shared_ptr<simulator::Simulator> simulator,
        std::mutex& simMutex);

    // Get predictions
    std::map<int, PredictedMetrics> predictRoadMetrics(
        std::chrono::minutes horizon);

    // Create network state matching prediction
    std::vector<simulator::Road> createPredictedNetwork(
        std::chrono::minutes horizon);

    void setConfig(const PredictionConfig& config);
    PredictionConfig getConfig() const;

private:
    PredictedMetrics predictForRoad(int roadId, std::chrono::minutes horizon);
    double calculateTrend(int roadId);
    double calculateConfidence(const TrafficPattern& pattern);
};
```

---

## Part D: Backend - Predictive Optimizer

**File**: `simulator/src/api/predictive_optimizer.h`

```cpp
struct PredictiveOptimizationConfig {
    int predictionHorizonMinutes = 30;
    int gaPopulationSize = 50;
    int gaGenerations = 50;
    int validationSteps = 1000;
    double minimumImprovement = 5.0;
};

struct OptimizationResult {
    simulator::Chromosome bestChromosome;
    double baselineFitness;
    double optimizedFitness;
    double improvementPercent;
    bool validated;
    std::chrono::system_clock::time_point targetTime;
};

enum class PipelineStatus {
    IDLE,
    PREDICTING,
    OPTIMIZING,
    VALIDATING,
    ROLLING_OUT,
    MONITORING,
    ROLLING_BACK
};

class PredictiveOptimizer {
public:
    PredictiveOptimizer(
        std::shared_ptr<TrafficPredictor> predictor,
        std::shared_ptr<TimingValidator> validator,
        std::shared_ptr<simulator::Simulator> simulator,
        std::mutex& simMutex);

    // Run full pipeline
    OptimizationResult runOptimizationPipeline(
        std::chrono::minutes horizon = std::chrono::minutes(30));

    // Individual stages
    std::map<int, PredictedMetrics> runPrediction(std::chrono::minutes horizon);
    OptimizationResult runOptimization(
        const std::vector<simulator::Road>& predictedNetwork);
    ValidationResult runValidation(const OptimizationResult& result);

    // Control
    void startPeriodicOptimization(std::chrono::minutes interval);
    void stopPeriodicOptimization();
    void abort();

    // Status
    PipelineStatus getStatus() const;
    int getGAGeneration() const;
    double getBestFitness() const;

    void setConfig(const PredictiveOptimizationConfig& config);
};
```

---

## Part E: Backend - Validation & Rollout

**File**: `simulator/src/validation/timing_validator.h`

```cpp
struct ValidationResult {
    bool passed;
    double predictedImprovement;
    double simulatedImprovement;
    double confidenceScore;
    std::string failureReason;
};

class TimingValidator {
public:
    ValidationResult validate(
        const simulator::Chromosome& optimized,
        const std::vector<simulator::Road>& network,
        int simulationSteps = 1000);

    ValidationResult compareTimings(
        const simulator::Chromosome& current,
        const simulator::Chromosome& proposed,
        const std::vector<simulator::Road>& network,
        int steps = 1000);

private:
    double runSimulation(
        const std::vector<simulator::Road>& network,
        const simulator::Chromosome& timings,
        int steps);
};
```

**File**: `simulator/src/api/continuous_optimization_controller.h` (additions)

```cpp
struct RolloutConfig {
    int transitionDurationSeconds = 300;
    int monitoringWindowSeconds = 600;
    double regressionThreshold = 10.0;
    bool autoRollbackEnabled = true;
};

struct RolloutState {
    enum Status { IDLE, TRANSITIONING, MONITORING, ROLLING_BACK };
    Status status = IDLE;
    simulator::Chromosome previousTimings;
    simulator::Chromosome targetTimings;
    std::chrono::steady_clock::time_point transitionStart;
    std::chrono::steady_clock::time_point monitoringStart;
    double preRolloutFitness;
};

// Add to ContinuousOptimizationController
void applyValidatedTimings(const Chromosome& timings, const ValidationResult& validation);
void monitorRollout();
void initiateRollback(const std::string& reason);
bool detectRegression();
```

---

## Implementation Sprints

### Sprint 1: Foundation (Pattern Storage + Basic Dashboard)
**Duration**: 1 week

**Backend**:
1. Create database migration for traffic patterns
2. Implement `TrafficPatternStorage` class
3. Add snapshot recording to simulation loop
4. Add pattern aggregation job

**Frontend**:
1. Create `LiveMap.tsx` with basic vehicle layer
2. Implement SSE connection for vehicle positions
3. Add traffic light layer
4. Basic map controls

**Deliverable**: Live map showing vehicles and traffic lights

### Sprint 2: Statistics Dashboard
**Duration**: 1 week

**Backend**:
1. Add `/api/statistics/*` endpoints
2. Implement historical data queries
3. Add per-road statistics aggregation

**Frontend**:
1. Create `Statistics.tsx` page
2. Implement `LiveMetricsPanel`
3. Add time-series charts
4. Add road statistics table

**Deliverable**: Statistics dashboard with live and historical data

### Sprint 3: Prediction Service
**Duration**: 1 week

**Backend**:
1. Implement `TrafficPredictor` class
2. Add prediction API endpoints
3. Test prediction accuracy
4. Tune blending weights

**Frontend**:
1. Add prediction preview to Optimization dashboard
2. Show predicted vs actual comparison
3. Display confidence scores

**Deliverable**: Working prediction with visualization

### Sprint 4: Predictive GA Optimization
**Duration**: 1 week

**Backend**:
1. Implement `PredictiveOptimizer` class
2. Connect predictor to GA
3. Add GA progress streaming
4. Add optimization history storage

**Frontend**:
1. Create `OptimizationDashboard.tsx`
2. Implement `FitnessCurve` real-time chart
3. Add pipeline status visualization
4. Show optimization history

**Deliverable**: Full GA optimization with real-time monitoring

### Sprint 5: Validation & Rollout
**Duration**: 1 week

**Backend**:
1. Implement `TimingValidator` class
2. Add validation step to pipeline
3. Implement monitoring and rollback
4. Add rollback API endpoints

**Frontend**:
1. Add validation results display
2. Implement rollout progress visualization
3. Add rollback button
4. Show regression alerts

**Deliverable**: Complete pipeline with safe rollout

### Sprint 6: Polish & Integration
**Duration**: 1 week

1. End-to-end integration testing
2. Performance optimization
3. Error handling improvements
4. Documentation
5. UI polish and responsive design

**Deliverable**: Production-ready system

---

## Files Summary

### New Backend Files
| File | Description |
|------|-------------|
| `database/migrations/004_traffic_patterns.sql` | Pattern storage schema |
| `simulator/src/data/storage/traffic_pattern_storage.h/cpp` | Pattern CRUD |
| `simulator/src/prediction/traffic_predictor.h/cpp` | Prediction service |
| `simulator/src/prediction/predicted_metrics.h` | Prediction types |
| `simulator/src/api/predictive_optimizer.h/cpp` | Predictive optimizer |
| `simulator/src/validation/timing_validator.h/cpp` | Validation service |

### Modified Backend Files
| File | Changes |
|------|---------|
| `simulator/src/api/continuous_optimization_controller.h/cpp` | Add rollout monitoring |
| `simulator/src/api/server.cpp` | Register new routes, add SSE streams |
| `simulator/src/data/storage/database_manager.h/cpp` | Add pattern methods |
| `simulator/CMakeLists.txt` | Add new source files |

### New Frontend Files
| File | Description |
|------|-------------|
| `frontend/src/pages/LiveMap.tsx` | Live map view |
| `frontend/src/pages/Statistics.tsx` | Statistics dashboard |
| `frontend/src/pages/OptimizationDashboard.tsx` | Optimization control |
| `frontend/src/components/map/VehicleLayer.tsx` | Vehicle rendering |
| `frontend/src/components/map/TrafficLightLayer.tsx` | Traffic light markers |
| `frontend/src/components/map/CongestionHeatmap.tsx` | Congestion overlay |
| `frontend/src/components/stats/LiveMetricsPanel.tsx` | Real-time KPIs |
| `frontend/src/components/stats/TimeSeriesChart.tsx` | Historical charts |
| `frontend/src/components/stats/RoadStatsTable.tsx` | Per-road table |
| `frontend/src/components/optimization/PipelineStatus.tsx` | Pipeline progress |
| `frontend/src/components/optimization/FitnessCurve.tsx` | GA fitness chart |
| `frontend/src/components/optimization/RolloutProgress.tsx` | Rollout visualization |
| `frontend/src/hooks/useVehicleStream.ts` | Vehicle SSE hook |
| `frontend/src/hooks/useMetricsStream.ts` | Metrics SSE hook |
| `frontend/src/hooks/useOptimizationStream.ts` | Optimization SSE hook |

### Modified Frontend Files
| File | Changes |
|------|---------|
| `frontend/src/App.tsx` | Add new routes |
| `frontend/src/components/Layout.tsx` | Add navigation links |
| `frontend/src/services/apiClient.ts` | Add new API methods |
| `frontend/src/types/api.ts` | Add new types |

---

## Configuration

**Default production config:**
```json
{
  "prediction": {
    "horizonMinutes": 30,
    "patternWeight": 0.7,
    "currentWeight": 0.3,
    "snapshotIntervalSeconds": 60
  },
  "optimization": {
    "populationSize": 50,
    "generations": 50,
    "minimumImprovement": 5.0,
    "runIntervalMinutes": 15
  },
  "validation": {
    "simulationSteps": 1000,
    "requiredImprovement": 2.0
  },
  "rollout": {
    "transitionSeconds": 300,
    "monitoringSeconds": 600,
    "regressionThreshold": 10.0,
    "autoRollback": true
  },
  "frontend": {
    "vehicleUpdateMs": 200,
    "metricsUpdateMs": 1000,
    "historyRetentionHours": 24
  }
}
```

---

## Future Enhancements

1. **ML-based prediction**: Replace pattern lookup with trained model
2. **Multi-horizon optimization**: Optimize for multiple time horizons
3. **Event-aware prediction**: Account for known events
4. **Weather integration**: Adjust predictions based on weather
5. **Feedback learning**: Auto-adjust weights based on accuracy
6. **Mobile app**: Real-time traffic updates for drivers
7. **Alert system**: Notifications for anomalies and rollbacks
