import type { OptimizationRun } from '../../../src/types/api';

export const mockCompletedOptimizationRun: OptimizationRun = {
  id: 1,
  startedAt: 1704067200,
  status: 'completed',
  parameters: {
    populationSize: 30,
    generations: 50,
    mutationRate: 0.15,
    mutationStdDev: 5.0,
    crossoverRate: 0.8,
    tournamentSize: 3,
    elitismRate: 0.1,
    minGreenTime: 10.0,
    maxGreenTime: 60.0,
    minRedTime: 10.0,
    maxRedTime: 60.0,
    simulationSteps: 1000,
    dt: 0.1,
    networkId: 1,
  },
  progress: {
    currentGeneration: 50,
    totalGenerations: 50,
    percentComplete: 100,
  },
  results: {
    baselineFitness: 16.08,
    bestFitness: 10.00,
    improvementPercent: 37.8,
    durationSeconds: 125,
    // Flat array of best fitness values per generation (what GAVisualizer expects)
    fitnessHistory: Array.from({ length: 50 }, (_, i) => 16.08 - (i * 0.12)),
    // Best chromosome (array of traffic light timings)
    bestChromosome: [
      { greenTime: 30.5, redTime: 25.2 },
      { greenTime: 35.1, redTime: 20.8 },
      { greenTime: 28.9, redTime: 26.4 },
      { greenTime: 32.3, redTime: 23.1 },
    ],
  },
};

export const mockRunningOptimizationRun: OptimizationRun = {
  id: 2,
  startedAt: Math.floor(Date.now() / 1000) - 300, // Started 5 mins ago
  status: 'running',
  parameters: {
    populationSize: 50,
    generations: 100,
    mutationRate: 0.15,
    mutationStdDev: 5.0,
    crossoverRate: 0.8,
    tournamentSize: 3,
    elitismRate: 0.1,
    minGreenTime: 10.0,
    maxGreenTime: 60.0,
    minRedTime: 10.0,
    maxRedTime: 60.0,
    simulationSteps: 1000,
    dt: 0.1,
    networkId: 1,
  },
  progress: {
    currentGeneration: 45,
    totalGenerations: 100,
    percentComplete: 45,
  },
};

export const mockOptimizationRuns: OptimizationRun[] = [
  mockCompletedOptimizationRun,
  mockRunningOptimizationRun,
  {
    id: 3,
    startedAt: 1704070800,
    status: 'failed',
    parameters: {
      populationSize: 20,
      generations: 30,
      mutationRate: 0.15,
      mutationStdDev: 5.0,
      crossoverRate: 0.8,
      tournamentSize: 3,
      elitismRate: 0.1,
      minGreenTime: 10.0,
      maxGreenTime: 60.0,
      minRedTime: 10.0,
      maxRedTime: 60.0,
      simulationSteps: 500,
      dt: 0.1,
      networkId: 1,
    },
    progress: {
      currentGeneration: 15,
      totalGenerations: 30,
      percentComplete: 50,
    },
  },
];
