import { useState, useEffect } from 'react';
import { apiClient } from '../services/apiClient';
import type { ValidationConfig } from '../types/api';

interface ValidationConfigPanelProps {
  onConfigChange?: () => void;
}

export default function ValidationConfigPanel({ onConfigChange }: ValidationConfigPanelProps) {
  const [config, setConfig] = useState<ValidationConfig | null>(null);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  const [formData, setFormData] = useState({
    enabled: true,
    improvementThreshold: 5.0,
    regressionThreshold: 10.0,
  });

  useEffect(() => {
    fetchConfig();
  }, []);

  const fetchConfig = async () => {
    try {
      const response = await apiClient.getValidationConfig();
      if (response.success && response.data) {
        setConfig(response.data);
        setFormData({
          enabled: response.data.enabled,
          improvementThreshold: response.data.improvementThreshold,
          regressionThreshold: response.data.regressionThreshold,
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
      await apiClient.setValidationConfig(formData);
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
      <h3 className="text-lg font-semibold text-gray-800 mb-4">Validation Configuration</h3>

      {error && (
        <div className="mb-3 text-sm text-red-600 bg-red-50 p-2 rounded">{error}</div>
      )}

      {success && (
        <div className="mb-3 text-sm text-green-600 bg-green-50 p-2 rounded">{success}</div>
      )}

      <div className="space-y-4">
        <div className="flex items-center justify-between">
          <div>
            <label className="text-sm font-medium text-gray-700">Validation Enabled</label>
            <p className="text-xs text-gray-500">Validate timings before applying</p>
          </div>
          <button
            onClick={() => setFormData({ ...formData, enabled: !formData.enabled })}
            className={`relative inline-flex h-6 w-11 items-center rounded-full transition-colors ${
              formData.enabled ? 'bg-blue-600' : 'bg-gray-300'
            }`}
          >
            <span
              className={`inline-block h-4 w-4 transform rounded-full bg-white transition-transform ${
                formData.enabled ? 'translate-x-6' : 'translate-x-1'
              }`}
            />
          </button>
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Improvement Threshold (%)
          </label>
          <div className="flex items-center gap-3">
            <input
              type="range"
              min={0}
              max={20}
              step={0.5}
              value={formData.improvementThreshold}
              onChange={(e) => setFormData({ ...formData, improvementThreshold: parseFloat(e.target.value) })}
              className="flex-1 h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer"
              disabled={!formData.enabled}
            />
            <span className="w-14 text-right font-medium text-gray-700">
              {formData.improvementThreshold.toFixed(1)}%
            </span>
          </div>
          <p className="text-xs text-gray-500 mt-1">
            Minimum required improvement to pass validation
          </p>
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Regression Threshold (%)
          </label>
          <div className="flex items-center gap-3">
            <input
              type="range"
              min={0}
              max={50}
              step={1}
              value={formData.regressionThreshold}
              onChange={(e) => setFormData({ ...formData, regressionThreshold: parseFloat(e.target.value) })}
              className="flex-1 h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer"
              disabled={!formData.enabled}
            />
            <span className="w-14 text-right font-medium text-gray-700">
              {formData.regressionThreshold.toFixed(0)}%
            </span>
          </div>
          <p className="text-xs text-gray-500 mt-1">
            Maximum acceptable regression before rejection
          </p>
        </div>

        {config && (
          <div className="pt-3 border-t border-gray-200">
            <div className="text-xs text-gray-500 space-y-1">
              <div className="flex justify-between">
                <span>Simulation Steps:</span>
                <span>{config.simulationSteps}</span>
              </div>
              <div className="flex justify-between">
                <span>Time Step (dt):</span>
                <span>{config.dt}s</span>
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
