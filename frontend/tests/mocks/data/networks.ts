import type { NetworkRecord, RoadGeometry } from '../../../src/types/api';

export const mockNetworks: NetworkRecord[] = [
  {
    id: 1,
    name: 'Test Network 10x10',
    description: 'A 10x10 grid test network',
    road_count: 200,
    intersection_count: 100,
    created_at: 1704067200,
  },
  {
    id: 2,
    name: 'Simple 4-Road Network',
    description: 'Simple 4 road intersection',
    road_count: 4,
    intersection_count: 1,
    created_at: 1704067200,
  },
];

export const mockRoads: RoadGeometry[] = [
  {
    id: 0,
    startLat: 48.1351,
    startLon: 11.5820,
    endLat: 48.1361,
    endLon: 11.5830,
    length: 500.0,
    lanes: 2,
    maxSpeed: 15.0,
  },
  {
    id: 1,
    startLat: 48.1361,
    startLon: 11.5830,
    endLat: 48.1371,
    endLon: 11.5840,
    length: 500.0,
    lanes: 2,
    maxSpeed: 15.0,
  },
  {
    id: 2,
    startLat: 48.1351,
    startLon: 11.5840,
    endLat: 48.1361,
    endLon: 11.5830,
    length: 500.0,
    lanes: 2,
    maxSpeed: 15.0,
  },
  {
    id: 3,
    startLat: 48.1371,
    startLon: 11.5820,
    endLat: 48.1361,
    endLon: 11.5830,
    length: 500.0,
    lanes: 2,
    maxSpeed: 15.0,
  },
];
