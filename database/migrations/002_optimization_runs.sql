-- Migration 002: Optimization Runs Storage
-- Stores GA optimization run metadata, evolution history, and best solutions

-- Optimization runs table
CREATE TABLE IF NOT EXISTS optimization_runs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    network_id INTEGER,
    status TEXT NOT NULL CHECK(status IN ('pending', 'running', 'completed', 'failed')),

    -- GA parameters
    population_size INTEGER NOT NULL,
    generations INTEGER NOT NULL,
    mutation_rate REAL NOT NULL,
    crossover_rate REAL NOT NULL,
    elitism_rate REAL NOT NULL,

    -- Timing bounds
    min_green_time REAL NOT NULL,
    max_green_time REAL NOT NULL,
    min_red_time REAL NOT NULL,
    max_red_time REAL NOT NULL,

    -- Simulation parameters
    simulation_steps INTEGER NOT NULL,
    dt REAL NOT NULL,

    -- Results
    baseline_fitness REAL,
    best_fitness REAL,
    improvement_percent REAL,

    -- Timestamps
    started_at INTEGER NOT NULL,  -- Unix timestamp
    completed_at INTEGER,         -- Unix timestamp
    duration_seconds INTEGER,

    -- Metadata
    created_by TEXT DEFAULT 'web_dashboard',
    notes TEXT,

    FOREIGN KEY (network_id) REFERENCES networks(id) ON DELETE SET NULL
);

-- Evolution history table (fitness per generation)
CREATE TABLE IF NOT EXISTS optimization_generations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    optimization_run_id INTEGER NOT NULL,
    generation_number INTEGER NOT NULL,
    best_fitness REAL NOT NULL,
    average_fitness REAL,
    worst_fitness REAL,
    timestamp INTEGER NOT NULL,  -- Unix timestamp

    FOREIGN KEY (optimization_run_id) REFERENCES optimization_runs(id) ON DELETE CASCADE,
    UNIQUE(optimization_run_id, generation_number)
);

-- Best solutions table (chromosome data)
CREATE TABLE IF NOT EXISTS optimization_solutions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    optimization_run_id INTEGER NOT NULL,
    is_best_solution BOOLEAN NOT NULL DEFAULT 0,
    fitness REAL NOT NULL,

    -- Chromosome data (JSON format)
    -- Structure: [{"greenTime": 30.0, "redTime": 25.0}, ...]
    chromosome_json TEXT NOT NULL,

    -- Traffic light mapping
    traffic_light_count INTEGER NOT NULL,

    -- Timestamps
    created_at INTEGER NOT NULL,  -- Unix timestamp

    FOREIGN KEY (optimization_run_id) REFERENCES optimization_runs(id) ON DELETE CASCADE
);

-- Create indexes for faster queries
CREATE INDEX IF NOT EXISTS idx_optimization_runs_status ON optimization_runs(status);
CREATE INDEX IF NOT EXISTS idx_optimization_runs_network ON optimization_runs(network_id);
CREATE INDEX IF NOT EXISTS idx_optimization_runs_started ON optimization_runs(started_at DESC);
CREATE INDEX IF NOT EXISTS idx_optimization_generations_run ON optimization_generations(optimization_run_id);
CREATE INDEX IF NOT EXISTS idx_optimization_solutions_run ON optimization_solutions(optimization_run_id);
CREATE INDEX IF NOT EXISTS idx_optimization_solutions_best ON optimization_solutions(is_best_solution);
