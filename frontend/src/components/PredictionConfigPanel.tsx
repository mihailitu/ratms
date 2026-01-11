import { useState, useEffect } from 'react';
import { apiClient } from '../services/apiClient';
import type { PredictionConfig } from '../types/api';

interface PredictionConfigPanelProps {
  onConfigChange?: () => void;
}

export default function PredictionConfigPanel({ onConfigChange }: PredictionConfigPanelProps) {
  const [config, setConfig] = useState<PredictionConfig | null>(null);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  const [formData, setFormData] = useState({
    horizonMinutes: 30,
    patternWeight: 0.7,
    currentWeight: 0.3,
  });

  useEffect(() => {
    fetchConfig();
  }, []);

  const fetchConfig = async () => {
    try {
      const response = await apiClient.getPredictionConfig();
      if (response.success && response.data) {
        setConfig(response.data);
        setFormData({
          horizonMinutes: response.data.horizonMinutes,
          patternWeight: response.data.patternWeight,
          currentWeight: response.data.currentWeight,
        });
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch config');
    } finally {
      setLoading(false);
    }
  };

  const handleSave = async () => {
    setSaving(true);
    setError(null);
    setSuccess(null);

    try {
      await apiClient.setPredictionConfig(formData);
      setSuccess('Configuration saved');
      await fetchConfig();
      onConfigChange?.();
      setTimeout(() => setSuccess(null), 3000);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to save config');
    } finally {
      setSaving(false);
    }
  };

  const handlePatternWeightChange = (value: number) => {
    setFormData({
      ...formData,
      patternWeight: value,
      currentWeight: parseFloat((1 - value).toFixed(2)),
    });
  };

  if (loading) {
    return (
      <div className="bg-white rounded-lg shadow p-4">
        <div className="animate-pulse space-y-3">
          <div className="h-5 bg-gray-200 rounded w-1/3"></div>
          <div className="h-8 bg-gray-200 rounded"></div>
        </div>
      </div>
    );
  }

  return (
    <div className="bg-white rounded-lg shadow p-4">
      <h3 className="text-lg font-semibold text-gray-800 mb-4">Prediction Configuration</h3>

      {error && (
        <div className="mb-3 text-sm text-red-600 bg-red-50 p-2 rounded">{error}</div>
      )}

      {success && (
        <div className="mb-3 text-sm text-green-600 bg-green-50 p-2 rounded">{success}</div>
      )}

      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Prediction Horizon (minutes)
          </label>
          <div className="flex items-center gap-3">
            <input
              type="range"
              min={config?.minHorizonMinutes || 10}
              max={config?.maxHorizonMinutes || 120}
              step={5}
              value={formData.horizonMinutes}
              onChange={(e) => setFormData({ ...formData, horizonMinutes: parseInt(e.target.value) })}
              className="flex-1 h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer"
            />
            <span className="w-16 text-right font-medium text-gray-700">
              {formData.horizonMinutes} min
            </span>
          </div>
          <p className="text-xs text-gray-500 mt-1">
            How far ahead to predict traffic conditions
          </p>
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Pattern Weight vs Current State
          </label>
          <div className="flex items-center gap-3">
            <span className="text-xs text-gray-500 w-20">Historical</span>
            <input
              type="range"
              min={0}
              max={1}
              step={0.05}
              value={formData.patternWeight}
              onChange={(e) => handlePatternWeightChange(parseFloat(e.target.value))}
              className="flex-1 h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer"
            />
            <span className="text-xs text-gray-500 w-20 text-right">Current</span>
          </div>
          <div className="flex justify-between text-xs text-gray-600 mt-1">
            <span>Pattern: {(formData.patternWeight * 100).toFixed(0)}%</span>
            <span>Current: {(formData.currentWeight * 100).toFixed(0)}%</span>
          </div>
          <p className="text-xs text-gray-500 mt-1">
            Balance between historical patterns and current simulation state
          </p>
        </div>

        {config && (
          <div className="pt-3 border-t border-gray-200">
            <div className="text-xs text-gray-500 space-y-1">
              <div className="flex justify-between">
                <span>Cache Duration:</span>
                <span>{config.cacheDurationSeconds}s</span>
              </div>
              <div className="flex justify-between">
                <span>Min Samples for Full Confidence:</span>
                <span>{config.minSamplesForFullConfidence}</span>
              </div>
            </div>
          </div>
        )}

        <button
          onClick={handleSave}
          disabled={saving}
          className="w-full bg-blue-600 hover:bg-blue-700 text-white py-2 px-4 rounded-md font-medium transition-colors disabled:bg-gray-400 disabled:cursor-not-allowed"
        >
          {saving ? 'Saving...' : 'Save Configuration'}
        </button>
      </div>
    </div>
  );
}
