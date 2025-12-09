-- Traffic profiles and flow rates for production system
-- Phase 3: Traffic Data Ingestion

CREATE TABLE IF NOT EXISTS traffic_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    description TEXT,
    is_active BOOLEAN DEFAULT 0,
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);

CREATE TABLE IF NOT EXISTS road_flow_rates (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    profile_id INTEGER NOT NULL,
    road_id INTEGER NOT NULL,
    lane INTEGER NOT NULL,
    vehicles_per_minute REAL NOT NULL,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (profile_id) REFERENCES traffic_profiles(id) ON DELETE CASCADE
);

-- Index for efficient lookups
CREATE INDEX IF NOT EXISTS idx_flow_rates_profile ON road_flow_rates(profile_id);
CREATE INDEX IF NOT EXISTS idx_flow_rates_road ON road_flow_rates(road_id, lane);

-- Default traffic profiles
INSERT OR IGNORE INTO traffic_profiles (name, description, is_active) VALUES
    ('morning_rush', 'Morning rush hour (7am-9am)', 0),
    ('evening_rush', 'Evening rush hour (5pm-7pm)', 0),
    ('off_peak', 'Off-peak hours', 1);
