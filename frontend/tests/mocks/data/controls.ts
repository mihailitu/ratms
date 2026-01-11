/**
 * Mock data for traffic light and spawn rate controls
 */

export const mockTrafficLights = [
  { roadId: 1, lane: 0, greenTime: 30, yellowTime: 3, redTime: 27, currentState: 'G' as const },
  { roadId: 1, lane: 1, greenTime: 30, yellowTime: 3, redTime: 27, currentState: 'G' as const },
  { roadId: 2, lane: 0, greenTime: 25, yellowTime: 3, redTime: 32, currentState: 'R' as const },
  { roadId: 3, lane: 0, greenTime: 20, yellowTime: 3, redTime: 37, currentState: 'Y' as const },
  { roadId: 3, lane: 1, greenTime: 20, yellowTime: 3, redTime: 37, currentState: 'Y' as const },
];

export const mockSpawnRates = [
  { roadId: 1, vehiclesPerMinute: 10 },
  { roadId: 2, vehiclesPerMinute: 5 },
];
