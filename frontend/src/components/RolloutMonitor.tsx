import type { RolloutState } from '../types/api';

interface RolloutMonitorProps {
  rolloutState: RolloutState;
  onRollback: () => void;
  isRollingBack: boolean;
}

export default function RolloutMonitor({ rolloutState, onRollback, isRollingBack }: RolloutMonitorProps) {
  const { status, preRollout, postRollout, regressionPercent, updateCount } = rolloutState;

  const getStatusBadge = () => {
    switch (status) {
      case 'in_progress':
        return <span className="px-2 py-1 text-xs font-medium bg-blue-100 text-blue-800 rounded">In Progress</span>;
      case 'complete':
        return <span className="px-2 py-1 text-xs font-medium bg-green-100 text-green-800 rounded">Complete</span>;
      case 'rolled_back':
        return <span className="px-2 py-1 text-xs font-medium bg-yellow-100 text-yellow-800 rounded">Rolled Back</span>;
      default:
        return <span className="px-2 py-1 text-xs font-medium bg-gray-100 text-gray-800 rounded">Idle</span>;
    }
  };

  const formatChange = (before: number, after: number, lowerIsBetter: boolean = false) => {
    if (before === 0 && after === 0) return '-';
    const change = ((after - before) / Math.max(before, 0.01)) * 100;
    const isImprovement = lowerIsBetter ? change < 0 : change > 0;
    const color = isImprovement ? 'text-green-600' : change === 0 ? 'text-gray-500' : 'text-red-600';
    const sign = change > 0 ? '+' : '';
    return <span className={color}>{sign}{change.toFixed(1)}%</span>;
  };

  const getRegressionColor = () => {
    if (regressionPercent <= -5) return 'text-green-600';
    if (regressionPercent <= 0) return 'text-green-500';
    if (regressionPercent <= 5) return 'text-yellow-600';
    return 'text-red-600';
  };

  return (
    <div className="bg-white rounded-lg shadow p-4">
      <div className="flex justify-between items-center mb-4">
        <h3 className="text-lg font-semibold text-gray-800">Rollout Monitor</h3>
        {getStatusBadge()}
      </div>

      {status === 'idle' ? (
        <p className="text-gray-500 text-sm">No active rollout. Start continuous optimization to see rollout metrics.</p>
      ) : (
        <>
          <div className="overflow-x-auto">
            <table className="min-w-full divide-y divide-gray-200">
              <thead>
                <tr>
                  <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Metric</th>
                  <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Before</th>
                  <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">After</th>
                  <th className="px-3 py-2 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Change</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-gray-200">
                <tr>
                  <td className="px-3 py-2 text-sm text-gray-700">Avg Speed</td>
                  <td className="px-3 py-2 text-sm text-gray-900">{preRollout.avgSpeed.toFixed(2)} m/s</td>
                  <td className="px-3 py-2 text-sm text-gray-900">{postRollout.avgSpeed.toFixed(2)} m/s</td>
                  <td className="px-3 py-2 text-sm">{formatChange(preRollout.avgSpeed, postRollout.avgSpeed, false)}</td>
                </tr>
                <tr>
                  <td className="px-3 py-2 text-sm text-gray-700">Avg Queue</td>
                  <td className="px-3 py-2 text-sm text-gray-900">{preRollout.avgQueue.toFixed(2)}</td>
                  <td className="px-3 py-2 text-sm text-gray-900">{postRollout.avgQueue.toFixed(2)}</td>
                  <td className="px-3 py-2 text-sm">{formatChange(preRollout.avgQueue, postRollout.avgQueue, true)}</td>
                </tr>
                <tr>
                  <td className="px-3 py-2 text-sm text-gray-700">Fitness</td>
                  <td className="px-3 py-2 text-sm text-gray-900">{preRollout.fitness.toFixed(2)}</td>
                  <td className="px-3 py-2 text-sm text-gray-900">{postRollout.fitness.toFixed(2)}</td>
                  <td className="px-3 py-2 text-sm">{formatChange(preRollout.fitness, postRollout.fitness, true)}</td>
                </tr>
              </tbody>
            </table>
          </div>

          <div className="mt-4 flex items-center justify-between">
            <div className="text-sm">
              <span className="text-gray-500">Regression: </span>
              <span className={`font-medium ${getRegressionColor()}`}>
                {regressionPercent > 0 ? '+' : ''}{regressionPercent.toFixed(1)}%
              </span>
              <span className="text-gray-400 ml-2">({updateCount} updates)</span>
            </div>

            {status === 'in_progress' && rolloutState.hasPreviousChromosome && (
              <button
                onClick={onRollback}
                disabled={isRollingBack}
                className={`px-3 py-1.5 text-sm font-medium rounded ${
                  isRollingBack
                    ? 'bg-gray-300 text-gray-500 cursor-not-allowed'
                    : 'bg-red-100 text-red-700 hover:bg-red-200'
                }`}
              >
                {isRollingBack ? 'Rolling back...' : 'Rollback'}
              </button>
            )}
          </div>
        </>
      )}
    </div>
  );
}
