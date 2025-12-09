-- Travel Time Metrics Schema
-- Tracks origin-destination pair travel times for performance analysis

-- Origin-Destination pairs for travel time tracking
CREATE TABLE IF NOT EXISTS od_pairs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    origin_road_id INTEGER NOT NULL,
    destination_road_id INTEGER NOT NULL,
    name TEXT,
    description TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    UNIQUE(origin_road_id, destination_road_id)
);

-- Individual travel time samples
CREATE TABLE IF NOT EXISTS travel_time_samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    od_pair_id INTEGER NOT NULL,
    vehicle_id INTEGER NOT NULL,
    travel_time_seconds REAL NOT NULL,
    start_time INTEGER NOT NULL,
    end_time INTEGER NOT NULL,
    sampled_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (od_pair_id) REFERENCES od_pairs(id) ON DELETE CASCADE
);

-- Aggregated travel time statistics per O-D pair
CREATE TABLE IF NOT EXISTS travel_time_stats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    od_pair_id INTEGER NOT NULL UNIQUE,
    avg_time REAL NOT NULL DEFAULT 0,
    min_time REAL NOT NULL DEFAULT 0,
    max_time REAL NOT NULL DEFAULT 0,
    p50_time REAL NOT NULL DEFAULT 0,
    p95_time REAL NOT NULL DEFAULT 0,
    sample_count INTEGER NOT NULL DEFAULT 0,
    last_updated INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (od_pair_id) REFERENCES od_pairs(id) ON DELETE CASCADE
);

-- Indexes for efficient queries
CREATE INDEX IF NOT EXISTS idx_travel_time_samples_od_pair ON travel_time_samples(od_pair_id);
CREATE INDEX IF NOT EXISTS idx_travel_time_samples_sampled_at ON travel_time_samples(sampled_at);
CREATE INDEX IF NOT EXISTS idx_od_pairs_origin ON od_pairs(origin_road_id);
CREATE INDEX IF NOT EXISTS idx_od_pairs_destination ON od_pairs(destination_road_id);
