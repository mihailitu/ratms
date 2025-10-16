import { useEffect, useState, useRef } from 'react';
import type { SimulationUpdate } from '../types/api';

export function useSimulationStream(enabled: boolean = true) {
  const [latestUpdate, setLatestUpdate] = useState<SimulationUpdate | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const eventSourceRef = useRef<EventSource | null>(null);

  useEffect(() => {
    if (!enabled) {
      // Clean up existing connection
      if (eventSourceRef.current) {
        eventSourceRef.current.close();
        eventSourceRef.current = null;
        setIsConnected(false);
      }
      return;
    }

    // Create EventSource connection
    const eventSource = new EventSource('http://localhost:8080/api/simulation/stream');
    eventSourceRef.current = eventSource;

    eventSource.onopen = () => {
      console.log('Connected to simulation stream');
      setIsConnected(true);
      setError(null);
    };

    eventSource.addEventListener('update', (event) => {
      try {
        const data: SimulationUpdate = JSON.parse(event.data);
        setLatestUpdate(data);
      } catch (err) {
        console.error('Failed to parse simulation update:', err);
        setError('Failed to parse update');
      }
    });

    eventSource.addEventListener('status', (event) => {
      try {
        const status = JSON.parse(event.data);
        if (status.status === 'stopped') {
          console.log('Simulation stopped, closing stream');
          eventSource.close();
          setIsConnected(false);
        }
      } catch (err) {
        console.error('Failed to parse status:', err);
      }
    });

    eventSource.onerror = (err) => {
      console.error('EventSource error:', err);
      setError('Connection error');
      setIsConnected(false);
      eventSource.close();
    };

    // Cleanup on unmount
    return () => {
      if (eventSource) {
        eventSource.close();
      }
    };
  }, [enabled]);

  return {
    latestUpdate,
    isConnected,
    error,
  };
}
