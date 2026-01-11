-- Traffic Pattern Storage Schema
-- Stores time-of-day traffic patterns for predictive optimization

-- Raw traffic snapshots collected periodically (kept for N days)
CREATE TABLE IF NOT EXISTS traffic_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    road_id INTEGER NOT NULL,
    vehicle_count INTEGER NOT NULL,
    queue_length REAL NOT NULL,
    avg_speed REAL NOT NULL,
    flow_rate REAL NOT NULL DEFAULT 0,
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Aggregated traffic patterns by time slot
-- 48 time slots per day (30-minute intervals)
-- day_of_week: 0=Sunday, 1=Monday, ..., 6=Saturday
CREATE TABLE IF NOT EXISTS traffic_patterns (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    road_id INTEGER NOT NULL,
    day_of_week INTEGER NOT NULL CHECK (day_of_week >= 0 AND day_of_week <= 6),
    time_slot INTEGER NOT NULL CHECK (time_slot >= 0 AND time_slot <= 47),
    avg_vehicle_count REAL DEFAULT 0,
    avg_queue_length REAL DEFAULT 0,
    avg_speed REAL DEFAULT 0,
    avg_flow_rate REAL DEFAULT 0,
    min_vehicle_count REAL DEFAULT 0,
    max_vehicle_count REAL DEFAULT 0,
    stddev_vehicle_count REAL DEFAULT 0,
    sample_count INTEGER DEFAULT 0,
    last_updated INTEGER NOT NULL,
    UNIQUE(road_id, day_of_week, time_slot)
);

-- Optimization history with prediction metadata
CREATE TABLE IF NOT EXISTS optimization_predictions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    optimization_run_id INTEGER NOT NULL,
    prediction_horizon_minutes INTEGER NOT NULL,
    predicted_at INTEGER NOT NULL,
    target_time INTEGER NOT NULL,
    confidence_score REAL DEFAULT 0,
    prediction_json TEXT,
    actual_metrics_json TEXT,
    prediction_error REAL,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (optimization_run_id) REFERENCES optimization_runs(id) ON DELETE CASCADE
);

-- Rollback events for analysis
CREATE TABLE IF NOT EXISTS rollback_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    optimization_run_id INTEGER NOT NULL,
    timestamp INTEGER NOT NULL,
    reason TEXT NOT NULL,
    metrics_before_json TEXT,
    metrics_after_json TEXT,
    regression_percent REAL,
    was_automatic INTEGER DEFAULT 0,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (optimization_run_id) REFERENCES optimization_runs(id) ON DELETE CASCADE
);

-- Indexes for efficient queries
CREATE INDEX IF NOT EXISTS idx_traffic_snapshots_timestamp ON traffic_snapshots(timestamp);
CREATE INDEX IF NOT EXISTS idx_traffic_snapshots_road ON traffic_snapshots(road_id);
CREATE INDEX IF NOT EXISTS idx_traffic_snapshots_road_time ON traffic_snapshots(road_id, timestamp);

CREATE INDEX IF NOT EXISTS idx_traffic_patterns_lookup ON traffic_patterns(day_of_week, time_slot);
CREATE INDEX IF NOT EXISTS idx_traffic_patterns_road ON traffic_patterns(road_id);
CREATE INDEX IF NOT EXISTS idx_traffic_patterns_road_day_slot ON traffic_patterns(road_id, day_of_week, time_slot);

CREATE INDEX IF NOT EXISTS idx_optimization_predictions_run ON optimization_predictions(optimization_run_id);
CREATE INDEX IF NOT EXISTS idx_optimization_predictions_time ON optimization_predictions(target_time);

CREATE INDEX IF NOT EXISTS idx_rollback_events_run ON rollback_events(optimization_run_id);
CREATE INDEX IF NOT EXISTS idx_rollback_events_time ON rollback_events(timestamp);
