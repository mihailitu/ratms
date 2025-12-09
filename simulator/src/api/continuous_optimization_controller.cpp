#include "continuous_optimization_controller.h"
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
        } catch (const std::exception& e) {
            LOG_WARN(LogComponent::API, "Failed to parse config from request: {}", e.what());
        }
    }

    start();
    sendSuccess(res, {
        {"message", "Continuous optimization started"},
        {"config", {
            {"optimizationIntervalSeconds", config_.optimizationIntervalSeconds},
            {"transitionDurationSeconds", config_.transitionDurationSeconds},
            {"populationSize", config_.populationSize},
            {"generations", config_.generations}
        }}
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

    sendSuccess(res, {
        {"running", running_.load()},
        {"totalOptimizationRuns", totalOptimizationRuns_.load()},
        {"successfulOptimizations", successfulOptimizations_.load()},
        {"lastImprovementPercent", lastImprovementPercent_.load()},
        {"secondsSinceLastOptimization", secondsSinceLastOptimization},
        {"nextOptimizationIn", std::max(0, config_.optimizationIntervalSeconds - static_cast<int>(secondsSinceLastOptimization))},
        {"activeTransitions", transitionsJson},
        {"config", {
            {"optimizationIntervalSeconds", config_.optimizationIntervalSeconds},
            {"transitionDurationSeconds", config_.transitionDurationSeconds},
            {"populationSize", config_.populationSize},
            {"generations", config_.generations}
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
        {"maxRedTime", config_.maxRedTime}
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
                roadIt->second.setTrafficLightTimings(transition.lane, currentGreen, 3.0, currentRed);
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
    LOG_INFO(LogComponent::Optimization, "Starting optimization cycle");

    // Create test network copy for optimization
    std::vector<simulator::Road> testNetwork = simulator::manyRandomVehicleTestMap(10);

    // Count traffic lights
    size_t totalTrafficLights = 0;
    for (const auto& road : testNetwork) {
        totalTrafficLights += road.getLanesNo();
    }

    // Get current config
    Config config;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        config = config_;
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

    // Run baseline simulation
    std::vector<simulator::Road> baselineNetwork = simulator::manyRandomVehicleTestMap(10);
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
                const simulator::TrafficLight& light = road.getTrafficLight(lane);
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

} // namespace api
} // namespace ratms
