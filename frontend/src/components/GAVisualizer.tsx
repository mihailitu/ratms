import type { OptimizationRun } from '../types/api';

interface GAVisualizerProps {
  run: OptimizationRun;
}

export default function GAVisualizer({ run }: GAVisualizerProps) {
  if (!run.results || run.status !== 'completed') {
    return null;
  }

  const { results } = run;
  const fitnessData = results.fitnessHistory || results.fitnessHistorySample || [];

  // Calculate chart dimensions and scaling
  const chartWidth = 600;
  const chartHeight = 300;
  const padding = { top: 20, right: 20, bottom: 40, left: 60 };
  const plotWidth = chartWidth - padding.left - padding.right;
  const plotHeight = chartHeight - padding.top - padding.bottom;

  // Find min/max for scaling
  const minFitness = Math.min(...fitnessData);
  const maxFitness = Math.max(...fitnessData);
  const fitnessRange = maxFitness - minFitness;
  const generations = fitnessData.length;

  // Scale functions
  const scaleX = (generation: number) => {
    return padding.left + (generation / (generations - 1)) * plotWidth;
  };

  const scaleY = (fitness: number) => {
    return padding.top + plotHeight - ((fitness - minFitness) / fitnessRange) * plotHeight;
  };

  // Create SVG path for the fitness line
  const linePath = fitnessData.map((fitness, i) => {
    const x = scaleX(i);
    const y = scaleY(fitness);
    return i === 0 ? `M ${x} ${y}` : `L ${x} ${y}`;
  }).join(' ');

  // Y-axis ticks
  const yTicks = 5;
  const yTickValues = Array.from({ length: yTicks }, (_, i) => {
    return minFitness + (fitnessRange / (yTicks - 1)) * i;
  });

  // X-axis ticks
  const xTicks = Math.min(10, generations);
  const xTickValues = Array.from({ length: xTicks }, (_, i) => {
    return Math.floor((generations - 1) * (i / (xTicks - 1)));
  });

  return (
    <div className="space-y-6">
      {/* Fitness Evolution Chart */}
      <div>
        <h3 className="text-sm font-semibold text-gray-700 mb-3">Fitness Evolution</h3>
        <div className="border border-gray-200 rounded-lg p-4 bg-gray-50">
          <svg width={chartWidth} height={chartHeight} className="mx-auto">
            {/* Grid lines */}
            {yTickValues.map((value, i) => (
              <line
                key={`grid-y-${i}`}
                x1={padding.left}
                y1={scaleY(value)}
                x2={chartWidth - padding.right}
                y2={scaleY(value)}
                stroke="#e5e7eb"
                strokeWidth="1"
              />
            ))}

            {/* Y-axis */}
            <line
              x1={padding.left}
              y1={padding.top}
              x2={padding.left}
              y2={chartHeight - padding.bottom}
              stroke="#374151"
              strokeWidth="2"
            />

            {/* X-axis */}
            <line
              x1={padding.left}
              y1={chartHeight - padding.bottom}
              x2={chartWidth - padding.right}
              y2={chartHeight - padding.bottom}
              stroke="#374151"
              strokeWidth="2"
            />

            {/* Y-axis labels */}
            {yTickValues.map((value, i) => (
              <text
                key={`label-y-${i}`}
                x={padding.left - 10}
                y={scaleY(value)}
                textAnchor="end"
                alignmentBaseline="middle"
                fontSize="12"
                fill="#6b7280"
              >
                {value.toFixed(1)}
              </text>
            ))}

            {/* X-axis labels */}
            {xTickValues.map((generation, i) => (
              <text
                key={`label-x-${i}`}
                x={scaleX(generation)}
                y={chartHeight - padding.bottom + 20}
                textAnchor="middle"
                fontSize="12"
                fill="#6b7280"
              >
                {generation}
              </text>
            ))}

            {/* Axis labels */}
            <text
              x={chartWidth / 2}
              y={chartHeight - 5}
              textAnchor="middle"
              fontSize="14"
              fill="#374151"
              fontWeight="500"
            >
              Generation
            </text>
            <text
              x={15}
              y={chartHeight / 2}
              textAnchor="middle"
              fontSize="14"
              fill="#374151"
              fontWeight="500"
              transform={`rotate(-90, 15, ${chartHeight / 2})`}
            >
              Fitness
            </text>

            {/* Fitness line */}
            <path
              d={linePath}
              fill="none"
              stroke="#2563eb"
              strokeWidth="2"
            />

            {/* Data points */}
            {fitnessData.map((fitness, i) => (
              <circle
                key={`point-${i}`}
                cx={scaleX(i)}
                cy={scaleY(fitness)}
                r="3"
                fill="#2563eb"
              />
            ))}

            {/* Best fitness indicator */}
            <circle
              cx={scaleX(fitnessData.length - 1)}
              cy={scaleY(results.bestFitness)}
              r="5"
              fill="#22c55e"
              stroke="#fff"
              strokeWidth="2"
            />
          </svg>

          <div className="mt-2 text-xs text-gray-600 text-center">
            {fitnessData.length < run.parameters.generations && (
              <span className="text-yellow-600">
                Showing sample of {fitnessData.length} generations
              </span>
            )}
          </div>
        </div>
      </div>

      {/* Best Chromosome (Traffic Light Timings) */}
      <div>
        <h3 className="text-sm font-semibold text-gray-700 mb-3">Optimized Traffic Light Timings</h3>
        <div className="border border-gray-200 rounded-lg overflow-hidden">
          <table className="min-w-full divide-y divide-gray-200">
            <thead className="bg-gray-50">
              <tr>
                <th className="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase">
                  Light #
                </th>
                <th className="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase">
                  Green Time (s)
                </th>
                <th className="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase">
                  Red Time (s)
                </th>
                <th className="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase">
                  Cycle (s)
                </th>
                <th className="px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase">
                  Visual
                </th>
              </tr>
            </thead>
            <tbody className="bg-white divide-y divide-gray-200">
              {results.bestChromosome.map((gene, index) => {
                const cycleTime = gene.greenTime + gene.redTime;
                const greenPercent = (gene.greenTime / cycleTime) * 100;
                return (
                  <tr key={index} className="hover:bg-gray-50">
                    <td className="px-4 py-3 text-sm font-medium text-gray-900">
                      {index + 1}
                    </td>
                    <td className="px-4 py-3 text-sm text-gray-700">
                      {gene.greenTime.toFixed(1)}
                    </td>
                    <td className="px-4 py-3 text-sm text-gray-700">
                      {gene.redTime.toFixed(1)}
                    </td>
                    <td className="px-4 py-3 text-sm text-gray-700">
                      {cycleTime.toFixed(1)}
                    </td>
                    <td className="px-4 py-3">
                      <div className="flex items-center gap-1">
                        <div
                          className="h-4 bg-green-500 rounded-l"
                          style={{ width: `${greenPercent}%` }}
                          title={`Green: ${greenPercent.toFixed(0)}%`}
                        />
                        <div
                          className="h-4 bg-red-500 rounded-r"
                          style={{ width: `${100 - greenPercent}%` }}
                          title={`Red: ${(100 - greenPercent).toFixed(0)}%`}
                        />
                      </div>
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>

        <div className="mt-3 text-xs text-gray-600">
          <div className="flex items-center gap-4">
            <div className="flex items-center gap-1">
              <div className="w-3 h-3 bg-green-500 rounded"></div>
              <span>Green Time</span>
            </div>
            <div className="flex items-center gap-1">
              <div className="w-3 h-3 bg-red-500 rounded"></div>
              <span>Red Time</span>
            </div>
          </div>
        </div>
      </div>

      {/* Statistics Summary */}
      <div>
        <h3 className="text-sm font-semibold text-gray-700 mb-3">Statistics</h3>
        <div className="grid grid-cols-2 gap-4">
          <div className="border border-gray-200 rounded-lg p-3">
            <div className="text-xs text-gray-500 mb-1">Total Traffic Lights</div>
            <div className="text-xl font-bold text-gray-900">{results.bestChromosome.length}</div>
          </div>
          <div className="border border-gray-200 rounded-lg p-3">
            <div className="text-xs text-gray-500 mb-1">Avg Cycle Time</div>
            <div className="text-xl font-bold text-gray-900">
              {(results.bestChromosome.reduce((sum, gene) => sum + gene.greenTime + gene.redTime, 0) / results.bestChromosome.length).toFixed(1)}s
            </div>
          </div>
          <div className="border border-gray-200 rounded-lg p-3">
            <div className="text-xs text-gray-500 mb-1">Avg Green Time</div>
            <div className="text-xl font-bold text-green-600">
              {(results.bestChromosome.reduce((sum, gene) => sum + gene.greenTime, 0) / results.bestChromosome.length).toFixed(1)}s
            </div>
          </div>
          <div className="border border-gray-200 rounded-lg p-3">
            <div className="text-xs text-gray-500 mb-1">Avg Red Time</div>
            <div className="text-xl font-bold text-red-600">
              {(results.bestChromosome.reduce((sum, gene) => sum + gene.redTime, 0) / results.bestChromosome.length).toFixed(1)}s
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
