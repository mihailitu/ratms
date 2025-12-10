#include "continuous_optimization_controller.h"
#include "predictive_optimizer.h"
#include "../tests/testmap.h"
#include "../utils/logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ratms {
namespace api {

// TimingTransition implementation
double TimingTransition::getCurrentGreenTime() const {
    double progress = getProgress();
    return startGreen + (endGreen - startGreen) * progress;
}

double TimingTransition::getCurrentRedTime() const {
    double progress = getProgress();
    return startRed + (endRed - startRed) * progress;
}

bool TimingTransition::isComplete() const {
    return std::chrono::steady_clock::now() >= endTime;
}

double TimingTransition::getProgress() const {
    auto now = std::chrono::steady_clock::now();
    if (now >= endTime) return 1.0;
    if (now <= startTime) return 0.0;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
    auto total = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    return static_cast<double>(elapsed.count()) / static_cast<double>(total.count());
}

// Helper functions
static void sendError(httplib::Response& res, int status, const std::string& message) {
    json error = {
        {"success", false},
        {"error", message}
    };
    res.status = status;
    res.set_content(error.dump(2), "application/json");
}

static void sendSuccess(httplib::Response& res, const json& data) {
    json response = {
        {"success", true},
        {"data", data}
    };
    res.set_content(response.dump(2), "application/json");
}

ContinuousOptimizationController::ContinuousOptimizationController(
    std::shared_ptr<data::DatabaseManager> dbManager,
    std::shared_ptr<simulator::Simulator> simulator,
    std::mutex& simMutex
) : dbManager_(dbManager), simulator_(simulator), simMutex_(simMutex) {
    LOG_INFO(LogComponent::Optimization, "ContinuousOptimizationController initialized");
}

ContinuousOptimizationController::~ContinuousOptimizationController() {
    stop();
}

void ContinuousOptimizationController::setPredictor(
    std::shared_ptr<prediction::TrafficPredictor> predictor) {
    predictor_ = predictor;

    // Create predictive optimizer if we have all dependencies
    if (predictor_ && dbManager_ && simulator_) {
        predictiveOptimizer_ = std::make_unique<PredictiveOptimizer>(
            predictor_, dbManager_, simulator_, simMutex_);
        LOG_INFO(LogComponent::Optimization, "PredictiveOptimizer initialized");
    }
}

void ContinuousOptimizationController::registerRoutes(httplib::Server& server) {
    server.Post("/api/optimization/continuous/start", [this](const httplib::Request& req, httplib::Response& res) {
        handleStart(req, res);
    });

    server.Post("/api/optimization/continuous/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleStop(req, res);
    });

    server.Get("/api/optimization/continuous/status", [this](const httplib::Request& req, httplib::Response& res) {
        handleStatus(req, res);
    });

    server.Post("/api/optimization/apply/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        handleApply(req, res);
    });

    server.Get("/api/optimization/continuous/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleConfig(req, res);
    });

    server.Post("/api/optimization/continuous/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetConfig(req, res);
    });

    // Rollout and validation endpoints
    server.Post("/api/optimization/rollback", [this](const httplib::Request& req, httplib::Response& res) {
        handleRollback(req, res);
    });

    server.Get("/api/optimization/rollout/status", [this](const httplib::Request& req, httplib::Response& res) {
        handleRolloutStatus(req, res);
    });

    server.Get("/api/optimization/validation/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleValidationConfig(req, res);
    });

    server.Post("/api/optimization/validation/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleSetValidationConfig(req, res);
    });

    LOG_INFO(LogComponent::API, "Continuous optimization routes registered");
}

void ContinuousOptimizationController::handleStart(const httplib::Request& req, httplib::Response& res) {
    if (running_) {
        sendError(res, 400, "Continuous optimization already running");
        return;
    }

    // Parse optional config from request body
    if (!req.body.empty()) {
        try {
            json body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(configMutex_);

            if (body.contains("optimizationIntervalSeconds")) {
                config_.optimizationIntervalSeconds = body["optimizationIntervalSeconds"];
            }
            if (body.contains("transitionDurationSeconds")) {
                config_.transitionDurationSeconds = body["transitionDurationSeconds"];
            }
            if (body.contains("populationSize")) {
                config_.populationSize = body["populationSize"];
            }
            if (body.contains("generations")) {
                config_.generations = body["generations"];
            }
            if (body.contains("usePrediction")) {
                config_.usePrediction = body["usePrediction"].get<bool>();
            }
            if (body.contains("predictionHorizonMinutes")) {
                int horizon = body["predictionHorizonMinutes"].get<int>();
                if (horizon >= 10 && horizon <= 120) {
                    config_.predictionHorizonMinutes = horizon;
                }
            }
        } catch (const std::exception& e) {
            LOG_WARN(LogComponent::API, "Failed to parse config from request: {}", e.what());
        }
    }

    // Check prediction mode requirements
    if (config_.usePrediction && !predictiveOptimizer_) {
        sendError(res, 400, "Prediction mode enabled but predictor not initialized. "
                           "Ensure traffic pattern storage is set up.");
        return;
    }

    start();

    std::string mode = config_.usePrediction ? "predictive" : "reactive";
    LOG_INFO(LogComponent::Optimization, "Starting continuous optimization in {} mode", mode);

    json configJson = {
        {"optimizationIntervalSeconds", config_.optimizationIntervalSeconds},
        {"transitionDurationSeconds", config_.transitionDurationSeconds},
        {"populationSize", config_.populationSize},
        {"generations", config_.generations},
        {"usePrediction", config_.usePrediction},
        {"predictionHorizonMinutes", config_.predictionHorizonMinutes}
    };

    sendSuccess(res, {
        {"message", "Continuous optimization started"},
        {"mode", mode},
        {"config", configJson}
    });
}

void ContinuousOptimizationController::handleStop(const httplib::Request& req, httplib::Response& res) {
    if (!running_) {
        sendError(res, 400, "Continuous optimization not running");
        return;
    }

    stop();
    sendSuccess(res, {{"message", "Continuous optimization stopped"}});
}

void ContinuousOptimizationController::handleStatus(const httplib::Request& req, httplib::Response& res) {
    // Get active transitions
    std::vector<TimingTransition> transitions;
    {
        std::lock_guard<std::mutex> lock(transitionsMutex_);
        transitions = activeTransitions_;
    }

    // Build transitions JSON
    json transitionsJson = json::array();
    for (const auto& t : transitions) {
        transitionsJson.push_back({
            {"roadId", t.roadId},
            {"lane", t.lane},
            {"startGreen", t.startGreen},
            {"endGreen", t.endGreen},
            {"currentGreen", t.getCurrentGreenTime()},
            {"startRed", t.startRed},
            {"endRed", t.endRed},
            {"currentRed", t.getCurrentRedTime()},
            {"progress", t.getProgress()},
            {"isComplete", t.isComplete()}
        });
    }

    // Get time since last optimization
    double secondsSinceLastOptimization = 0;
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastOptimizationTime_);
        secondsSinceLastOptimization = elapsed.count();
    }

    // Build prediction status
    json predictionStatus = {
        {"enabled", config_.usePrediction},
        {"available", predictiveOptimizer_ != nullptr},
        {"horizonMinutes", config_.predictionHorizonMinutes}
    };

    if (predictiveOptimizer_) {
        predictionStatus["pipelineStatus"] = pipelineStatusToString(predictiveOptimizer_->getStatus());
        predictionStatus["averageAccuracy"] = predictiveOptimizer_->getAverageAccuracy();
    }

    std::string mode = config_.usePrediction ? "predictive" : "reactive";

    sendSuccess(res, {
        {"running", running_.load()},
        {"mode", mode},
        {"totalOptimizationRuns", totalOptimizationRuns_.load()},
        {"successfulOptimizations", successfulOptimizations_.load()},
        {"lastImprovementPercent", lastImprovementPercent_.load()},
        {"secondsSinceLastOptimization", secondsSinceLastOptimization},
        {"nextOptimizationIn", std::max(0, config_.optimizationIntervalSeconds - static_cast<int>(secondsSinceLastOptimization))},
        {"activeTransitions", transitionsJson},
        {"prediction", predictionStatus},
        {"config", {
            {"optimizationIntervalSeconds", config_.optimizationIntervalSeconds},
            {"transitionDurationSeconds", config_.transitionDurationSeconds},
            {"populationSize", config_.populationSize},
            {"generations", config_.generations},
            {"usePrediction", config_.usePrediction},
            {"predictionHorizonMinutes", config_.predictionHorizonMinutes}
        }}
    });
}

void ContinuousOptimizationController::handleApply(const httplib::Request& req, httplib::Response& res) {
    try {
        int runId = std::stoi(req.matches[1]);
        LOG_INFO(LogComponent::Optimization, "Manual apply request for run {}", runId);

        if (applyOptimizationRun(runId)) {
            sendSuccess(res, {
                {"message", "Optimization applied with gradual transition"},
                {"runId", runId},
                {"transitionDurationSeconds", config_.transitionDurationSeconds}
            });
        } else {
            sendError(res, 404, "Optimization run not found or not completed");
        }
    } catch (const std::exception& e) {
        sendError(res, 400, e.what());
    }
}

void ContinuousOptimizationController::handleConfig(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(configMutex_);
    sendSuccess(res, {
        {"optimizationIntervalSeconds", config_.optimizationIntervalSeconds},
        {"transitionDurationSeconds", config_.transitionDurationSeconds},
        {"populationSize", config_.populationSize},
        {"generations", config_.generations},
        {"simulationSteps", config_.simulationSteps},
        {"dt", config_.dt},
        {"minGreenTime", config_.minGreenTime},
        {"maxGreenTime", config_.maxGreenTime},
        {"minRedTime", config_.minRedTime},
        {"maxRedTime", config_.maxRedTime},
        {"usePrediction", config_.usePrediction},
        {"predictionHorizonMinutes", config_.predictionHorizonMinutes},
        {"predictionAvailable", predictiveOptimizer_ != nullptr}
    });
}

void ContinuousOptimizationController::handleSetConfig(const httplib::Request& req, httplib::Response& res) {
    try {
        json body = json::parse(req.body);
        std::lock_guard<std::mutex> lock(configMutex_);

        if (body.contains("optimizationIntervalSeconds")) {
            int val = body["optimizationIntervalSeconds"];
            if (val < 60 || val > 3600) {
                sendError(res, 400, "optimizationIntervalSeconds must be between 60 and 3600");
                return;
            }
            config_.optimizationIntervalSeconds = val;
        }
        if (body.contains("transitionDurationSeconds")) {
            int val = body["transitionDurationSeconds"];
            if (val < 30 || val > 600) {
                sendError(res, 400, "transitionDurationSeconds must be between 30 and 600");
                return;
            }
            config_.transitionDurationSeconds = val;
        }
        if (body.contains("populationSize")) {
            int val = body["populationSize"];
            if (val < 10 || val > 100) {
                sendError(res, 400, "populationSize must be between 10 and 100");
                return;
            }
            config_.populationSize = val;
        }
        if (body.contains("generations")) {
            int val = body["generations"];
            if (val < 10 || val > 200) {
                sendError(res, 400, "generations must be between 10 and 200");
                return;
            }
            config_.generations = val;
        }
        if (body.contains("usePrediction")) {
            bool usePred = body["usePrediction"].get<bool>();
            if (usePred && !predictiveOptimizer_) {
                sendError(res, 400, "Cannot enable prediction - predictor not initialized");
                return;
            }
            config_.usePrediction = usePred;
        }
        if (body.contains("predictionHorizonMinutes")) {
            int val = body["predictionHorizonMinutes"];
            if (val < 10 || val > 120) {
                sendError(res, 400, "predictionHorizonMinutes must be between 10 and 120");
                return;
            }
            config_.predictionHorizonMinutes = val;
        }

        LOG_INFO(LogComponent::Optimization, "Continuous optimization config updated");
        sendSuccess(res, {{"message", "Configuration updated"}});

    } catch (const std::exception& e) {
        sendError(res, 400, e.what());
    }
}

void ContinuousOptimizationController::start() {
    if (running_) return;

    running_ = true;
    lastOptimizationTime_ = std::chrono::steady_clock::now();
    optimizationThread_ = std::make_unique<std::thread>(&ContinuousOptimizationController::optimizationLoop, this);
    LOG_INFO(LogComponent::Optimization, "Continuous optimization started with {}s interval",
             config_.optimizationIntervalSeconds);
}

void ContinuousOptimizationController::stop() {
    running_ = false;
    if (optimizationThread_ && optimizationThread_->joinable()) {
        optimizationThread_->join();
    }
    LOG_INFO(LogComponent::Optimization, "Continuous optimization stopped");
}

void ContinuousOptimizationController::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_ = config;
}

std::vector<TimingTransition> ContinuousOptimizationController::getActiveTransitions() const {
    std::lock_guard<std::mutex> lock(transitionsMutex_);
    return activeTransitions_;
}

void ContinuousOptimizationController::updateTransitions() {
    std::lock_guard<std::mutex> lock(transitionsMutex_);

    if (activeTransitions_.empty()) return;

    // Apply current interpolated values to traffic lights
    {
        std::lock_guard<std::mutex> simLock(simMutex_);
        if (!simulator_) return;

        for (const auto& transition : activeTransitions_) {
            auto roadIt = simulator_->cityMap.find(transition.roadId);
            if (roadIt != simulator_->cityMap.end()) {
                double currentGreen = transition.getCurrentGreenTime();
                double currentRed = transition.getCurrentRedTime();
                if (transition.lane < roadIt->second.getTrafficLightsMutable().size()) {
                    roadIt->second.getTrafficLightsMutable()[transition.lane].setTimings(currentGreen, 3.0, currentRed);
                }
            }
        }
    }

    // Remove completed transitions
    activeTransitions_.erase(
        std::remove_if(activeTransitions_.begin(), activeTransitions_.end(),
            [](const TimingTransition& t) { return t.isComplete(); }),
        activeTransitions_.end()
    );
}

void ContinuousOptimizationController::optimizationLoop() {
    LOG_INFO(LogComponent::Optimization, "Continuous optimization loop started");

    while (running_) {
        // Wait for next optimization interval
        for (int i = 0; i < config_.optimizationIntervalSeconds && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Update transitions every second
            updateTransitions();
        }

        if (!running_) break;

        // Run optimization cycle
        try {
            runOptimizationCycle();
            totalOptimizationRuns_++;
            {
                std::lock_guard<std::mutex> lock(statsMutex_);
                lastOptimizationTime_ = std::chrono::steady_clock::now();
            }
        } catch (const std::exception& e) {
            LOG_ERROR(LogComponent::Optimization, "Optimization cycle failed: {}", e.what());
        }
    }

    LOG_INFO(LogComponent::Optimization, "Continuous optimization loop stopped");
}

void ContinuousOptimizationController::runOptimizationCycle() {
    // Get current config
    Config config;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        config = config_;
    }

    // Use predictive optimization if enabled and available
    if (config.usePrediction && predictiveOptimizer_) {
        LOG_INFO(LogComponent::Optimization,
                 "Starting predictive optimization cycle (horizon={}min)",
                 config.predictionHorizonMinutes);

        // Record actual metrics for previous predictions (for accuracy tracking)
        predictiveOptimizer_->recordActualMetrics();

        // Run predictive optimization
        auto result = predictiveOptimizer_->runOptimization(config.predictionHorizonMinutes);

        if (result.finalStatus == PipelineStatus::COMPLETE &&
            result.bestChromosome.has_value() &&
            result.improvementPercent > 0) {

            // Apply with gradual transition
            applyChromosomeGradually(result.bestChromosome.value());
            successfulOptimizations_++;
            lastImprovementPercent_ = result.improvementPercent;

            LOG_INFO(LogComponent::Optimization,
                     "Predictive optimization cycle complete: improvement={:.1f}%, confidence={:.2f}",
                     result.improvementPercent, result.averagePredictionConfidence);
        } else if (result.finalStatus == PipelineStatus::ERROR) {
            LOG_ERROR(LogComponent::Optimization,
                      "Predictive optimization failed: {}", result.errorMessage);
        } else {
            LOG_INFO(LogComponent::Optimization,
                     "Predictive optimization found no improvement");
        }

        return;
    }

    // Fall back to reactive (current state) optimization
    LOG_INFO(LogComponent::Optimization, "Starting reactive optimization cycle");

    // Copy current network for optimization (instead of hardcoded test network)
    std::vector<simulator::Road> testNetwork = copyCurrentNetwork();

    if (testNetwork.empty()) {
        LOG_WARN(LogComponent::Optimization, "No network loaded, skipping optimization");
        return;
    }

    LOG_INFO(LogComponent::Optimization, "Optimizing network with {} roads", testNetwork.size());

    // Count traffic lights
    size_t totalTrafficLights = 0;
    for (const auto& road : testNetwork) {
        totalTrafficLights += road.getLanesNo();
    }

    // Setup GA parameters for quick optimization
    simulator::GeneticAlgorithm::Parameters gaParams;
    gaParams.populationSize = config.populationSize;
    gaParams.generations = config.generations;
    gaParams.mutationRate = 0.15;
    gaParams.mutationStdDev = 5.0;
    gaParams.crossoverRate = 0.8;
    gaParams.tournamentSize = 3;
    gaParams.elitismRate = 0.1;
    gaParams.minGreenTime = config.minGreenTime;
    gaParams.maxGreenTime = config.maxGreenTime;
    gaParams.minRedTime = config.minRedTime;
    gaParams.maxRedTime = config.maxRedTime;
    gaParams.seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());

    // Create fitness evaluator
    simulator::FitnessEvaluator evaluator(config.simulationSteps, config.dt);

    // Run baseline simulation using copy of current network
    std::vector<simulator::Road> baselineNetwork = copyCurrentNetwork();
    simulator::Simulator baseSim;
    for (auto& road : baselineNetwork) {
        baseSim.addRoadToMap(road);
    }

    simulator::MetricsCollector baselineCollector;
    std::vector<simulator::RoadTransition> pendingTransitions;
    for (int step = 0; step < config.simulationSteps; ++step) {
        pendingTransitions.clear();
        for (auto& roadPair : baseSim.cityMap) {
            roadPair.second.update(config.dt, baseSim.cityMap, pendingTransitions);
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
            baselineCollector.collectMetrics(baseSim.cityMap, config.dt);
        }
    }

    simulator::SimulationMetrics baselineMetrics = baselineCollector.getMetrics();
    if (baselineMetrics.sampleCount > 0) {
        baselineMetrics.averageQueueLength /= baselineMetrics.sampleCount;
        baselineMetrics.averageSpeed /= baselineMetrics.sampleCount;
    }
    double baselineFitness = baselineMetrics.getFitness();

    // Create fitness function
    auto fitnessFunc = [&evaluator, &testNetwork](const simulator::Chromosome& chromosome) -> double {
        std::vector<simulator::Road> networkCopy = testNetwork;
        return evaluator.evaluate(chromosome, networkCopy);
    };

    // Run GA optimization
    simulator::GeneticAlgorithm ga(gaParams, fitnessFunc);
    ga.initializePopulation(totalTrafficLights);
    simulator::Chromosome bestSolution = ga.evolve();

    // Calculate improvement
    double improvementPercent = ((baselineFitness - bestSolution.fitness) / baselineFitness) * 100.0;
    lastImprovementPercent_ = improvementPercent;

    LOG_INFO(LogComponent::Optimization,
             "Optimization cycle complete: baseline={:.2f}, best={:.2f}, improvement={:.1f}%",
             baselineFitness, bestSolution.fitness, improvementPercent);

    // Only apply if there's improvement
    if (improvementPercent > 0) {
        applyChromosomeGradually(bestSolution);
        successfulOptimizations_++;
    } else {
        LOG_INFO(LogComponent::Optimization, "No improvement found, skipping application");
    }
}

void ContinuousOptimizationController::applyChromosomeGradually(const simulator::Chromosome& chromosome) {
    std::lock_guard<std::mutex> transLock(transitionsMutex_);
    std::lock_guard<std::mutex> simLock(simMutex_);

    if (!simulator_) return;

    auto now = std::chrono::steady_clock::now();
    auto endTime = now + std::chrono::seconds(config_.transitionDurationSeconds);

    // Clear any existing transitions
    activeTransitions_.clear();

    size_t geneIdx = 0;
    for (auto& roadPair : simulator_->cityMap) {
        simulator::Road& road = roadPair.second;
        unsigned lanesNo = road.getLanesNo();

        for (unsigned lane = 0; lane < lanesNo; ++lane) {
            if (geneIdx < chromosome.genes.size()) {
                const simulator::TrafficLightTiming& timing = chromosome.genes[geneIdx];

                // Get current values
                const simulator::TrafficLight& light = road.getTrafficLights()[lane];
                double currentGreen = light.getGreenTime();
                double currentRed = light.getRedTime();

                // Create transition
                TimingTransition transition;
                transition.roadId = roadPair.first;
                transition.lane = lane;
                transition.startGreen = currentGreen;
                transition.endGreen = timing.greenTime;
                transition.startRed = currentRed;
                transition.endRed = timing.redTime;
                transition.startTime = now;
                transition.endTime = endTime;

                activeTransitions_.push_back(transition);
                geneIdx++;
            }
        }
    }

    LOG_INFO(LogComponent::Optimization, "Created {} gradual transitions over {}s",
             activeTransitions_.size(), config_.transitionDurationSeconds);
}

bool ContinuousOptimizationController::applyOptimizationRun(int runId) {
    // Load optimization run from database
    auto record = dbManager_->getOptimizationRun(runId);
    if (record.id <= 0 || record.status != "completed") {
        return false;
    }

    // Load best solution
    auto solution = dbManager_->getBestOptimizationSolution(runId);
    if (solution.id <= 0) {
        return false;
    }

    // Parse chromosome
    try {
        json chromosomeJson = json::parse(solution.chromosome_json);
        simulator::Chromosome chromosome;

        for (const auto& geneJson : chromosomeJson) {
            simulator::TrafficLightTiming timing;
            timing.greenTime = geneJson["greenTime"];
            timing.redTime = geneJson["redTime"];
            chromosome.genes.push_back(timing);
        }
        chromosome.fitness = solution.fitness;

        // Apply with gradual transition
        applyChromosomeGradually(chromosome);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::Optimization, "Failed to parse chromosome for run {}: {}", runId, e.what());
        return false;
    }
}

std::vector<simulator::Road> ContinuousOptimizationController::copyCurrentNetwork() {
    std::lock_guard<std::mutex> lock(simMutex_);
    if (!simulator_) return {};

    std::vector<simulator::Road> networkCopy;
    networkCopy.reserve(simulator_->cityMap.size());

    for (const auto& [id, road] : simulator_->cityMap) {
        networkCopy.push_back(road);  // Road copy constructor
    }

    return networkCopy;
}

// ============================================================================
// Rollout Monitoring Implementation
// ============================================================================

RolloutState ContinuousOptimizationController::getRolloutState() const {
    std::lock_guard<std::mutex> lock(rolloutMutex_);
    return rolloutState_;
}

void ContinuousOptimizationController::startRollout(
    const simulator::Chromosome& newChromosome,
    const simulator::Chromosome& previousChromosome,
    double preRolloutSpeed,
    double preRolloutQueue) {

    std::lock_guard<std::mutex> lock(rolloutMutex_);

    rolloutState_.startTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    rolloutState_.endTime = 0;
    rolloutState_.status = "in_progress";

    rolloutState_.preRolloutAvgSpeed = preRolloutSpeed;
    rolloutState_.preRolloutAvgQueue = preRolloutQueue;
    // Calculate baseline fitness from metrics
    rolloutState_.preRolloutFitness = preRolloutQueue * 100.0 + (10.0 - preRolloutSpeed) * 0.5;

    rolloutState_.postRolloutAvgSpeed = 0.0;
    rolloutState_.postRolloutAvgQueue = 0.0;
    rolloutState_.postRolloutFitness = 0.0;
    rolloutState_.regressionPercent = 0.0;

    rolloutState_.currentChromosome = newChromosome;
    rolloutState_.previousChromosome = previousChromosome;
    rolloutState_.updateCount = 0;

    LOG_INFO(LogComponent::Optimization, "Rollout started: preSpeed={:.2f}, preQueue={:.2f}",
             preRolloutSpeed, preRolloutQueue);
}

void ContinuousOptimizationController::updateRolloutMetrics(double avgSpeed, double avgQueue) {
    std::lock_guard<std::mutex> lock(rolloutMutex_);

    if (rolloutState_.status != "in_progress") return;

    rolloutState_.postRolloutAvgSpeed = avgSpeed;
    rolloutState_.postRolloutAvgQueue = avgQueue;
    rolloutState_.postRolloutFitness = avgQueue * 100.0 + (10.0 - avgSpeed) * 0.5;
    rolloutState_.updateCount++;

    // Calculate regression (positive = worse)
    if (rolloutState_.preRolloutFitness > 0) {
        rolloutState_.regressionPercent =
            (rolloutState_.postRolloutFitness - rolloutState_.preRolloutFitness) /
            rolloutState_.preRolloutFitness * 100.0;
    }

    // Check if monitoring period is complete
    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t elapsed = now - rolloutState_.startTime;

    if (elapsed >= config_.rolloutMonitoringDurationSeconds) {
        completeRollout();
    }
}

bool ContinuousOptimizationController::checkForRegression() {
    std::lock_guard<std::mutex> lock(rolloutMutex_);

    if (rolloutState_.status != "in_progress") return false;

    // Need at least a few updates before checking
    if (rolloutState_.updateCount < 3) return false;

    // Check if regression exceeds threshold
    if (rolloutState_.regressionPercent > config_.rolloutRegressionThreshold) {
        LOG_WARN(LogComponent::Optimization,
                 "Regression detected: {:.1f}% (threshold: {:.1f}%)",
                 rolloutState_.regressionPercent, config_.rolloutRegressionThreshold);
        return true;
    }

    return false;
}

void ContinuousOptimizationController::completeRollout() {
    // Assumes rolloutMutex_ is already held
    rolloutState_.endTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    rolloutState_.status = "complete";

    LOG_INFO(LogComponent::Optimization,
             "Rollout completed: postSpeed={:.2f}, postQueue={:.2f}, regression={:.1f}%",
             rolloutState_.postRolloutAvgSpeed, rolloutState_.postRolloutAvgQueue,
             rolloutState_.regressionPercent);
}

bool ContinuousOptimizationController::rollback() {
    RolloutState state;
    {
        std::lock_guard<std::mutex> lock(rolloutMutex_);
        if (rolloutState_.status != "in_progress") {
            LOG_WARN(LogComponent::Optimization, "Rollback requested but no active rollout");
            return false;
        }
        state = rolloutState_;
    }

    // Apply previous chromosome
    if (state.previousChromosome.genes.empty()) {
        LOG_ERROR(LogComponent::Optimization, "Rollback failed: no previous chromosome stored");
        return false;
    }

    applyChromosomeGradually(state.previousChromosome);

    {
        std::lock_guard<std::mutex> lock(rolloutMutex_);
        rolloutState_.endTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        rolloutState_.status = "rolled_back";
    }

    LOG_INFO(LogComponent::Optimization, "Rollback completed successfully");
    return true;
}

// ============================================================================
// Validation Configuration
// ============================================================================

validation::ValidationConfig ContinuousOptimizationController::getValidationConfig() const {
    return validationConfig_;
}

void ContinuousOptimizationController::setValidationConfig(
    const validation::ValidationConfig& config) {
    validationConfig_ = config;
    if (validator_) {
        validator_->setConfig(config);
    }
}

// ============================================================================
// Rollout and Validation Route Handlers
// ============================================================================

void ContinuousOptimizationController::handleRollback(
    const httplib::Request& req, httplib::Response& res) {

    if (rollback()) {
        sendSuccess(res, {
            {"message", "Rollback initiated"},
            {"transitionDurationSeconds", config_.transitionDurationSeconds}
        });
    } else {
        sendError(res, 400, "No active rollout to rollback or no previous chromosome available");
    }
}

void ContinuousOptimizationController::handleRolloutStatus(
    const httplib::Request& req, httplib::Response& res) {

    RolloutState state = getRolloutState();

    json stateJson = {
        {"status", state.status},
        {"startTime", state.startTime},
        {"endTime", state.endTime},
        {"preRollout", {
            {"avgSpeed", state.preRolloutAvgSpeed},
            {"avgQueue", state.preRolloutAvgQueue},
            {"fitness", state.preRolloutFitness}
        }},
        {"postRollout", {
            {"avgSpeed", state.postRolloutAvgSpeed},
            {"avgQueue", state.postRolloutAvgQueue},
            {"fitness", state.postRolloutFitness}
        }},
        {"regressionPercent", state.regressionPercent},
        {"updateCount", state.updateCount},
        {"hasCurrentChromosome", !state.currentChromosome.genes.empty()},
        {"hasPreviousChromosome", !state.previousChromosome.genes.empty()},
        {"config", {
            {"enableRolloutMonitoring", config_.enableRolloutMonitoring},
            {"rolloutRegressionThreshold", config_.rolloutRegressionThreshold},
            {"rolloutMonitoringDurationSeconds", config_.rolloutMonitoringDurationSeconds}
        }}
    };

    sendSuccess(res, stateJson);
}

void ContinuousOptimizationController::handleValidationConfig(
    const httplib::Request& req, httplib::Response& res) {

    sendSuccess(res, {
        {"simulationSteps", validationConfig_.simulationSteps},
        {"dt", validationConfig_.dt},
        {"improvementThreshold", validationConfig_.improvementThreshold},
        {"regressionThreshold", validationConfig_.regressionThreshold},
        {"enabled", config_.enableValidation}
    });
}

void ContinuousOptimizationController::handleSetValidationConfig(
    const httplib::Request& req, httplib::Response& res) {

    try {
        json body = json::parse(req.body);

        if (body.contains("simulationSteps")) {
            int val = body["simulationSteps"];
            if (val < 100 || val > 2000) {
                sendError(res, 400, "simulationSteps must be between 100 and 2000");
                return;
            }
            validationConfig_.simulationSteps = val;
        }
        if (body.contains("dt")) {
            double val = body["dt"];
            if (val < 0.01 || val > 1.0) {
                sendError(res, 400, "dt must be between 0.01 and 1.0");
                return;
            }
            validationConfig_.dt = val;
        }
        if (body.contains("improvementThreshold")) {
            double val = body["improvementThreshold"];
            if (val < 0 || val > 50) {
                sendError(res, 400, "improvementThreshold must be between 0 and 50");
                return;
            }
            validationConfig_.improvementThreshold = val;
        }
        if (body.contains("regressionThreshold")) {
            double val = body["regressionThreshold"];
            if (val < 0 || val > 50) {
                sendError(res, 400, "regressionThreshold must be between 0 and 50");
                return;
            }
            validationConfig_.regressionThreshold = val;
        }
        if (body.contains("enabled")) {
            config_.enableValidation = body["enabled"].get<bool>();
        }

        // Update validator if it exists
        if (validator_) {
            validator_->setConfig(validationConfig_);
        }

        LOG_INFO(LogComponent::Optimization, "Validation config updated");
        sendSuccess(res, {{"message", "Validation configuration updated"}});

    } catch (const std::exception& e) {
        sendError(res, 400, e.what());
    }
}

} // namespace api
} // namespace ratms
