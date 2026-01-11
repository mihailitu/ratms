interface PipelineStatusIndicatorProps {
  status: string;
}

const PIPELINE_STAGES = [
  { key: 'idle', label: 'Idle' },
  { key: 'predicting', label: 'Predicting' },
  { key: 'optimizing', label: 'Optimizing' },
  { key: 'validating', label: 'Validating' },
  { key: 'applying', label: 'Applying' },
  { key: 'complete', label: 'Complete' },
];

export default function PipelineStatusIndicator({ status }: PipelineStatusIndicatorProps) {
  const normalizedStatus = status?.toLowerCase() || 'idle';
  const isError = normalizedStatus === 'error';

  const currentIndex = PIPELINE_STAGES.findIndex(s => s.key === normalizedStatus);

  const getStageColor = (index: number, stageKey: string) => {
    if (isError) {
      return 'bg-red-500';
    }
    if (stageKey === normalizedStatus) {
      if (stageKey === 'complete') return 'bg-green-500';
      return 'bg-blue-500 animate-pulse';
    }
    if (currentIndex > index) {
      return 'bg-green-500';
    }
    return 'bg-gray-300';
  };

  const getConnectorColor = (index: number) => {
    if (isError) return 'bg-red-300';
    if (currentIndex > index) return 'bg-green-400';
    return 'bg-gray-200';
  };

  return (
    <div className="bg-white rounded-lg shadow p-4">
      <h3 className="text-sm font-medium text-gray-700 mb-3">Pipeline Status</h3>

      {isError ? (
        <div className="flex items-center gap-2 text-red-600 mb-3">
          <svg className="w-5 h-5" fill="currentColor" viewBox="0 0 20 20">
            <path fillRule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7 4a1 1 0 11-2 0 1 1 0 012 0zm-1-9a1 1 0 00-1 1v4a1 1 0 102 0V6a1 1 0 00-1-1z" clipRule="evenodd" />
          </svg>
          <span className="font-medium">Error occurred</span>
        </div>
      ) : null}

      <div className="flex items-center justify-between">
        {PIPELINE_STAGES.map((stage, index) => (
          <div key={stage.key} className="flex items-center">
            <div className="flex flex-col items-center">
              <div
                className={`w-4 h-4 rounded-full ${getStageColor(index, stage.key)} transition-colors duration-300`}
              />
              <span className={`text-xs mt-1 ${
                stage.key === normalizedStatus
                  ? 'text-blue-600 font-medium'
                  : currentIndex > index
                    ? 'text-green-600'
                    : 'text-gray-500'
              }`}>
                {stage.label}
              </span>
            </div>
            {index < PIPELINE_STAGES.length - 1 && (
              <div className={`w-8 h-0.5 mx-1 ${getConnectorColor(index)} transition-colors duration-300`} />
            )}
          </div>
        ))}
      </div>
    </div>
  );
}
