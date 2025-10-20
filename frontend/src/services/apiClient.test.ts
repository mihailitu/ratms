import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import type {
  HealthResponse,
  SimulationStatus,
  SimulationStartResponse,
  SimulationStopResponse,
  NetworkRecord,
  SimulationRecord,
  MetricRecord,
} from '../types/api';

// Create a properly typed mock axios instance
const mockAxiosInstance = {
  get: vi.fn(),
  post: vi.fn(),
  interceptors: {
    response: {
      use: vi.fn((onSuccess: any, onError: any) => {
        return 0;
      }),
    },
  },
};

// Mock the axios module
vi.mock('axios', () => ({
  default: {
    create: vi.fn(() => mockAxiosInstance),
  },
}));

// Import ApiClient after mocking axios
const { ApiClient } = await import('./apiClient');

describe('ApiClient', () => {
  let apiClient: InstanceType<typeof ApiClient>;

  beforeEach(() => {
    // Create a new ApiClient instance
    apiClient = new ApiClient();
    vi.clearAllMocks();
  });

  afterEach(() => {
    vi.clearAllMocks();
  });

  describe('Health Check', () => {
    it('should fetch health status successfully', async () => {
      const mockResponse: HealthResponse = {
        service: 'RATMS API Server',
        status: 'healthy',
        timestamp: 1234567890,
        version: '0.1.0',
      };

      mockAxiosInstance.get.mockResolvedValue({ data: mockResponse });

      const result = await apiClient.getHealth();

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/health');
      expect(result).toEqual(mockResponse);
    });

    it('should handle health check errors', async () => {
      const error = new Error('Network error');
      mockAxiosInstance.get.mockRejectedValue(error);

      await expect(apiClient.getHealth()).rejects.toThrow('Network error');
    });
  });

  describe('Simulation Control', () => {
    it('should get simulation status', async () => {
      const mockStatus: SimulationStatus = {
        status: 'running',
        server_running: true,
        road_count: 4,
      };

      mockAxiosInstance.get.mockResolvedValue({ data: mockStatus });

      const result = await apiClient.getSimulationStatus();

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/simulation/status');
      expect(result).toEqual(mockStatus);
    });

    it('should start simulation', async () => {
      const mockResponse: SimulationStartResponse = {
        message: 'Simulation started',
        status: 'running',
      };

      mockAxiosInstance.post.mockResolvedValue({ data: mockResponse });

      const result = await apiClient.startSimulation();

      expect(mockAxiosInstance.post).toHaveBeenCalledWith('/api/simulation/start');
      expect(result).toEqual(mockResponse);
    });

    it('should stop simulation', async () => {
      const mockResponse: SimulationStopResponse = {
        message: 'Simulation stopped',
        status: 'stopped',
      };

      mockAxiosInstance.post.mockResolvedValue({ data: mockResponse });

      const result = await apiClient.stopSimulation();

      expect(mockAxiosInstance.post).toHaveBeenCalledWith('/api/simulation/stop');
      expect(result).toEqual(mockResponse);
    });

    it('should handle start simulation errors', async () => {
      const error = new Error('Simulation already running');
      mockAxiosInstance.post.mockRejectedValue(error);

      await expect(apiClient.startSimulation()).rejects.toThrow('Simulation already running');
    });
  });

  describe('Simulations', () => {
    it('should fetch simulations list', async () => {
      const mockSimulations: SimulationRecord[] = [
        {
          id: 1,
          name: 'Test Simulation',
          description: 'Test',
          status: 'completed',
          network_id: 1,
          created_at: 1234567890,
          started_at: 1234567890,
          completed_at: 1234567900,
          config: '{}',
        },
      ];

      mockAxiosInstance.get.mockResolvedValue({ data: mockSimulations });

      const result = await apiClient.getSimulations();

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/simulations');
      expect(result).toEqual(mockSimulations);
    });

    it('should fetch single simulation by id', async () => {
      const mockSimulation: SimulationRecord = {
        id: 1,
        name: 'Test Simulation',
        description: 'Test',
        status: 'completed',
        network_id: 1,
        created_at: 1234567890,
        started_at: 1234567890,
        completed_at: 1234567900,
        config: '{}',
      };

      mockAxiosInstance.get.mockResolvedValue({ data: mockSimulation });

      const result = await apiClient.getSimulation(1);

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/simulations/1');
      expect(result).toEqual(mockSimulation);
    });

    it('should fetch metrics for simulation', async () => {
      const mockMetrics: MetricRecord[] = [
        {
          id: 1,
          simulation_id: 1,
          timestamp: 1234567890,
          avg_speed: 15.5,
          avg_queue_length: 2.3,
          total_vehicles: 10,
          vehicles_exited: 5,
        },
      ];

      mockAxiosInstance.get.mockResolvedValue({ data: mockMetrics });

      const result = await apiClient.getMetrics(1);

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/simulations/1/metrics');
      expect(result).toEqual(mockMetrics);
    });
  });

  describe('Networks', () => {
    it('should fetch networks list', async () => {
      const mockNetworks: NetworkRecord[] = [
        {
          id: 1,
          name: 'Test Network',
          description: 'Test network',
          road_count: 4,
          intersection_count: 1,
          created_at: 1234567890,
        },
      ];

      mockAxiosInstance.get.mockResolvedValue({ data: mockNetworks });

      const result = await apiClient.getNetworks();

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/networks');
      expect(result).toEqual(mockNetworks);
    });
  });

  describe('Optimization', () => {
    it('should start optimization', async () => {
      const params = {
        network_id: 1,
        population_size: 50,
        generations: 100,
        mutation_rate: 0.1,
      };

      const mockResponse = {
        run_id: 1,
        message: 'Optimization started',
        status: 'running',
      };

      mockAxiosInstance.post.mockResolvedValue({ data: mockResponse });

      const result = await apiClient.startOptimization(params);

      expect(mockAxiosInstance.post).toHaveBeenCalledWith('/api/optimization/start', params);
      expect(result).toEqual(mockResponse);
    });

    it('should get optimization status', async () => {
      const mockStatus = {
        run_id: 1,
        status: 'running',
        current_generation: 50,
        total_generations: 100,
        best_fitness: 15.5,
      };

      mockAxiosInstance.get.mockResolvedValue({ data: mockStatus });

      const result = await apiClient.getOptimizationStatus(1);

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/optimization/status/1');
      expect(result).toEqual(mockStatus);
    });

    it('should get optimization results', async () => {
      const mockResults = {
        run_id: 1,
        status: 'completed',
        baseline_fitness: 20.0,
        best_fitness: 15.0,
        improvement: 25.0,
        best_solution: [],
        fitness_history: [],
      };

      mockAxiosInstance.get.mockResolvedValue({ data: mockResults });

      const result = await apiClient.getOptimizationResults(1);

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/optimization/results/1');
      expect(result).toEqual(mockResults);
    });

    it('should get optimization history', async () => {
      const mockHistory = {
        runs: [],
      };

      mockAxiosInstance.get.mockResolvedValue({ data: mockHistory });

      const result = await apiClient.getOptimizationHistory();

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/optimization/history');
      expect(result).toEqual(mockHistory);
    });

    it('should stop optimization', async () => {
      const mockResponse = {
        message: 'Optimization stopped',
        status: 'stopped',
      };

      mockAxiosInstance.post.mockResolvedValue({ data: mockResponse });

      const result = await apiClient.stopOptimization(1);

      expect(mockAxiosInstance.post).toHaveBeenCalledWith('/api/optimization/stop/1');
      expect(result).toEqual(mockResponse);
    });
  });

  describe('Road Geometry', () => {
    it('should fetch roads', async () => {
      const mockRoads = {
        count: 2,
        roads: [
          {
            id: 0,
            startLat: 48.135,
            startLon: 11.582,
            endLat: 48.136,
            endLon: 11.583,
            length: 400,
            lanes: 1,
            maxSpeed: 20,
          },
        ],
      };

      mockAxiosInstance.get.mockResolvedValue({ data: mockRoads });

      const result = await apiClient.getRoads();

      expect(mockAxiosInstance.get).toHaveBeenCalledWith('/api/simulation/roads');
      expect(result).toEqual(mockRoads);
    });
  });

  describe('Error Handling', () => {
    it('should handle network errors gracefully', async () => {
      const networkError = new Error('Network Error');
      mockAxiosInstance.get.mockRejectedValue(networkError);

      await expect(apiClient.getHealth()).rejects.toThrow('Network Error');
    });

    it('should handle timeout errors', async () => {
      const timeoutError = new Error('timeout of 10000ms exceeded');
      mockAxiosInstance.get.mockRejectedValue(timeoutError);

      await expect(apiClient.getSimulationStatus()).rejects.toThrow('timeout');
    });
  });
});
