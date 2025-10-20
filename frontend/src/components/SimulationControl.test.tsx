import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import SimulationControl from './SimulationControl';
import { apiClient } from '../services/apiClient';
import type { SimulationStatus } from '../types/api';

// Mock the API client
vi.mock('../services/apiClient', () => ({
  apiClient: {
    startSimulation: vi.fn(),
    stopSimulation: vi.fn(),
  },
}));

describe('SimulationControl', () => {
  const mockOnStatusChange = vi.fn();

  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('Rendering', () => {
    it('should render start and stop buttons', () => {
      const status: SimulationStatus = {
        status: 'stopped',
        server_running: true,
        road_count: 4,
      };

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      expect(screen.getByText('Start Simulation')).toBeInTheDocument();
      expect(screen.getByText('Stop Simulation')).toBeInTheDocument();
    });

    it('should show status indicator when running', () => {
      const status: SimulationStatus = {
        status: 'running',
        server_running: true,
        road_count: 4,
      };

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      expect(screen.getByText('Running')).toBeInTheDocument();
    });

    it('should show stopped status when not running', () => {
      const status: SimulationStatus = {
        status: 'stopped',
        server_running: true,
        road_count: 4,
      };

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      expect(screen.getByText('Stopped')).toBeInTheDocument();
    });
  });

  describe('Button States', () => {
    it('should disable start button when simulation is running', () => {
      const status: SimulationStatus = {
        status: 'running',
        server_running: true,
        road_count: 4,
      };

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const startButton = screen.getByText('Start Simulation');
      expect(startButton).toBeDisabled();
    });

    it('should disable stop button when simulation is stopped', () => {
      const status: SimulationStatus = {
        status: 'stopped',
        server_running: true,
        road_count: 4,
      };

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const stopButton = screen.getByText('Stop Simulation');
      expect(stopButton).toBeDisabled();
    });

    it('should enable start button when simulation is stopped', () => {
      const status: SimulationStatus = {
        status: 'stopped',
        server_running: true,
        road_count: 4,
      };

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const startButton = screen.getByText('Start Simulation');
      expect(startButton).not.toBeDisabled();
    });

    it('should enable stop button when simulation is running', () => {
      const status: SimulationStatus = {
        status: 'running',
        server_running: true,
        road_count: 4,
      };

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const stopButton = screen.getByText('Stop Simulation');
      expect(stopButton).not.toBeDisabled();
    });
  });

  describe('Start Simulation', () => {
    it('should call startSimulation API when start button is clicked', async () => {
      const status: SimulationStatus = {
        status: 'stopped',
        server_running: true,
        road_count: 4,
      };

      vi.mocked(apiClient.startSimulation).mockResolvedValue({
        message: 'Simulation started',
        status: 'running',
      });

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const startButton = screen.getByText('Start Simulation');
      fireEvent.click(startButton);

      await waitFor(() => {
        expect(apiClient.startSimulation).toHaveBeenCalled();
      });
    });

    it('should call onStatusChange after successful start', async () => {
      const status: SimulationStatus = {
        status: 'stopped',
        server_running: true,
        road_count: 4,
      };

      vi.mocked(apiClient.startSimulation).mockResolvedValue({
        message: 'Simulation started',
        status: 'running',
      });

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const startButton = screen.getByText('Start Simulation');
      fireEvent.click(startButton);

      await waitFor(() => {
        expect(mockOnStatusChange).toHaveBeenCalled();
      });
    });

    it('should show loading state while starting', async () => {
      const status: SimulationStatus = {
        status: 'stopped',
        server_running: true,
        road_count: 4,
      };

      vi.mocked(apiClient.startSimulation).mockImplementation(
        () => new Promise(resolve => setTimeout(resolve, 100))
      );

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const startButton = screen.getByText('Start Simulation');
      fireEvent.click(startButton);

      await waitFor(() => {
        expect(screen.getByText('Starting...')).toBeInTheDocument();
      });
    });

    it('should show error message when start fails', async () => {
      const status: SimulationStatus = {
        status: 'stopped',
        server_running: true,
        road_count: 4,
      };

      vi.mocked(apiClient.startSimulation).mockRejectedValue(
        new Error('Failed to start simulation')
      );

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const startButton = screen.getByText('Start Simulation');
      fireEvent.click(startButton);

      await waitFor(() => {
        expect(screen.getByText('Failed to start simulation')).toBeInTheDocument();
      });
    });
  });

  describe('Stop Simulation', () => {
    it('should call stopSimulation API when stop button is clicked', async () => {
      const status: SimulationStatus = {
        status: 'running',
        server_running: true,
        road_count: 4,
      };

      vi.mocked(apiClient.stopSimulation).mockResolvedValue({
        message: 'Simulation stopped',
        status: 'stopped',
      });

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const stopButton = screen.getByText('Stop Simulation');
      fireEvent.click(stopButton);

      await waitFor(() => {
        expect(apiClient.stopSimulation).toHaveBeenCalled();
      });
    });

    it('should call onStatusChange after successful stop', async () => {
      const status: SimulationStatus = {
        status: 'running',
        server_running: true,
        road_count: 4,
      };

      vi.mocked(apiClient.stopSimulation).mockResolvedValue({
        message: 'Simulation stopped',
        status: 'stopped',
      });

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const stopButton = screen.getByText('Stop Simulation');
      fireEvent.click(stopButton);

      await waitFor(() => {
        expect(mockOnStatusChange).toHaveBeenCalled();
      });
    });

    it('should show loading state while stopping', async () => {
      const status: SimulationStatus = {
        status: 'running',
        server_running: true,
        road_count: 4,
      };

      vi.mocked(apiClient.stopSimulation).mockImplementation(
        () => new Promise(resolve => setTimeout(resolve, 100))
      );

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const stopButton = screen.getByText('Stop Simulation');
      fireEvent.click(stopButton);

      await waitFor(() => {
        expect(screen.getByText('Stopping...')).toBeInTheDocument();
      });
    });

    it('should show error message when stop fails', async () => {
      const status: SimulationStatus = {
        status: 'running',
        server_running: true,
        road_count: 4,
      };

      vi.mocked(apiClient.stopSimulation).mockRejectedValue(
        new Error('Failed to stop simulation')
      );

      render(<SimulationControl status={status} onStatusChange={mockOnStatusChange} />);

      const stopButton = screen.getByText('Stop Simulation');
      fireEvent.click(stopButton);

      await waitFor(() => {
        expect(screen.getByText('Failed to stop simulation')).toBeInTheDocument();
      });
    });
  });

  describe('Null Status', () => {
    it('should handle null status gracefully', () => {
      render(<SimulationControl status={null} onStatusChange={mockOnStatusChange} />);

      expect(screen.getByText('Start Simulation')).toBeInTheDocument();
      expect(screen.getByText('Stop Simulation')).toBeInTheDocument();
    });
  });
});
