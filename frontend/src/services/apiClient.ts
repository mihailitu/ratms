import axios, { type AxiosInstance } from 'axios';
import type {
  HealthResponse,
  SimulationStatus,
  SimulationStartResponse,
  SimulationStopResponse,
  SimulationRecord,
  MetricRecord,
  NetworkRecord,
  StartOptimizationRequest,
  StartOptimizationResponse,
  OptimizationStatusResponse,
  OptimizationResultsResponse,
  OptimizationHistoryResponse,
  StopOptimizationResponse,
  SimulationStatistics,
  ComparisonResponse,
  MetricTypesResponse,
} from '../types/api';

class ApiClient {
  private client: AxiosInstance;

  constructor(baseURL: string = 'http://localhost:8080') {
    this.client = axios.create({
      baseURL,
      timeout: 10000,
      headers: {
        'Content-Type': 'application/json',
      },
    });

    // Add response interceptor for error handling
    this.client.interceptors.response.use(
      (response) => response,
      (error) => {
        console.error('API Error:', error.response?.data || error.message);
        return Promise.reject(error);
      }
    );
  }

  // Health check
  async getHealth(): Promise<HealthResponse> {
    const response = await this.client.get<HealthResponse>('/api/health');
    return response.data;
  }

  // Simulation control
  async getSimulationStatus(): Promise<SimulationStatus> {
    const response = await this.client.get<SimulationStatus>('/api/simulation/status');
    return response.data;
  }

  async startSimulation(): Promise<SimulationStartResponse> {
    // Send empty body to ensure Content-Length is set (cpp-httplib requires it)
    const response = await this.client.post<SimulationStartResponse>('/api/simulation/start', {});
    return response.data;
  }

  async stopSimulation(): Promise<SimulationStopResponse> {
    // Send empty body to ensure Content-Length is set (cpp-httplib requires it)
    const response = await this.client.post<SimulationStopResponse>('/api/simulation/stop', {});
    return response.data;
  }

  // Simulations
  async getSimulations(): Promise<SimulationRecord[]> {
    const response = await this.client.get<SimulationRecord[]>('/api/simulations');
    return response.data;
  }

  async getSimulation(id: number): Promise<SimulationRecord> {
    const response = await this.client.get<SimulationRecord>(`/api/simulations/${id}`);
    return response.data;
  }

  async getMetrics(simulationId: number): Promise<MetricRecord[]> {
    const response = await this.client.get<MetricRecord[]>(`/api/simulations/${simulationId}/metrics`);
    return response.data;
  }

  // Networks
  async getNetworks(): Promise<NetworkRecord[]> {
    const response = await this.client.get<NetworkRecord[]>('/api/networks');
    return response.data;
  }

  // Optimization (Genetic Algorithm)
  async startOptimization(params: StartOptimizationRequest): Promise<StartOptimizationResponse> {
    const response = await this.client.post<StartOptimizationResponse>('/api/optimization/start', params);
    return response.data;
  }

  async getOptimizationStatus(runId: number): Promise<OptimizationStatusResponse> {
    const response = await this.client.get<OptimizationStatusResponse>(`/api/optimization/status/${runId}`);
    return response.data;
  }

  async getOptimizationResults(runId: number): Promise<OptimizationResultsResponse> {
    const response = await this.client.get<OptimizationResultsResponse>(`/api/optimization/results/${runId}`);
    return response.data;
  }

  async getOptimizationHistory(): Promise<OptimizationHistoryResponse> {
    const response = await this.client.get<OptimizationHistoryResponse>('/api/optimization/history');
    return response.data;
  }

  async stopOptimization(runId: number): Promise<StopOptimizationResponse> {
    // Send empty body to ensure Content-Length is set (cpp-httplib requires it)
    const response = await this.client.post<StopOptimizationResponse>(`/api/optimization/stop/${runId}`, {});
    return response.data;
  }

  // Road geometry
  async getRoads(): Promise<import('../types/api').RoadsResponse> {
    const response = await this.client.get<import('../types/api').RoadsResponse>('/api/simulation/roads');
    return response.data;
  }

  // Analytics
  async getSimulationStatistics(simulationId: number, metricType?: string): Promise<SimulationStatistics> {
    const url = metricType
      ? `/api/analytics/simulations/${simulationId}/statistics/${metricType}`
      : `/api/analytics/simulations/${simulationId}/statistics`;
    const response = await this.client.get<SimulationStatistics>(url);
    return response.data;
  }

  async compareSimulations(simulationIds: number[], metricType: string): Promise<ComparisonResponse> {
    const response = await this.client.post<ComparisonResponse>('/api/analytics/compare', {
      simulation_ids: simulationIds,
      metric_type: metricType,
    });
    return response.data;
  }

  async exportMetrics(simulationId: number, metricType?: string): Promise<Blob> {
    const url = metricType
      ? `/api/analytics/simulations/${simulationId}/export?metric_type=${metricType}`
      : `/api/analytics/simulations/${simulationId}/export`;
    const response = await this.client.get(url, {
      responseType: 'blob',
    });
    return response.data;
  }

  async getMetricTypes(): Promise<MetricTypesResponse> {
    const response = await this.client.get<MetricTypesResponse>('/api/analytics/metric-types');
    return response.data;
  }
}

export const apiClient = new ApiClient();
export default apiClient;
export { ApiClient };
