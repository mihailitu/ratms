-- Migration 007: Traffic Feed Entries
-- Stores traffic feed data for ML training

CREATE TABLE IF NOT EXISTS traffic_feed_entries (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,                    -- Unix timestamp of the feed entry
    road_id INTEGER NOT NULL,                      -- Road ID
    expected_vehicle_count INTEGER NOT NULL,       -- Expected number of vehicles
    expected_avg_speed REAL,                       -- Expected average speed (m/s), NULL if unknown
    confidence REAL DEFAULT 1.0,                   -- Confidence level 0.0-1.0
    source TEXT NOT NULL,                          -- Source: "simulated", "external", "ml_predicted"
    created_at INTEGER                             -- When the record was created
);

-- Index for time-based queries (most common)
CREATE INDEX IF NOT EXISTS idx_feed_entries_timestamp
    ON traffic_feed_entries(timestamp);

-- Index for road-specific queries
CREATE INDEX IF NOT EXISTS idx_feed_entries_road_time
    ON traffic_feed_entries(road_id, timestamp);

-- Index for source filtering
CREATE INDEX IF NOT EXISTS idx_feed_entries_source
    ON traffic_feed_entries(source);
