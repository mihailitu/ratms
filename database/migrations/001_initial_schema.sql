-- RATMS Database Schema
-- Version: 1.0
-- Description: Initial schema for simulation results, metrics, and configurations

-- Simulations table: stores metadata about simulation runs
CREATE TABLE IF NOT EXISTS simulations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT,
    description TEXT,
    network_id INTEGER,
    status TEXT CHECK(status IN ('pending', 'running', 'completed', 'failed')),
    start_time INTEGER,
    end_time INTEGER,
    duration_seconds REAL,
    config_json TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (network_id) REFERENCES networks(id)
);

-- Networks table: stores road network configurations
CREATE TABLE IF NOT EXISTS networks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    road_count INTEGER,
    intersection_count INTEGER,
    config_json TEXT NOT NULL,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Metrics table: time-series simulation metrics
CREATE TABLE IF NOT EXISTS metrics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    simulation_id INTEGER NOT NULL,
    timestamp REAL NOT NULL,
    metric_type TEXT NOT NULL,
    road_id INTEGER,
    value REAL NOT NULL,
    unit TEXT,
    metadata_json TEXT,
    recorded_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (simulation_id) REFERENCES simulations(id) ON DELETE CASCADE
);

-- Roads table: individual road segments
CREATE TABLE IF NOT EXISTS roads (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    network_id INTEGER NOT NULL,
    road_id INTEGER NOT NULL,
    name TEXT,
    length REAL NOT NULL,
    lanes INTEGER NOT NULL,
    max_speed REAL NOT NULL,
    start_x REAL,
    start_y REAL,
    end_x REAL,
    end_y REAL,
    start_lat REAL,
    start_lon REAL,
    end_lat REAL,
    end_lon REAL,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (network_id) REFERENCES networks(id) ON DELETE CASCADE,
    UNIQUE(network_id, road_id)
);

-- Traffic lights table: traffic light configurations
CREATE TABLE IF NOT EXISTS traffic_lights (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    road_id INTEGER NOT NULL,
    lane INTEGER NOT NULL,
    green_time REAL NOT NULL,
    yellow_time REAL NOT NULL,
    red_time REAL NOT NULL,
    initial_state TEXT CHECK(initial_state IN ('red', 'yellow', 'green')),
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (road_id) REFERENCES roads(id) ON DELETE CASCADE
);

-- Optimization results table: GA optimization runs
CREATE TABLE IF NOT EXISTS optimization_results (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    simulation_id INTEGER,
    algorithm TEXT NOT NULL,
    population_size INTEGER,
    generations INTEGER,
    fitness_value REAL,
    parameters_json TEXT NOT NULL,
    convergence_data_json TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (simulation_id) REFERENCES simulations(id) ON DELETE CASCADE
);

-- Indexes for common queries
CREATE INDEX IF NOT EXISTS idx_metrics_simulation ON metrics(simulation_id);
CREATE INDEX IF NOT EXISTS idx_metrics_timestamp ON metrics(timestamp);
CREATE INDEX IF NOT EXISTS idx_metrics_type ON metrics(metric_type);
CREATE INDEX IF NOT EXISTS idx_simulations_status ON simulations(status);
CREATE INDEX IF NOT EXISTS idx_simulations_created ON simulations(created_at);
CREATE INDEX IF NOT EXISTS idx_roads_network ON roads(network_id);
