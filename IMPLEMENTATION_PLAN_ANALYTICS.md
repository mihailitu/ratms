# Implementation Plan: Option G - Performance Analytics Dashboard

**Phase:** 12
**Objective:** Add comprehensive traffic analysis tools with comparative analysis, statistical summaries, and export capabilities

---

## Overview

This implementation will add a new Analytics page to the web dashboard that provides:
1. **Multi-simulation comparison** - Compare metrics across different simulation runs
2. **Statistical summaries** - Min, max, mean, median, standard deviation, percentiles (25th, 75th, 95th)
3. **Time-series comparison charts** - Overlay multiple simulations on the same graph
4. **CSV export** - Export metrics data for external analysis
5. **Metric type filtering** - Focus on specific metrics (queue length, speed, throughput, etc.)

---

## Backend Implementation (C++)

### 1. Database Manager Enhancements (`simulator/src/data/storage/database_manager.h/cpp`)

#### New Data Structures:
```cpp
struct MetricStatistics {
    std::string metric_type;
    double min_value;
    double max_value;
    double mean_value;
    double median_value;
    double stddev_value;
    double p25_value;  // 25th percentile
    double p75_value;  // 75th percentile
    double p95_value;  // 95th percentile
    int sample_count;
};

struct ComparativeMetrics {
    int simulation_id;
    std::string simulation_name;
    std::vector<MetricRecord> metrics;
};
```

#### New Methods:

**Statistical Analysis:**
```cpp
// Get statistics for a specific metric type across a simulation
MetricStatistics getMetricStatistics(int simulation_id, const std::string& metric_type);

// Get all metric statistics for a simulation (grouped by metric_type)
std::map<std::string, MetricStatistics> getAllMetricStatistics(int simulation_id);

// Get metrics for multiple simulations for comparison
std::vector<ComparativeMetrics> getComparativeMetrics(
    const std::vector<int>& simulation_ids,
    const std::string& metric_type
);
```

**Implementation Details:**
- Use SQLite aggregate functions: MIN, MAX, AVG, COUNT
- Calculate percentiles using ORDER BY and LIMIT/OFFSET
- Calculate standard deviation using formula: sqrt(avg(x²) - avg(x)²)
- Join with simulations table to get simulation names

**SQL Queries:**
```sql
-- Statistics query
SELECT
    metric_type,
    MIN(value) as min_value,
    MAX(value) as max_value,
    AVG(value) as mean_value,
    COUNT(*) as sample_count
FROM metrics
WHERE simulation_id = ? AND metric_type = ?

-- Percentile query (median example)
SELECT value
FROM metrics
WHERE simulation_id = ? AND metric_type = ?
ORDER BY value
LIMIT 1 OFFSET (SELECT COUNT(*)/2 FROM metrics WHERE simulation_id = ? AND metric_type = ?)
```

---

### 2. API Server Enhancements (`simulator/src/api/server.h/cpp`)

#### New API Endpoints:

**A. Get Statistics for Single Simulation**
```
GET /api/analytics/simulations/:id/statistics
GET /api/analytics/simulations/:id/statistics/:metric_type
```

Response:
```json
{
  "simulation_id": 1,
  "simulation_name": "City Grid 10x10",
  "statistics": {
    "avg_queue_length": {
      "metric_type": "avg_queue_length",
      "min_value": 0.0,
      "max_value": 5.2,
      "mean_value": 1.8,
      "median_value": 1.5,
      "stddev_value": 1.2,
      "p25_value": 0.8,
      "p75_value": 2.5,
      "p95_value": 4.1,
      "sample_count": 500
    },
    "avg_speed": { ... }
  }
}
```

**B. Compare Multiple Simulations**
```
POST /api/analytics/compare
Body: {
  "simulation_ids": [1, 2, 3],
  "metric_type": "avg_queue_length"
}
```

Response:
```json
{
  "metric_type": "avg_queue_length",
  "simulations": [
    {
      "simulation_id": 1,
      "simulation_name": "Baseline",
      "metrics": [
        {"timestamp": 0.0, "value": 1.5},
        {"timestamp": 1.0, "value": 1.8}
      ]
    },
    {
      "simulation_id": 2,
      "simulation_name": "Optimized",
      "metrics": [...]
    }
  ]
}
```

**C. Export Metrics to CSV**
```
GET /api/analytics/simulations/:id/export
GET /api/analytics/simulations/:id/export?metric_type=avg_queue_length
```

Response: CSV file download
```csv
timestamp,metric_type,value,road_id,unit
0.0,avg_queue_length,1.5,0,vehicles
1.0,avg_queue_length,1.8,0,vehicles
```

**D. Get Available Metric Types**
```
GET /api/analytics/metric-types
```

Response:
```json
{
  "metric_types": [
    "avg_queue_length",
    "avg_speed",
    "vehicles_exited",
    "max_queue_length"
  ]
}
```

**Implementation:**
- Add new handler methods in Server class
- Use DatabaseManager analytics methods
- Format responses as JSON using nlohmann/json
- Set appropriate Content-Type headers (text/csv for CSV export)
- Add CORS headers for all endpoints

---

## Frontend Implementation (TypeScript/React)

### 3. TypeScript Types (`frontend/src/types/api.ts`)

Add new types:
```typescript
export interface MetricStatistics {
  metric_type: string;
  min_value: number;
  max_value: number;
  mean_value: number;
  median_value: number;
  stddev_value: number;
  p25_value: number;
  p75_value: number;
  p95_value: number;
  sample_count: number;
}

export interface SimulationStatistics {
  simulation_id: number;
  simulation_name: string;
  statistics: Record<string, MetricStatistics>;
}

export interface ComparativeMetric {
  simulation_id: number;
  simulation_name: string;
  metrics: Array<{
    timestamp: number;
    value: number;
  }>;
}

export interface ComparisonResponse {
  metric_type: string;
  simulations: ComparativeMetric[];
}

export interface MetricTypesResponse {
  metric_types: string[];
}
```

---

### 4. API Client Updates (`frontend/src/services/apiClient.ts`)

Add new methods:
```typescript
class ApiClient {
  // Get statistics for a simulation
  async getSimulationStatistics(
    simulationId: number,
    metricType?: string
  ): Promise<SimulationStatistics> {
    const url = metricType
      ? `/api/analytics/simulations/${simulationId}/statistics/${metricType}`
      : `/api/analytics/simulations/${simulationId}/statistics`;
    const response = await this.client.get<SimulationStatistics>(url);
    return response.data;
  }

  // Compare multiple simulations
  async compareSimulations(
    simulationIds: number[],
    metricType: string
  ): Promise<ComparisonResponse> {
    const response = await this.client.post<ComparisonResponse>(
      '/api/analytics/compare',
      { simulation_ids: simulationIds, metric_type: metricType }
    );
    return response.data;
  }

  // Export metrics as CSV
  async exportMetrics(
    simulationId: number,
    metricType?: string
  ): Promise<Blob> {
    const url = metricType
      ? `/api/analytics/simulations/${simulationId}/export?metric_type=${metricType}`
      : `/api/analytics/simulations/${simulationId}/export`;
    const response = await this.client.get(url, {
      responseType: 'blob'
    });
    return response.data;
  }

  // Get available metric types
  async getMetricTypes(): Promise<MetricTypesResponse> {
    const response = await this.client.get<MetricTypesResponse>(
      '/api/analytics/metric-types'
    );
    return response.data;
  }
}
```

---

### 5. Analytics Page (`frontend/src/pages/Analytics.tsx`)

New page component with three main sections:

#### Section A: Simulation Selector
- Multi-select dropdown for choosing simulations to compare
- Metric type selector (avg_queue_length, avg_speed, etc.)
- "Compare" button to generate charts

#### Section B: Comparison Charts
- Line chart showing time-series data for selected simulations
- Different colors for each simulation
- Tooltips showing exact values
- Legend with simulation names
- X-axis: Simulation time (seconds)
- Y-axis: Metric value (with unit)

#### Section C: Statistical Summary Table
- Table showing statistics for each selected simulation
- Columns: Simulation, Min, Max, Mean, Median, Std Dev, P25, P75, P95, Samples
- Sortable columns
- Color coding for best/worst values
- Export button for CSV download

**Component Structure:**
```tsx
const Analytics: React.FC = () => {
  const [simulations, setSimulations] = useState<SimulationRecord[]>([]);
  const [selectedSimIds, setSelectedSimIds] = useState<number[]>([]);
  const [metricType, setMetricType] = useState<string>('avg_queue_length');
  const [comparisonData, setComparisonData] = useState<ComparisonResponse | null>(null);
  const [statistics, setStatistics] = useState<Map<number, SimulationStatistics>>(new Map());

  // Fetch available simulations on mount
  // Handle simulation selection
  // Handle metric type change
  // Fetch comparison data
  // Fetch statistics for each simulation

  return (
    <div className="analytics-page">
      <h1>Performance Analytics</h1>

      <SimulationSelector />
      <MetricTypeSelector />
      <ComparisonChart />
      <StatisticsTable />
      <ExportButton />
    </div>
  );
};
```

---

### 6. Comparison Chart Component (`frontend/src/components/ComparisonChart.tsx`)

Using Recharts library:
```tsx
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend } from 'recharts';

const ComparisonChart: React.FC<{ data: ComparisonResponse }> = ({ data }) => {
  // Transform data for Recharts format
  const chartData = transformDataForChart(data);

  return (
    <LineChart width={800} height={400} data={chartData}>
      <CartesianGrid strokeDasharray="3 3" />
      <XAxis dataKey="timestamp" label={{ value: 'Time (s)', position: 'insideBottom', offset: -5 }} />
      <YAxis label={{ value: data.metric_type, angle: -90, position: 'insideLeft' }} />
      <Tooltip />
      <Legend />
      {data.simulations.map((sim, index) => (
        <Line
          key={sim.simulation_id}
          type="monotone"
          dataKey={`sim_${sim.simulation_id}`}
          name={sim.simulation_name}
          stroke={COLORS[index % COLORS.length]}
          strokeWidth={2}
        />
      ))}
    </LineChart>
  );
};
```

---

### 7. Statistics Table Component (`frontend/src/components/StatisticsTable.tsx`)

Display statistical summaries:
```tsx
const StatisticsTable: React.FC<{ statistics: SimulationStatistics[] }> = ({ statistics }) => {
  return (
    <table className="statistics-table">
      <thead>
        <tr>
          <th>Simulation</th>
          <th>Min</th>
          <th>Max</th>
          <th>Mean</th>
          <th>Median</th>
          <th>Std Dev</th>
          <th>P25</th>
          <th>P75</th>
          <th>P95</th>
          <th>Samples</th>
        </tr>
      </thead>
      <tbody>
        {statistics.map(stat => (
          <tr key={stat.simulation_id}>
            <td>{stat.simulation_name}</td>
            <td>{stat.min_value.toFixed(2)}</td>
            <td>{stat.max_value.toFixed(2)}</td>
            <td>{stat.mean_value.toFixed(2)}</td>
            <td>{stat.median_value.toFixed(2)}</td>
            <td>{stat.stddev_value.toFixed(2)}</td>
            <td>{stat.p25_value.toFixed(2)}</td>
            <td>{stat.p75_value.toFixed(2)}</td>
            <td>{stat.p95_value.toFixed(2)}</td>
            <td>{stat.sample_count}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};
```

---

### 8. CSV Export Functionality

Download handler:
```typescript
const handleExport = async (simulationId: number, metricType?: string) => {
  try {
    const blob = await apiClient.exportMetrics(simulationId, metricType);
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `simulation_${simulationId}_${metricType || 'all'}_metrics.csv`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    window.URL.revokeObjectURL(url);
  } catch (error) {
    console.error('Export failed:', error);
  }
};
```

---

### 9. Navigation Updates (`frontend/src/components/Layout.tsx`)

Add "Analytics" link to navigation:
```tsx
<nav>
  <Link to="/">Dashboard</Link>
  <Link to="/simulations">Simulations</Link>
  <Link to="/map">Map</Link>
  <Link to="/optimization">Optimization</Link>
  <Link to="/analytics">Analytics</Link>  {/* NEW */}
</nav>
```

---

### 10. Routing (`frontend/src/App.tsx`)

Add analytics route:
```tsx
<Routes>
  <Route path="/" element={<Dashboard />} />
  <Route path="/simulations" element={<Simulations />} />
  <Route path="/simulations/:id" element={<SimulationDetail />} />
  <Route path="/map" element={<MapView />} />
  <Route path="/optimization" element={<Optimization />} />
  <Route path="/analytics" element={<Analytics />} />  {/* NEW */}
</Routes>
```

---

## Implementation Steps

### Phase 1: Backend Foundation (C++)
1. ✅ Add analytics data structures to database_manager.h
2. ⏳ Implement `getMetricStatistics()` in database_manager.cpp
3. ⏳ Implement `getAllMetricStatistics()` in database_manager.cpp
4. ⏳ Implement `getComparativeMetrics()` in database_manager.cpp
5. ⏳ Add analytics endpoint handlers to server.h
6. ⏳ Implement analytics endpoints in server.cpp
7. ⏳ Build and test backend

### Phase 2: Frontend Foundation (TypeScript)
8. ⏳ Add analytics types to api.ts
9. ⏳ Add analytics methods to apiClient.ts
10. ⏳ Create Analytics.tsx page component
11. ⏳ Create ComparisonChart.tsx component
12. ⏳ Create StatisticsTable.tsx component

### Phase 3: Integration
13. ⏳ Add analytics route to App.tsx
14. ⏳ Add navigation link to Layout.tsx
15. ⏳ Implement CSV export functionality
16. ⏳ Add styling with Tailwind CSS

### Phase 4: Testing & Documentation
17. ⏳ Test with multiple simulations
18. ⏳ Test CSV export
19. ⏳ Test statistical calculations
20. ⏳ Update PROJECT_STATUS.md
21. ⏳ Update CLAUDE.md with analytics API documentation

---

## Data Flow

```
User selects simulations + metric type
    ↓
Frontend sends POST /api/analytics/compare
    ↓
Server.handleCompareSimulations()
    ↓
DatabaseManager.getComparativeMetrics()
    ↓
SQL query with JOIN on simulations and metrics tables
    ↓
Return time-series data for each simulation
    ↓
Format as JSON and send to frontend
    ↓
Frontend transforms data for Recharts
    ↓
Display multi-line comparison chart
```

---

## SQL Query Examples

### Comparative Metrics Query:
```sql
SELECT
    m.simulation_id,
    s.name as simulation_name,
    m.timestamp,
    m.value
FROM metrics m
JOIN simulations s ON m.simulation_id = s.id
WHERE m.simulation_id IN (?, ?, ?)
  AND m.metric_type = ?
ORDER BY m.simulation_id, m.timestamp
```

### Statistics Query:
```sql
-- Basic statistics
SELECT
    MIN(value) as min_value,
    MAX(value) as max_value,
    AVG(value) as mean_value,
    COUNT(*) as sample_count
FROM metrics
WHERE simulation_id = ? AND metric_type = ?

-- Standard deviation
SELECT
    SQRT(AVG(value * value) - AVG(value) * AVG(value)) as stddev
FROM metrics
WHERE simulation_id = ? AND metric_type = ?
```

### Percentile Queries:
```sql
-- Median (50th percentile)
SELECT value
FROM (
    SELECT value, ROW_NUMBER() OVER (ORDER BY value) as row_num,
           COUNT(*) OVER () as total_count
    FROM metrics
    WHERE simulation_id = ? AND metric_type = ?
)
WHERE row_num = (total_count + 1) / 2

-- 25th percentile
WHERE row_num = (total_count + 1) / 4

-- 75th percentile
WHERE row_num = (total_count + 1) * 3 / 4

-- 95th percentile
WHERE row_num = (total_count + 1) * 95 / 100
```

---

## UI Mockup

```
┌─────────────────────────────────────────────────────────────┐
│  RATMS - Performance Analytics                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Select Simulations: [✓ Baseline] [✓ Optimized] [  Run 3]  │
│  Metric Type: [avg_queue_length ▼]                         │
│  [Compare Button]                                           │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Comparison Chart                                     │  │
│  │                                                       │  │
│  │   5.0 ┤                                              │  │
│  │       │     ╱╲   ╱╲                                  │  │
│  │   4.0 ┤    ╱  ╲ ╱  ╲    Baseline (blue)            │  │
│  │       │   ╱    ╲     ╲  ╱╲                          │  │
│  │   3.0 ┤  ╱      ╲     ╲╱  ╲                         │  │
│  │       │ ╱        ╲         ╲   Optimized (green)    │  │
│  │   2.0 ┤╱          ╲         ╲─────────              │  │
│  │       │            ╲                                 │  │
│  │   1.0 ┤             ╲___________________________     │  │
│  │       │                                              │  │
│  │   0.0 └──────────────────────────────────────────    │  │
│  │       0s    50s   100s   150s   200s                │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  Statistical Summary:                                       │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Sim      │ Min │ Max │ Mean │ Med │ Std │ P25│ P75 │   │
│  ├──────────┼─────┼─────┼──────┼─────┼─────┼────┼─────┤   │
│  │ Baseline │ 1.2 │ 5.1 │ 2.8  │ 2.5 │ 1.1 │1.8 │ 3.5 │   │
│  │ Optimized│ 0.5 │ 2.3 │ 1.1  │ 1.0 │ 0.4 │0.8 │ 1.3 │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                             │
│  [Export CSV] [Export All Metrics]                         │
└─────────────────────────────────────────────────────────────┘
```

---

## Success Criteria

- ✅ Backend API endpoints return correct statistical data
- ✅ Frontend displays comparison charts for multiple simulations
- ✅ Statistical summaries show accurate percentiles
- ✅ CSV export downloads valid CSV files
- ✅ UI is responsive and styled with Tailwind CSS
- ✅ Navigation includes Analytics link
- ✅ No build errors in backend or frontend
- ✅ Documentation updated in PROJECT_STATUS.md

---

## Estimated Effort

- Backend implementation: 2-3 hours
- Frontend implementation: 2-3 hours
- Testing and debugging: 1 hour
- Documentation: 30 minutes

**Total: 6-7 hours (2-3 development sessions)**

---

## Next Steps After Completion

After Phase 12 is complete, potential enhancements:
- **Heatmaps** for congestion visualization on map
- **PDF report generation** for professional reports
- **Real-time analytics** during running simulations
- **Automated comparison** of GA optimization runs
- **Custom metric definitions** via configuration

---

## Notes

- Use existing Recharts library (already in package.json)
- Leverage existing DatabaseManager pattern with prepared statements
- Follow existing API endpoint patterns for consistency
- Use Tailwind CSS for styling (consistent with rest of app)
- Ensure thread safety with mutex protection in Server class
- Handle edge cases: empty simulations, missing metrics, single simulation
