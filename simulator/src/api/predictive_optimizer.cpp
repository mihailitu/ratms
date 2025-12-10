#include "predictive_optimizer.h"
#include "../utils/logger.h"
#include <nlohmann/json.hpp>
#include <ctime>
#include <algorithm>
#include <numeric>

using json = nlohmann::json;

namespace ratms {
namespace api {

PredictiveOptimizer::PredictiveOptimizer(
    std::shared_ptr<prediction::TrafficPredictor> predictor,
    std::shared_ptr<data::DatabaseManager> dbManager,
    std::shared_ptr<simulator::Simulator> simulator,
    std::mutex& simMutex
) : predictor_(predictor),
    dbManager_(dbManager),
    simulator_(simulator),
    simMutex_(simMutex) {
    LOG_INFO(LogComponent::Optimization, "PredictiveOptimizer initialized");
}

void PredictiveOptimizer::setConfig(const PredictiveOptimizerConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_ = config;
    LOG_INFO(LogComponent::Optimization, "PredictiveOptimizer config updated: horizon={}min",
             config.predictionHorizonMinutes);
}

PredictiveOptimizerConfig PredictiveOptimizer::getConfig() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_;
}

std::string PredictiveOptimizer::getStatusMessage() const {
    std::lock_guard<std::mutex> lock(statusMutex_);
    return statusMessage_;
}

double PredictiveOptimizer::getProgress() const {
    switch (currentStatus_.load()) {
        case PipelineStatus::IDLE: return 0.0;
        case PipelineStatus::PREDICTING: return 0.1;
        case PipelineStatus::OPTIMIZING: return 0.5;
        case PipelineStatus::VALIDATING: return 0.8;
        case PipelineStatus::APPLYING: return 0.9;
        case PipelineStatus::COMPLETE: return 1.0;
        case PipelineStatus::ERROR: return 0.0;
        default: return 0.0;
    }
}

PredictiveOptimizationResult PredictiveOptimizer::runOptimization() {
    std::lock_guard<std::mutex> lock(configMutex_);
    return runOptimization(config_.predictionHorizonMinutes);
}

PredictiveOptimizationResult PredictiveOptimizer::runOptimization(int horizonMinutes) {
    PredictiveOptimizationResult result;
    result.runId = -1;
    result.startTime = std::time(nullptr);
    result.horizonMinutes = horizonMinutes;

    LOG_INFO(LogComponent::Optimization,
             "Starting predictive optimization with {}min horizon", horizonMinutes);

    try {
        // Stage 1: PREDICTING
        currentStatus_ = PipelineStatus::PREDICTING;
        {
            std::lock_guard<std::mutex> lock(statusMutex_);
            statusMessage_ = "Getting traffic prediction for T+" +
                            std::to_string(horizonMinutes) + " minutes";
        }

        auto prediction = performPrediction(horizonMinutes);

        result.predictedDayOfWeek = prediction.targetDayOfWeek;
        result.predictedTimeSlot = prediction.targetTimeSlot;
        result.predictedTimeSlotString = prediction.targetTimeSlotString;
        result.averagePredictionConfidence = prediction.averageConfidence;

        LOG_INFO(LogComponent::Optimization,
                 "Prediction complete: target={} (day={}, slot={}), confidence={:.2f}",
                 prediction.targetTimeSlotString,
                 prediction.targetDayOfWeek,
                 prediction.targetTimeSlot,
                 prediction.averageConfidence);

        // Store prediction for accuracy tracking
        {
            std::lock_guard<std::mutex> lock(accuracyMutex_);
            PendingPrediction pending;
            pending.predictionTime = result.startTime;
            pending.targetTime = prediction.targetTimestamp;
            pending.horizonMinutes = horizonMinutes;

            for (const auto& roadPred : prediction.roadPredictions) {
                pending.predictedVehicleCounts[roadPred.roadId] = roadPred.vehicleCount;
                pending.predictedQueueLengths[roadPred.roadId] = roadPred.queueLength;
            }

            pendingPredictions_.push_back(pending);

            // Limit pending predictions to prevent memory growth
            if (pendingPredictions_.size() > 50) {
                pendingPredictions_.erase(pendingPredictions_.begin());
            }
        }

        // Stage 2: Create predicted network
        auto predictedNetwork = createPredictedNetwork(prediction);

        if (predictedNetwork.empty()) {
            throw std::runtime_error("Failed to create predicted network - no roads available");
        }

        LOG_INFO(LogComponent::Optimization,
                 "Created predicted network with {} roads", predictedNetwork.size());

        // Stage 3: OPTIMIZING
        currentStatus_ = PipelineStatus::OPTIMIZING;
        {
            std::lock_guard<std::mutex> lock(statusMutex_);
            statusMessage_ = "Running GA optimization on predicted traffic state";
        }

        double baselineFitness = 0.0;
        auto bestChromosome = runGAOptimization(predictedNetwork, baselineFitness);

        result.baselineFitness = baselineFitness;
        result.optimizedFitness = bestChromosome.fitness;
        result.improvementPercent =
            ((baselineFitness - bestChromosome.fitness) / baselineFitness) * 100.0;
        result.bestChromosome = bestChromosome;

        LOG_INFO(LogComponent::Optimization,
                 "GA optimization complete: baseline={:.2f}, optimized={:.2f}, improvement={:.1f}%",
                 baselineFitness, bestChromosome.fitness, result.improvementPercent);

        // Stage 4: APPLYING (store result, actual application is separate)
        currentStatus_ = PipelineStatus::APPLYING;
        {
            std::lock_guard<std::mutex> lock(statusMutex_);
            statusMessage_ = "Storing optimization results";
        }

        // Persist to database if available
        if (dbManager_ && result.improvementPercent > 0) {
            persistResults(result);
        }

        // Stage 5: COMPLETE
        result.endTime = std::time(nullptr);
        result.finalStatus = PipelineStatus::COMPLETE;
        currentStatus_ = PipelineStatus::COMPLETE;
        {
            std::lock_guard<std::mutex> lock(statusMutex_);
            statusMessage_ = "Optimization complete";
        }

        // Update statistics
        totalRuns_++;
        if (result.improvementPercent > 0) {
            successfulRuns_++;
        }
        // Update running average
        double currentAvg = averageImprovement_.load();
        int runs = totalRuns_.load();
        averageImprovement_ = ((currentAvg * (runs - 1)) + result.improvementPercent) / runs;

        LOG_INFO(LogComponent::Optimization,
                 "Predictive optimization completed successfully");

    } catch (const std::exception& e) {
        result.endTime = std::time(nullptr);
        result.finalStatus = PipelineStatus::ERROR;
        result.errorMessage = e.what();
        currentStatus_ = PipelineStatus::ERROR;
        {
            std::lock_guard<std::mutex> lock(statusMutex_);
            statusMessage_ = std::string("Error: ") + e.what();
        }

        LOG_ERROR(LogComponent::Optimization,
                  "Predictive optimization failed: {}", e.what());
    }

    return result;
}

prediction::PredictionResult PredictiveOptimizer::performPrediction(int horizonMinutes) {
    if (!predictor_) {
        throw std::runtime_error("Traffic predictor not initialized");
    }

    return predictor_->predictForecast(horizonMinutes);
}

std::vector<simulator::Road> PredictiveOptimizer::createPredictedNetwork(
    const prediction::PredictionResult& prediction) {

    std::vector<simulator::Road> network;

    // Copy current network
    {
        std::lock_guard<std::mutex> lock(simMutex_);
        if (!simulator_) {
            return network;
        }

        network.reserve(simulator_->cityMap.size());
        for (const auto& [id, road] : simulator_->cityMap) {
            network.push_back(road);
        }
    }

    // Adjust network based on predictions
    adjustNetworkForPrediction(network, prediction);

    return network;
}

void PredictiveOptimizer::adjustNetworkForPrediction(
    std::vector<simulator::Road>& network,
    const prediction::PredictionResult& prediction) {

    // Build a map of predicted metrics by road ID
    std::map<int, prediction::PredictedMetrics> predictionByRoad;
    for (const auto& roadPred : prediction.roadPredictions) {
        predictionByRoad[roadPred.roadId] = roadPred;
    }

    // Get config for scaling
    PredictiveOptimizerConfig cfg;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        cfg = config_;
    }

    // Adjust each road based on prediction
    for (auto& road : network) {
        int roadId = road.getId();
        auto it = predictionByRoad.find(roadId);

        if (it == predictionByRoad.end()) {
            continue;  // No prediction for this road, keep as-is
        }

        const auto& pred = it->second;

        // Get current vehicle count
        int currentCount = road.getVehicleCount();
        int targetCount = static_cast<int>(pred.vehicleCount * cfg.vehicleScaleFactor);

        // Adjust vehicle count to match prediction
        if (targetCount > currentCount) {
            // Need to add vehicles
            int toAdd = targetCount - currentCount;
            double roadLength = road.getLength();
            double maxSpeed = road.getMaxSpeed();

            for (int i = 0; i < toAdd && i < 50; ++i) {  // Cap at 50 per road
                // Distribute along road
                double position = (roadLength * 0.1) + (roadLength * 0.8 * i / std::max(toAdd, 1));
                if (position >= roadLength - 10.0) break;

                // Velocity based on predicted average speed
                double velocity = pred.avgSpeed > 0 ? pred.avgSpeed * 0.9 : maxSpeed * 0.5;

                simulator::Vehicle v(position, 5.0, velocity);
                v.setAggressivity(0.5);  // Average driver

                // Add to lane with fewest vehicles
                unsigned bestLane = 0;
                size_t minVehicles = SIZE_MAX;
                for (unsigned lane = 0; lane < road.getLanesNo(); ++lane) {
                    size_t count = road.getVehicles()[lane].size();
                    if (count < minVehicles) {
                        minVehicles = count;
                        bestLane = lane;
                    }
                }
                road.addVehicle(v, bestLane);
            }
        }
        // Note: Vehicle removal not implemented as Road class doesn't expose mutable vehicle access.
        // For predictive optimization, we focus on adding vehicles for increased traffic predictions.
    }

    LOG_DEBUG(LogComponent::Optimization,
              "Adjusted network based on prediction for {} roads",
              prediction.roadPredictions.size());
}

simulator::Chromosome PredictiveOptimizer::runGAOptimization(
    const std::vector<simulator::Road>& network,
    double& baselineFitness) {

    // Get config
    PredictiveOptimizerConfig cfg;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        cfg = config_;
    }

    // Count traffic lights
    size_t totalTrafficLights = 0;
    for (const auto& road : network) {
        totalTrafficLights += road.getLanesNo();
    }

    // Setup GA parameters
    simulator::GeneticAlgorithm::Parameters gaParams;
    gaParams.populationSize = cfg.populationSize;
    gaParams.generations = cfg.generations;
    gaParams.mutationRate = 0.15;
    gaParams.mutationStdDev = 5.0;
    gaParams.crossoverRate = 0.8;
    gaParams.tournamentSize = 3;
    gaParams.elitismRate = 0.1;
    gaParams.minGreenTime = cfg.minGreenTime;
    gaParams.maxGreenTime = cfg.maxGreenTime;
    gaParams.minRedTime = cfg.minRedTime;
    gaParams.maxRedTime = cfg.maxRedTime;
    gaParams.seed = static_cast<unsigned>(
        std::chrono::system_clock::now().time_since_epoch().count());

    // Create fitness evaluator
    simulator::FitnessEvaluator evaluator(cfg.simulationSteps, cfg.dt);

    // Run baseline simulation
    std::vector<simulator::Road> baselineNetwork = network;  // Copy
    simulator::Simulator baseSim;
    for (auto& road : baselineNetwork) {
        baseSim.addRoadToMap(road);
    }

    simulator::MetricsCollector baselineCollector;
    std::vector<simulator::RoadTransition> pendingTransitions;

    for (int step = 0; step < cfg.simulationSteps; ++step) {
        pendingTransitions.clear();
        for (auto& roadPair : baseSim.cityMap) {
            roadPair.second.update(cfg.dt, baseSim.cityMap, pendingTransitions);
        }

        for (const auto& transition : pendingTransitions) {
            simulator::Vehicle transitioningVehicle = std::get<0>(transition);
            simulator::roadID destRoadID = std::get<1>(transition);
            unsigned destLane = std::get<2>(transition);

            auto destRoadIt = baseSim.cityMap.find(destRoadID);
            if (destRoadIt != baseSim.cityMap.end()) {
                transitioningVehicle.setPos(0.0);
                destRoadIt->second.addVehicle(transitioningVehicle, destLane);
            } else {
                baselineCollector.getMetricsMutable().vehiclesExited += 1.0;
            }
        }

        if (step % 10 == 0) {
            baselineCollector.collectMetrics(baseSim.cityMap, cfg.dt);
        }
    }

    simulator::SimulationMetrics baselineMetrics = baselineCollector.getMetrics();
    if (baselineMetrics.sampleCount > 0) {
        baselineMetrics.averageQueueLength /= baselineMetrics.sampleCount;
        baselineMetrics.averageSpeed /= baselineMetrics.sampleCount;
    }
    baselineFitness = baselineMetrics.getFitness();

    // Create fitness function using predicted network
    auto fitnessFunc = [&evaluator, &network](const simulator::Chromosome& chromosome) -> double {
        std::vector<simulator::Road> networkCopy = network;
        return evaluator.evaluate(chromosome, networkCopy);
    };

    // Run GA optimization
    simulator::GeneticAlgorithm ga(gaParams, fitnessFunc);
    ga.initializePopulation(totalTrafficLights);
    simulator::Chromosome bestSolution = ga.evolve();

    return bestSolution;
}

bool PredictiveOptimizer::persistResults(const PredictiveOptimizationResult& result) {
    if (!dbManager_ || !result.bestChromosome.has_value()) {
        return false;
    }

    try {
        // Create optimization run record
        data::DatabaseManager::OptimizationRunRecord runRecord;
        runRecord.network_id = 1;  // Default network
        runRecord.status = "completed";
        runRecord.started_at = result.startTime;
        runRecord.completed_at = result.endTime;
        runRecord.duration_seconds = result.endTime - result.startTime;
        runRecord.population_size = config_.populationSize;
        runRecord.generations = config_.generations;
        runRecord.simulation_steps = config_.simulationSteps;
        runRecord.baseline_fitness = result.baselineFitness;
        runRecord.best_fitness = result.optimizedFitness;
        runRecord.improvement_percent = result.improvementPercent;

        int runId = dbManager_->createOptimizationRun(runRecord);
        if (runId <= 0) {
            LOG_WARN(LogComponent::Optimization,
                     "Failed to create optimization run record");
            return false;
        }

        // Store best solution
        const auto& chromosome = result.bestChromosome.value();
        json chromosomeJson = json::array();
        for (const auto& gene : chromosome.genes) {
            chromosomeJson.push_back({
                {"greenTime", gene.greenTime},
                {"redTime", gene.redTime}
            });
        }

        data::DatabaseManager::OptimizationSolutionRecord solutionRecord;
        solutionRecord.optimization_run_id = runId;
        solutionRecord.fitness = chromosome.fitness;
        solutionRecord.chromosome_json = chromosomeJson.dump();
        solutionRecord.is_best_solution = true;
        solutionRecord.traffic_light_count = static_cast<int>(chromosome.genes.size());
        solutionRecord.created_at = std::time(nullptr);

        dbManager_->insertOptimizationSolution(solutionRecord);

        LOG_INFO(LogComponent::Optimization,
                 "Persisted predictive optimization results: runId={}", runId);

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::Optimization,
                  "Failed to persist optimization results: {}", e.what());
        return false;
    }
}

bool PredictiveOptimizer::applyChromosome(const simulator::Chromosome& chromosome) {
    std::lock_guard<std::mutex> lock(simMutex_);

    if (!simulator_) {
        LOG_ERROR(LogComponent::Optimization, "Cannot apply chromosome - simulator not initialized");
        return false;
    }

    size_t geneIdx = 0;
    for (auto& [roadId, road] : simulator_->cityMap) {
        for (unsigned lane = 0; lane < road.getLanesNo(); ++lane) {
            if (geneIdx < chromosome.genes.size()) {
                const auto& timing = chromosome.genes[geneIdx];
                road.getTrafficLightsMutable()[lane].setTimings(
                    timing.greenTime, 3.0, timing.redTime);
                geneIdx++;
            }
        }
    }

    LOG_INFO(LogComponent::Optimization,
             "Applied optimized chromosome with {} traffic light timings", geneIdx);
    return true;
}

void PredictiveOptimizer::recordActualMetrics() {
    std::lock_guard<std::mutex> lock(accuracyMutex_);

    int64_t now = std::time(nullptr);

    // Find predictions whose target time has passed
    auto it = pendingPredictions_.begin();
    while (it != pendingPredictions_.end()) {
        if (now >= it->targetTime) {
            // Get actual metrics from simulator
            std::map<int, double> actualVehicleCounts;
            std::map<int, double> actualQueueLengths;

            {
                std::lock_guard<std::mutex> simLock(simMutex_);
                if (simulator_) {
                    for (const auto& [roadId, road] : simulator_->cityMap) {
                        actualVehicleCounts[roadId] = road.getVehicleCount();

                        // Calculate queue length
                        double queueLength = 0;
                        double roadLength = road.getLength();
                        for (unsigned lane = 0; lane < road.getLanesNo(); ++lane) {
                            for (const auto& v : road.getVehicles()[lane]) {
                                if (v.getPos() >= roadLength - 50.0 && v.getVelocity() < 2.0) {
                                    queueLength += 1.0;
                                }
                            }
                        }
                        actualQueueLengths[roadId] = queueLength;
                    }
                }
            }

            // Calculate accuracy
            PredictionAccuracy accuracy;
            accuracy.timestamp = now;
            accuracy.horizonMinutes = it->horizonMinutes;

            double totalPredVehicles = 0, totalActualVehicles = 0;
            double totalPredQueue = 0, totalActualQueue = 0;
            int roadCount = 0;

            for (const auto& [roadId, predCount] : it->predictedVehicleCounts) {
                auto actualIt = actualVehicleCounts.find(roadId);
                if (actualIt != actualVehicleCounts.end()) {
                    totalPredVehicles += predCount;
                    totalActualVehicles += actualIt->second;
                    roadCount++;
                }
            }

            for (const auto& [roadId, predQueue] : it->predictedQueueLengths) {
                auto actualIt = actualQueueLengths.find(roadId);
                if (actualIt != actualQueueLengths.end()) {
                    totalPredQueue += predQueue;
                    totalActualQueue += actualIt->second;
                }
            }

            if (roadCount > 0) {
                accuracy.predictedVehicleCount = totalPredVehicles / roadCount;
                accuracy.actualVehicleCount = totalActualVehicles / roadCount;
                accuracy.vehicleCountError =
                    std::abs(accuracy.predictedVehicleCount - accuracy.actualVehicleCount);

                accuracy.predictedQueueLength = totalPredQueue / roadCount;
                accuracy.actualQueueLength = totalActualQueue / roadCount;
                accuracy.queueLengthError =
                    std::abs(accuracy.predictedQueueLength - accuracy.actualQueueLength);

                // Calculate overall accuracy score
                double vehicleAccuracy = calculateAccuracyScore(
                    accuracy.predictedVehicleCount, accuracy.actualVehicleCount);
                double queueAccuracy = calculateAccuracyScore(
                    accuracy.predictedQueueLength, accuracy.actualQueueLength);

                accuracy.accuracyScore = (vehicleAccuracy + queueAccuracy) / 2.0;

                accuracyHistory_.push_back(accuracy);

                // Limit history size
                if (accuracyHistory_.size() > MAX_ACCURACY_HISTORY) {
                    accuracyHistory_.erase(accuracyHistory_.begin());
                }

                LOG_INFO(LogComponent::Optimization,
                         "Recorded prediction accuracy: vehicles={:.1f} vs {:.1f}, "
                         "queue={:.1f} vs {:.1f}, score={:.2f}",
                         accuracy.predictedVehicleCount, accuracy.actualVehicleCount,
                         accuracy.predictedQueueLength, accuracy.actualQueueLength,
                         accuracy.accuracyScore);
            }

            it = pendingPredictions_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<PredictionAccuracy> PredictiveOptimizer::getAccuracyHistory() const {
    std::lock_guard<std::mutex> lock(accuracyMutex_);
    return accuracyHistory_;
}

double PredictiveOptimizer::getAverageAccuracy() const {
    std::lock_guard<std::mutex> lock(accuracyMutex_);

    if (accuracyHistory_.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& acc : accuracyHistory_) {
        sum += acc.accuracyScore;
    }

    return sum / accuracyHistory_.size();
}

double PredictiveOptimizer::calculateAccuracyScore(
    double predictedValue, double actualValue) const {

    if (predictedValue == 0 && actualValue == 0) {
        return 1.0;  // Perfect match for zero values
    }

    double maxVal = std::max(predictedValue, actualValue);
    if (maxVal == 0) {
        return 0.0;
    }

    double error = std::abs(predictedValue - actualValue);
    double normalizedError = error / maxVal;

    // Score decreases with error, clamped to [0, 1]
    return std::max(0.0, 1.0 - normalizedError);
}

} // namespace api
} // namespace ratms
