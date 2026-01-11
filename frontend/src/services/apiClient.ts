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
  TrafficLightsResponse,
  SetTrafficLightsRequest,
  SetTrafficLightsResponse,
  SpawnRatesResponse,
  SetSpawnRatesRequest,
  SetSpawnRatesResponse,
  TrafficProfilesResponse,
  FlowRatesResponse,
  SpawningStatus,
  FlowRate,
  ContinuousOptimizationStatus,
  ContinuousOptimizationConfig,
  SystemHealth,
  SimulationConfig,
  PredictionResult,
  PredictionConfig,
  ValidationConfig,
  RolloutState,
  ProfilesListResponse,
  FullTrafficProfile,
  CreateProfileRequest,
  CaptureProfileRequest,
  ProfileImportExportFormat,
  ODPair,
  TravelTimeSample,
  TravelTimeStats,
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

  // Traffic Light Control (Stage 2)
  async getTrafficLights(): Promise<TrafficLightsResponse> {
    const response = await this.client.get<TrafficLightsResponse>('/api/traffic-lights');
    return response.data;
  }

  async setTrafficLights(request: SetTrafficLightsRequest): Promise<SetTrafficLightsResponse> {
    const response = await this.client.post<SetTrafficLightsResponse>('/api/traffic-lights', request);
    return response.data;
  }

  // Spawn Rate Control (Stage 3)
  async getSpawnRates(): Promise<SpawnRatesResponse> {
    const response = await this.client.get<SpawnRatesResponse>('/api/spawn-rates');
    return response.data;
  }

  async setSpawnRates(request: SetSpawnRatesRequest): Promise<SetSpawnRatesResponse> {
    const response = await this.client.post<SetSpawnRatesResponse>('/api/spawn-rates', request);
    return response.data;
  }

  // Production System - Continuous Simulation
  async startContinuousSimulation(): Promise<SimulationStartResponse> {
    const response = await this.client.post<SimulationStartResponse>('/api/simulation/continuous', {});
    return response.data;
  }

  async getSystemHealth(): Promise<SystemHealth> {
    const response = await this.client.get<SystemHealth>('/api/health');
    return response.data;
  }

  // Simulation Control - Pause/Resume (Stage 4)
  async pauseSimulation(): Promise<{ success: boolean; message: string; step: number; time: number }> {
    const response = await this.client.post('/api/simulation/pause', {});
    return response.data;
  }

  async resumeSimulation(): Promise<{ success: boolean; message: string; step: number; time: number }> {
    const response = await this.client.post('/api/simulation/resume', {});
    return response.data;
  }

  async getSimulationConfig(): Promise<SimulationConfig> {
    const response = await this.client.get<SimulationConfig>('/api/simulation/config');
    return response.data;
  }

  async setSimulationConfig(config: Partial<SimulationConfig>): Promise<{ success: boolean; stepLimit: number; continuousMode: boolean }> {
    const response = await this.client.post('/api/simulation/config', config);
    return response.data;
  }

  // Traffic Profiles and Flow Rates
  async getTrafficProfiles(): Promise<TrafficProfilesResponse> {
    const response = await this.client.get<TrafficProfilesResponse>('/api/traffic/profiles');
    return response.data;
  }

  async setActiveProfile(profileName: string): Promise<{ success: boolean; data: { message: string; activeProfile: string } }> {
    const response = await this.client.post('/api/traffic/profiles/active', { profile: profileName });
    return response.data;
  }

  async getFlowRates(): Promise<FlowRatesResponse> {
    const response = await this.client.get<FlowRatesResponse>('/api/traffic/flow-rates');
    return response.data;
  }

  async setFlowRates(flowRates: FlowRate[]): Promise<{ success: boolean; data: { message: string; count: number } }> {
    const response = await this.client.post('/api/traffic/flow-rates', { flowRates });
    return response.data;
  }

  async startSpawning(): Promise<{ success: boolean; data: { message: string; activeProfile: string } }> {
    const response = await this.client.post('/api/traffic/spawning/start', {});
    return response.data;
  }

  async stopSpawning(): Promise<{ success: boolean; data: { message: string } }> {
    const response = await this.client.post('/api/traffic/spawning/stop', {});
    return response.data;
  }

  async getSpawningStatus(): Promise<{ success: boolean; data: SpawningStatus }> {
    const response = await this.client.get('/api/traffic/spawning/status');
    return response.data;
  }

  // Continuous Optimization
  async startContinuousOptimization(config?: Partial<ContinuousOptimizationConfig>): Promise<{ success: boolean; data: { message: string; config: ContinuousOptimizationConfig } }> {
    const response = await this.client.post('/api/optimization/continuous/start', config || {});
    return response.data;
  }

  async stopContinuousOptimization(): Promise<{ success: boolean; data: { message: string } }> {
    const response = await this.client.post('/api/optimization/continuous/stop', {});
    return response.data;
  }

  async getContinuousOptimizationStatus(): Promise<{ success: boolean; data: ContinuousOptimizationStatus }> {
    const response = await this.client.get('/api/optimization/continuous/status');
    return response.data;
  }

  async getContinuousOptimizationConfig(): Promise<{ success: boolean; data: ContinuousOptimizationConfig }> {
    const response = await this.client.get('/api/optimization/continuous/config');
    return response.data;
  }

  async setContinuousOptimizationConfig(config: Partial<ContinuousOptimizationConfig>): Promise<{ success: boolean; data: { message: string } }> {
    const response = await this.client.post('/api/optimization/continuous/config', config);
    return response.data;
  }

  async applyOptimizationRun(runId: number): Promise<{ success: boolean; data: { message: string; runId: number; transitionDurationSeconds: number } }> {
    const response = await this.client.post(`/api/optimization/apply/${runId}`, {});
    return response.data;
  }

  // Prediction API (Phase 5)
  async getPredictionCurrent(): Promise<{ success: boolean; data: PredictionResult }> {
    const response = await this.client.get('/api/prediction/current');
    return response.data;
  }

  async getPredictionForecast(horizonMinutes: number): Promise<{ success: boolean; data: PredictionResult }> {
    const response = await this.client.get(`/api/prediction/forecast?horizon=${horizonMinutes}`);
    return response.data;
  }

  async getPredictionConfig(): Promise<{ success: boolean; data: PredictionConfig }> {
    const response = await this.client.get('/api/prediction/config');
    return response.data;
  }

  async setPredictionConfig(config: Partial<PredictionConfig>): Promise<{ success: boolean; data: { message: string } }> {
    const response = await this.client.post('/api/prediction/config', config);
    return response.data;
  }

  // Validation API (Phase 5)
  async getValidationConfig(): Promise<{ success: boolean; data: ValidationConfig }> {
    const response = await this.client.get('/api/optimization/validation/config');
    return response.data;
  }

  async setValidationConfig(config: Partial<ValidationConfig>): Promise<{ success: boolean; data: { message: string } }> {
    const response = await this.client.post('/api/optimization/validation/config', config);
    return response.data;
  }

  // Rollout API (Phase 5)
  async getRolloutStatus(): Promise<{ success: boolean; data: RolloutState }> {
    const response = await this.client.get('/api/optimization/rollout/status');
    return response.data;
  }

  async rollback(): Promise<{ success: boolean; data: { message: string; transitionDurationSeconds: number } }> {
    const response = await this.client.post('/api/optimization/rollback', {});
    return response.data;
  }

  // Traffic Profiles API (Stage 5)
  async getProfiles(): Promise<ProfilesListResponse> {
    const response = await this.client.get<ProfilesListResponse>('/api/profiles');
    return response.data;
  }

  async getProfile(nameOrId: string | number): Promise<FullTrafficProfile> {
    const response = await this.client.get<FullTrafficProfile>(`/api/profiles/${nameOrId}`);
    return response.data;
  }

  async createProfile(profile: CreateProfileRequest): Promise<{ success: boolean; id: number; name: string }> {
    const response = await this.client.post('/api/profiles', profile);
    return response.data;
  }

  async updateProfile(nameOrId: string | number, profile: Partial<CreateProfileRequest>): Promise<{ success: boolean; id: number; name: string }> {
    const response = await this.client.put(`/api/profiles/${nameOrId}`, profile);
    return response.data;
  }

  async deleteProfile(nameOrId: string | number): Promise<{ success: boolean; message: string }> {
    const response = await this.client.delete(`/api/profiles/${nameOrId}`);
    return response.data;
  }

  async applyProfile(nameOrId: string | number): Promise<{ success: boolean; message: string }> {
    const response = await this.client.post(`/api/profiles/${nameOrId}/apply`, {});
    return response.data;
  }

  async captureCurrentAsProfile(request: CaptureProfileRequest): Promise<{ success: boolean; id: number; name: string; message: string }> {
    const response = await this.client.post('/api/profiles/capture', request);
    return response.data;
  }

  async exportProfile(nameOrId: string | number): Promise<ProfileImportExportFormat> {
    const response = await this.client.get<ProfileImportExportFormat>(`/api/profiles/${nameOrId}/export`);
    return response.data;
  }

  async importProfile(profile: ProfileImportExportFormat): Promise<{ success: boolean; id: number; name: string; message: string }> {
    const response = await this.client.post('/api/profiles/import', profile);
    return response.data;
  }

  // Travel Time API (Stage 8)
  async getODPairs(): Promise<{ success: boolean; odPairs: ODPair[]; count: number }> {
    const response = await this.client.get('/api/travel-time/od-pairs');
    return response.data;
  }

  async createODPair(originRoadId: number, destinationRoadId: number, name?: string, description?: string): Promise<{ success: boolean; id: number; message: string }> {
    const response = await this.client.post('/api/travel-time/od-pairs', {
      originRoadId,
      destinationRoadId,
      name: name || '',
      description: description || ''
    });
    return response.data;
  }

  async deleteODPair(odPairId: number): Promise<{ success: boolean; message: string }> {
    const response = await this.client.delete(`/api/travel-time/od-pairs/${odPairId}`);
    return response.data;
  }

  async getAllTravelTimeStats(): Promise<{ success: boolean; stats: TravelTimeStats[]; count: number }> {
    const response = await this.client.get('/api/travel-time/stats');
    return response.data;
  }

  async getTravelTimeStats(odPairId: number): Promise<{ success: boolean; stats: TravelTimeStats }> {
    const response = await this.client.get(`/api/travel-time/stats/${odPairId}`);
    return response.data;
  }

  async getTravelTimeSamples(odPairId: number, limit?: number): Promise<{ success: boolean; samples: TravelTimeSample[]; count: number }> {
    const url = limit
      ? `/api/travel-time/samples/${odPairId}?limit=${limit}`
      : `/api/travel-time/samples/${odPairId}`;
    const response = await this.client.get(url);
    return response.data;
  }

  async getTrackedVehicles(): Promise<{ success: boolean; vehicles: Array<{ vehicleId: number; odPairId: number; originRoadId: number; destinationRoadId: number; startTime: number }>; count: number }> {
    const response = await this.client.get('/api/travel-time/tracked');
    return response.data;
  }
}

export const apiClient = new ApiClient();
export default apiClient;
export { ApiClient };
