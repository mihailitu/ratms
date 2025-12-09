import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { renderHook, waitFor } from '@testing-library/react';
import { useSimulationStream } from './useSimulationStream';
import type { SimulationUpdate } from '../types/api';

// Mock EventSource
class MockEventSource {
  url: string;
  onopen: ((event: Event) => void) | null = null;
  onerror: ((event: Event) => void) | null = null;
  close = vi.fn();
  addEventListener = vi.fn();
  removeEventListener = vi.fn();
  dispatchEvent = vi.fn();
  readyState = 0;
  CONNECTING = 0;
  OPEN = 1;
  CLOSED = 2;

  constructor(url: string) {
    this.url = url;
    // Simulate successful connection
    setTimeout(() => {
      this.readyState = 1;
      if (this.onopen) {
        this.onopen(new Event('open'));
      }
    }, 0);
  }
}

describe('useSimulationStream', () => {
  let mockEventSource: MockEventSource;

  beforeEach(() => {
    // Replace global EventSource with mock
    (global as any).EventSource = vi.fn((url: string) => {
      mockEventSource = new MockEventSource(url);
      return mockEventSource;
    });
  });

  afterEach(() => {
    vi.clearAllMocks();
  });

  it('should initialize with default values', () => {
    const { result } = renderHook(() => useSimulationStream(false));

    expect(result.current.latestUpdate).toBeNull();
    expect(result.current.isConnected).toBe(false);
    expect(result.current.error).toBeNull();
  });

  it('should create EventSource when enabled', async () => {
    renderHook(() => useSimulationStream(true));

    await waitFor(() => {
      expect(global.EventSource).toHaveBeenCalledWith('http://localhost:8080/api/simulation/stream');
    });
  });

  it('should set isConnected to true on connection', async () => {
    const { result } = renderHook(() => useSimulationStream(true));

    await waitFor(() => {
      expect(result.current.isConnected).toBe(true);
    });
  });

  it('should handle simulation updates', async () => {
    const { result } = renderHook(() => useSimulationStream(true));

    await waitFor(() => {
      expect(mockEventSource).toBeDefined();
    });

    // Simulate receiving an update event
    const mockUpdate: SimulationUpdate = {
      step: 100,
      time: 10.0,
      vehicles: [
        {
          id: 1,
          roadId: 0,
          lane: 0,
          position: 150.0,
          velocity: 15.0,
          acceleration: 0.5,
        },
      ],
      trafficLights: [
        {
          roadId: 0,
          lane: 0,
          state: 'G',
          lat: 48.135,
          lon: 11.582,
        },
      ],
    };

    const updateCallback = mockEventSource.addEventListener.mock.calls.find(
      call => call[0] === 'update'
    )?.[1];

    if (updateCallback) {
      updateCallback({ data: JSON.stringify(mockUpdate) } as any);
    }

    await waitFor(() => {
      expect(result.current.latestUpdate).toEqual(mockUpdate);
    });
  });

  it('should handle connection errors', async () => {
    const { result } = renderHook(() => useSimulationStream(true));

    await waitFor(() => {
      expect(mockEventSource).toBeDefined();
    });

    // Trigger error
    if (mockEventSource.onerror) {
      mockEventSource.onerror(new Event('error'));
    }

    await waitFor(() => {
      expect(result.current.error).toBe('Connection error');
      expect(result.current.isConnected).toBe(false);
    });
  });

  it('should close connection when disabled', async () => {
    const { result, rerender } = renderHook(
      ({ enabled }) => useSimulationStream(enabled),
      { initialProps: { enabled: true } }
    );

    await waitFor(() => {
      expect(result.current.isConnected).toBe(true);
    });

    // Disable the stream
    rerender({ enabled: false });

    await waitFor(() => {
      expect(mockEventSource.close).toHaveBeenCalled();
      expect(result.current.isConnected).toBe(false);
    });
  });

  it('should clean up on unmount', async () => {
    const { unmount } = renderHook(() => useSimulationStream(true));

    await waitFor(() => {
      expect(mockEventSource).toBeDefined();
    });

    unmount();

    expect(mockEventSource.close).toHaveBeenCalled();
  });

  it('should handle malformed JSON in updates', async () => {
    const { result } = renderHook(() => useSimulationStream(true));

    await waitFor(() => {
      expect(mockEventSource).toBeDefined();
    });

    const updateCallback = mockEventSource.addEventListener.mock.calls.find(
      call => call[0] === 'update'
    )?.[1];

    if (updateCallback) {
      updateCallback({ data: 'invalid json' } as any);
    }

    await waitFor(() => {
      expect(result.current.error).toBe('Failed to parse update');
    });
  });

  it('should handle status events', async () => {
    renderHook(() => useSimulationStream(true));

    await waitFor(() => {
      expect(mockEventSource).toBeDefined();
    });

    const statusCallback = mockEventSource.addEventListener.mock.calls.find(
      call => call[0] === 'status'
    )?.[1];

    if (statusCallback) {
      statusCallback({ data: JSON.stringify({ status: 'stopped' }) } as any);
    }

    await waitFor(() => {
      expect(mockEventSource.close).toHaveBeenCalled();
    });
  });
});
