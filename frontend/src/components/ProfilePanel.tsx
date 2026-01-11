import { useState, useEffect, useRef } from 'react';
import { apiClient } from '../services/apiClient';
import type { TrafficProfile, ProfileImportExportFormat } from '../types/api';

interface ProfilePanelProps {
  isOpen: boolean;
  onToggle: () => void;
}

export default function ProfilePanel({ isOpen, onToggle }: ProfilePanelProps) {
  const [profiles, setProfiles] = useState<TrafficProfile[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  const [selectedProfile, setSelectedProfile] = useState<number | null>(null);

  // Create profile form
  const [showCreateForm, setShowCreateForm] = useState(false);
  const [newProfileName, setNewProfileName] = useState('');
  const [newProfileDesc, setNewProfileDesc] = useState('');

  // Capture form
  const [showCaptureForm, setShowCaptureForm] = useState(false);
  const [captureName, setCaptureName] = useState('');
  const [captureDesc, setCaptureDesc] = useState('');

  const fileInputRef = useRef<HTMLInputElement>(null);

  const fetchProfiles = async () => {
    try {
      setLoading(true);
      setError(null);
      const response = await apiClient.getProfiles();
      setProfiles(response.profiles);

      // Select active profile if any
      const active = response.profiles.find(p => p.isActive);
      if (active) {
        setSelectedProfile(active.id);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch profiles');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (isOpen) {
      fetchProfiles();
    }
  }, [isOpen]);

  const showSuccessMessage = (msg: string) => {
    setSuccess(msg);
    setTimeout(() => setSuccess(null), 3000);
  };

  const handleApplyProfile = async () => {
    if (!selectedProfile) return;

    try {
      setLoading(true);
      setError(null);
      await apiClient.applyProfile(selectedProfile);
      showSuccessMessage('Profile applied successfully');
      await fetchProfiles();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to apply profile');
    } finally {
      setLoading(false);
    }
  };

  const handleCreateProfile = async () => {
    if (!newProfileName.trim()) {
      setError('Profile name is required');
      return;
    }

    try {
      setLoading(true);
      setError(null);
      await apiClient.createProfile({
        name: newProfileName.trim(),
        description: newProfileDesc.trim(),
      });
      showSuccessMessage('Profile created');
      setNewProfileName('');
      setNewProfileDesc('');
      setShowCreateForm(false);
      await fetchProfiles();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to create profile');
    } finally {
      setLoading(false);
    }
  };

  const handleCaptureProfile = async () => {
    if (!captureName.trim()) {
      setError('Profile name is required');
      return;
    }

    try {
      setLoading(true);
      setError(null);
      await apiClient.captureCurrentAsProfile({
        name: captureName.trim(),
        description: captureDesc.trim() || 'Captured from current simulation state',
      });
      showSuccessMessage('Current state captured as profile');
      setCaptureName('');
      setCaptureDesc('');
      setShowCaptureForm(false);
      await fetchProfiles();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to capture profile');
    } finally {
      setLoading(false);
    }
  };

  const handleDeleteProfile = async () => {
    if (!selectedProfile) return;

    const profile = profiles.find(p => p.id === selectedProfile);
    if (!profile) return;

    if (!window.confirm(`Delete profile "${profile.name}"?`)) return;

    try {
      setLoading(true);
      setError(null);
      await apiClient.deleteProfile(selectedProfile);
      showSuccessMessage('Profile deleted');
      setSelectedProfile(null);
      await fetchProfiles();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to delete profile');
    } finally {
      setLoading(false);
    }
  };

  const handleExportProfile = async () => {
    if (!selectedProfile) return;

    try {
      setLoading(true);
      setError(null);
      const profileData = await apiClient.exportProfile(selectedProfile);

      const blob = new Blob([JSON.stringify(profileData, null, 2)], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `profile_${profileData.name}.json`;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);

      showSuccessMessage('Profile exported');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to export profile');
    } finally {
      setLoading(false);
    }
  };

  const handleImportClick = () => {
    fileInputRef.current?.click();
  };

  const handleFileImport = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    try {
      setLoading(true);
      setError(null);

      const text = await file.text();
      const profileData: ProfileImportExportFormat = JSON.parse(text);

      await apiClient.importProfile(profileData);
      showSuccessMessage(`Profile "${profileData.name}" imported`);
      await fetchProfiles();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to import profile');
    } finally {
      setLoading(false);
      // Reset file input
      if (fileInputRef.current) {
        fileInputRef.current.value = '';
      }
    }
  };

  const getSelectedProfileInfo = () => {
    if (!selectedProfile) return null;
    return profiles.find(p => p.id === selectedProfile);
  };

  return (
    <div className="absolute top-4 left-4 z-[1000] bg-white rounded-lg shadow-lg max-w-sm">
      <button
        onClick={onToggle}
        className="w-full px-4 py-2 flex items-center justify-between bg-indigo-600 text-white rounded-t-lg hover:bg-indigo-700"
      >
        <span className="font-medium">Traffic Profiles</span>
        <span className="text-lg">{isOpen ? '-' : '+'}</span>
      </button>

      {isOpen && (
        <div className="p-4 space-y-4 max-h-[70vh] overflow-y-auto">
          {error && (
            <div className="p-2 bg-red-100 text-red-700 rounded text-sm">
              {error}
            </div>
          )}

          {success && (
            <div className="p-2 bg-green-100 text-green-700 rounded text-sm">
              {success}
            </div>
          )}

          {loading && (
            <div className="text-center text-gray-500 text-sm">Loading...</div>
          )}

          {/* Profile Selector */}
          <div className="space-y-2">
            <label className="block text-sm font-medium text-gray-700">
              Select Profile
            </label>
            <select
              value={selectedProfile || ''}
              onChange={(e) => setSelectedProfile(e.target.value ? Number(e.target.value) : null)}
              className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-indigo-500 focus:border-indigo-500"
              disabled={loading}
            >
              <option value="">-- Select a profile --</option>
              {profiles.map((profile) => (
                <option key={profile.id} value={profile.id}>
                  {profile.name} {profile.isActive ? '(active)' : ''}
                </option>
              ))}
            </select>
          </div>

          {/* Selected Profile Info */}
          {getSelectedProfileInfo() && (
            <div className="p-3 bg-gray-50 rounded-md text-sm space-y-1">
              <div className="font-medium">{getSelectedProfileInfo()?.name}</div>
              <div className="text-gray-600">{getSelectedProfileInfo()?.description || 'No description'}</div>
              <div className="text-gray-500 text-xs">
                Traffic Lights: {getSelectedProfileInfo()?.trafficLightCount || 0} |
                Spawn Rates: {getSelectedProfileInfo()?.spawnRateCount || 0}
              </div>
            </div>
          )}

          {/* Action Buttons */}
          <div className="flex flex-wrap gap-2">
            <button
              onClick={handleApplyProfile}
              disabled={!selectedProfile || loading}
              className="px-3 py-1.5 bg-green-600 text-white rounded hover:bg-green-700 disabled:bg-gray-300 disabled:cursor-not-allowed text-sm"
            >
              Apply
            </button>
            <button
              onClick={handleExportProfile}
              disabled={!selectedProfile || loading}
              className="px-3 py-1.5 bg-blue-600 text-white rounded hover:bg-blue-700 disabled:bg-gray-300 disabled:cursor-not-allowed text-sm"
            >
              Export
            </button>
            <button
              onClick={handleDeleteProfile}
              disabled={!selectedProfile || loading}
              className="px-3 py-1.5 bg-red-600 text-white rounded hover:bg-red-700 disabled:bg-gray-300 disabled:cursor-not-allowed text-sm"
            >
              Delete
            </button>
          </div>

          <hr className="border-gray-200" />

          {/* Create/Capture/Import Buttons */}
          <div className="flex flex-wrap gap-2">
            <button
              onClick={() => setShowCreateForm(!showCreateForm)}
              className="px-3 py-1.5 bg-indigo-600 text-white rounded hover:bg-indigo-700 text-sm"
            >
              Create New
            </button>
            <button
              onClick={() => setShowCaptureForm(!showCaptureForm)}
              className="px-3 py-1.5 bg-purple-600 text-white rounded hover:bg-purple-700 text-sm"
            >
              Capture Current
            </button>
            <button
              onClick={handleImportClick}
              disabled={loading}
              className="px-3 py-1.5 bg-gray-600 text-white rounded hover:bg-gray-700 disabled:bg-gray-300 text-sm"
            >
              Import JSON
            </button>
            <input
              ref={fileInputRef}
              type="file"
              accept=".json"
              onChange={handleFileImport}
              className="hidden"
            />
          </div>

          {/* Create Form */}
          {showCreateForm && (
            <div className="p-3 bg-indigo-50 rounded-md space-y-2">
              <div className="font-medium text-sm text-indigo-700">Create New Profile</div>
              <input
                type="text"
                placeholder="Profile name"
                value={newProfileName}
                onChange={(e) => setNewProfileName(e.target.value)}
                className="w-full px-2 py-1 text-sm border border-gray-300 rounded"
              />
              <input
                type="text"
                placeholder="Description (optional)"
                value={newProfileDesc}
                onChange={(e) => setNewProfileDesc(e.target.value)}
                className="w-full px-2 py-1 text-sm border border-gray-300 rounded"
              />
              <div className="flex gap-2">
                <button
                  onClick={handleCreateProfile}
                  disabled={loading || !newProfileName.trim()}
                  className="px-2 py-1 bg-indigo-600 text-white rounded text-sm hover:bg-indigo-700 disabled:bg-gray-300"
                >
                  Create
                </button>
                <button
                  onClick={() => setShowCreateForm(false)}
                  className="px-2 py-1 bg-gray-400 text-white rounded text-sm hover:bg-gray-500"
                >
                  Cancel
                </button>
              </div>
            </div>
          )}

          {/* Capture Form */}
          {showCaptureForm && (
            <div className="p-3 bg-purple-50 rounded-md space-y-2">
              <div className="font-medium text-sm text-purple-700">Capture Current State</div>
              <p className="text-xs text-gray-600">
                Saves current traffic light timings as a new profile.
              </p>
              <input
                type="text"
                placeholder="Profile name"
                value={captureName}
                onChange={(e) => setCaptureName(e.target.value)}
                className="w-full px-2 py-1 text-sm border border-gray-300 rounded"
              />
              <input
                type="text"
                placeholder="Description (optional)"
                value={captureDesc}
                onChange={(e) => setCaptureDesc(e.target.value)}
                className="w-full px-2 py-1 text-sm border border-gray-300 rounded"
              />
              <div className="flex gap-2">
                <button
                  onClick={handleCaptureProfile}
                  disabled={loading || !captureName.trim()}
                  className="px-2 py-1 bg-purple-600 text-white rounded text-sm hover:bg-purple-700 disabled:bg-gray-300"
                >
                  Capture
                </button>
                <button
                  onClick={() => setShowCaptureForm(false)}
                  className="px-2 py-1 bg-gray-400 text-white rounded text-sm hover:bg-gray-500"
                >
                  Cancel
                </button>
              </div>
            </div>
          )}

          {/* Profile Count */}
          <div className="text-xs text-gray-500 text-center">
            {profiles.length} profile{profiles.length !== 1 ? 's' : ''} available
          </div>
        </div>
      )}
    </div>
  );
}
