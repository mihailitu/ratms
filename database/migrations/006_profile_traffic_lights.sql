-- Migration: 006_profile_traffic_lights.sql
-- Add traffic light timings to traffic profiles

CREATE TABLE IF NOT EXISTS profile_traffic_lights (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    profile_id INTEGER NOT NULL,
    road_id INTEGER NOT NULL,
    lane INTEGER NOT NULL,
    green_time REAL NOT NULL,
    yellow_time REAL DEFAULT 3.0,
    red_time REAL NOT NULL,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (profile_id) REFERENCES traffic_profiles(id) ON DELETE CASCADE,
    UNIQUE(profile_id, road_id, lane)
);

CREATE INDEX IF NOT EXISTS idx_profile_traffic_lights_profile ON profile_traffic_lights(profile_id);
CREATE INDEX IF NOT EXISTS idx_profile_traffic_lights_road ON profile_traffic_lights(road_id, lane);
