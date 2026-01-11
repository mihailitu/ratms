import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, ReferenceLine } from 'recharts';

interface ConfidenceDataPoint {
  timestamp: number;
  confidence: number;
}

interface PredictionConfidenceChartProps {
  history: ConfidenceDataPoint[];
}

export default function PredictionConfidenceChart({ history }: PredictionConfidenceChartProps) {
  const formatTime = (timestamp: number) => {
    const date = new Date(timestamp * 1000);
    return date.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
  };

  const getConfidenceColor = (confidence: number) => {
    if (confidence >= 0.7) return '#22c55e';
    if (confidence >= 0.4) return '#eab308';
    return '#ef4444';
  };

  const latestConfidence = history.length > 0 ? history[history.length - 1].confidence : 0;

  return (
    <div className="bg-white rounded-lg shadow p-4">
      <div className="flex justify-between items-center mb-4">
        <h3 className="text-lg font-semibold text-gray-800">Prediction Confidence</h3>
        <div className="flex items-center gap-2">
          <span className="text-sm text-gray-500">Current:</span>
          <span
            className="text-lg font-bold"
            style={{ color: getConfidenceColor(latestConfidence) }}
          >
            {(latestConfidence * 100).toFixed(1)}%
          </span>
        </div>
      </div>

      {history.length === 0 ? (
        <div className="h-48 flex items-center justify-center text-gray-500 text-sm">
          No prediction data yet. Start continuous optimization with prediction enabled.
        </div>
      ) : (
        <ResponsiveContainer width="100%" height={200}>
          <LineChart data={history} margin={{ top: 5, right: 20, left: 0, bottom: 5 }}>
            <CartesianGrid strokeDasharray="3 3" stroke="#e5e7eb" />
            <XAxis
              dataKey="timestamp"
              tickFormatter={formatTime}
              tick={{ fontSize: 11, fill: '#6b7280' }}
              stroke="#9ca3af"
            />
            <YAxis
              domain={[0, 1]}
              tickFormatter={(v: number) => `${(v * 100).toFixed(0)}%`}
              tick={{ fontSize: 11, fill: '#6b7280' }}
              stroke="#9ca3af"
              width={45}
            />
            <Tooltip
              labelFormatter={(value: number) => `Time: ${formatTime(value)}`}
              formatter={(value: number) => [`${(value * 100).toFixed(1)}%`, 'Confidence']}
              contentStyle={{
                backgroundColor: 'white',
                border: '1px solid #e5e7eb',
                borderRadius: '0.375rem',
                fontSize: '12px',
              }}
            />
            <ReferenceLine y={0.7} stroke="#22c55e" strokeDasharray="5 5" label={{ value: 'Good', fontSize: 10, fill: '#22c55e' }} />
            <ReferenceLine y={0.4} stroke="#eab308" strokeDasharray="5 5" label={{ value: 'Fair', fontSize: 10, fill: '#eab308' }} />
            <Line
              type="monotone"
              dataKey="confidence"
              stroke="#3b82f6"
              strokeWidth={2}
              dot={false}
              activeDot={{ r: 4, fill: '#3b82f6' }}
            />
          </LineChart>
        </ResponsiveContainer>
      )}

      <div className="mt-2 flex justify-center gap-4 text-xs text-gray-500">
        <span className="flex items-center gap-1">
          <span className="w-2 h-2 rounded-full bg-green-500"></span> Good (&ge;70%)
        </span>
        <span className="flex items-center gap-1">
          <span className="w-2 h-2 rounded-full bg-yellow-500"></span> Fair (40-70%)
        </span>
        <span className="flex items-center gap-1">
          <span className="w-2 h-2 rounded-full bg-red-500"></span> Low (&lt;40%)
        </span>
      </div>
    </div>
  );
}
