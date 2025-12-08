#include "optimization_controller.h"
#include "../tests/testmap.h"
#include "../utils/logger.h"
#include <chrono>
#include <sstream>

namespace ratms {
namespace api {

using json = nlohmann::json;
using namespace ratms;

// Validation helper for GA parameters
struct ValidationResult {
    bool valid;
    std::string error;
};

static ValidationResult validateGAParams(const simulator::GeneticAlgorithm::Parameters& params,
                                          int simulationSteps, double dt) {
    // Population bounds
    if (params.populationSize < 2 || params.populationSize > 1000)
        return {false, "populationSize must be between 2 and 1000"};

    // Generation bounds
    if (params.generations < 1 || params.generations > 10000)
        return {false, "generations must be between 1 and 10000"};

    // Probability bounds [0, 1]
    if (params.mutationRate < 0.0 || params.mutationRate > 1.0)
        return {false, "mutationRate must be between 0.0 and 1.0"};
    if (params.crossoverRate < 0.0 || params.crossoverRate > 1.0)
        return {false, "crossoverRate must be between 0.0 and 1.0"};
    if (params.elitismRate < 0.0 || params.elitismRate > 1.0)
        return {false, "elitismRate must be between 0.0 and 1.0"};

    // Tournament size must not exceed population
    if (params.tournamentSize < 1 || params.tournamentSize > params.populationSize)
        return {false, "tournamentSize must be between 1 and populationSize"};

    // Timing bounds - must be positive and min <= max
    if (params.minGreenTime <= 0 || params.maxGreenTime <= 0)
        return {false, "green times must be positive"};
    if (params.minGreenTime > params.maxGreenTime)
        return {false, "minGreenTime must be <= maxGreenTime"};
    if (params.minRedTime <= 0 || params.maxRedTime <= 0)
        return {false, "red times must be positive"};
    if (params.minRedTime > params.maxRedTime)
        return {false, "minRedTime must be <= maxRedTime"};

    // Simulation bounds
    if (simulationSteps < 1 || simulationSteps > 100000)
        return {false, "simulationSteps must be between 1 and 100000"};
    if (dt < 0.01 || dt > 1.0)
        return {false, "dt must be between 0.01 and 1.0"};

    return {true, ""};
}

OptimizationController::OptimizationController(std::shared_ptr<ratms::data::DatabaseManager> dbManager)
    : dbManager_(dbManager) {
    // Load optimization history from database on startup
    loadOptimizationHistory();
}

OptimizationController::~OptimizationController() {
    // Wait for all optimization threads to complete
    std::lock_guard<std::mutex> lock(runsMutex_);
    for (auto& [id, run] : activeRuns_) {
        if (run->optimizationThread && run->optimizationThread->joinable()) {
            run->isRunning = false;
            run->optimizationThread->join();
        }
    }
}

void OptimizationController::registerRoutes(httplib::Server& server) {
    // POST /api/optimization/start - Start new optimization run
    server.Post("/api/optimization/start", [this](const httplib::Request& req, httplib::Response& res) {
        handleStartOptimization(req, res);
    });

    // GET /api/optimization/status/:id - Get optimization status
    server.Get("/api/optimization/status/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetStatus(req, res);
    });

    // GET /api/optimization/results/:id - Get optimization results
    server.Get("/api/optimization/results/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetResults(req, res);
    });

    // GET /api/optimization/history - List all optimization runs
    server.Get("/api/optimization/history", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetHistory(req, res);
    });

    // POST /api/optimization/stop/:id - Stop running optimization
    server.Post("/api/optimization/stop/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        handleStopOptimization(req, res);
    });
}

void OptimizationController::handleStartOptimization(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    try {
        LOG_INFO(LogComponent::Optimization, "Received optimization start request");
        // Parse request body
        json requestBody = json::parse(req.body);

        // Extract parameters
        simulator::GeneticAlgorithm::Parameters gaParams;
        gaParams.populationSize = requestBody.value("populationSize", 30);
        gaParams.generations = requestBody.value("generations", 50);
        gaParams.mutationRate = requestBody.value("mutationRate", 0.15);
        gaParams.mutationStdDev = requestBody.value("mutationStdDev", 5.0);
        gaParams.crossoverRate = requestBody.value("crossoverRate", 0.8);
        gaParams.tournamentSize = requestBody.value("tournamentSize", 3);
        gaParams.elitismRate = requestBody.value("elitismRate", 0.1);
        gaParams.minGreenTime = requestBody.value("minGreenTime", 10.0);
        gaParams.maxGreenTime = requestBody.value("maxGreenTime", 60.0);
        gaParams.minRedTime = requestBody.value("minRedTime", 10.0);
        gaParams.maxRedTime = requestBody.value("maxRedTime", 60.0);
        gaParams.seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());

        int simulationSteps = requestBody.value("simulationSteps", 1000);
        double dt = requestBody.value("dt", 0.1);
        int networkId = requestBody.value("networkId", 1);

        // Validate parameters
        auto validation = validateGAParams(gaParams, simulationSteps, dt);
        if (!validation.valid) {
            LOG_WARN(LogComponent::Optimization, "Invalid GA parameters: {}", validation.error);
            json error = {
                {"success", false},
                {"error", validation.error}
            };
            res.set_content(error.dump(), "application/json");
            res.status = 400;
            return;
        }

        // Create optimization run
        int runId = createOptimizationRun(gaParams, simulationSteps, dt, networkId);

        // Get the run
        std::shared_ptr<OptimizationRun> run;
        {
            std::lock_guard<std::mutex> lock(runsMutex_);
            run = activeRuns_[runId];
        }

        // Start optimization in background thread (no detach - destructor handles join)
        run->optimizationThread = std::make_unique<std::thread>(
            &OptimizationController::runOptimizationBackground, this, run, networkId
        );

        // Return response
        json response = {
            {"success", true},
            {"runId", runId},
            {"message", "Optimization started"},
            {"run", runToJson(run, false)}
        };

        res.set_content(response.dump(), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        json error = {
            {"success", false},
            {"error", e.what()}
        };
        res.set_content(error.dump(), "application/json");
        res.status = 400;
    }
}

void OptimizationController::handleGetStatus(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    try {
        int runId = std::stoi(req.matches[1]);
        LOG_DEBUG(LogComponent::Optimization, "Status request for run {}", runId);

        std::lock_guard<std::mutex> lock(runsMutex_);
        auto it = activeRuns_.find(runId);
        if (it == activeRuns_.end()) {
            json error = {
                {"success", false},
                {"error", "Optimization run not found"}
            };
            res.set_content(error.dump(), "application/json");
            res.status = 404;
            return;
        }

        json response = {
            {"success", true},
            {"run", runToJson(it->second, false)}
        };

        res.set_content(response.dump(), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        json error = {
            {"success", false},
            {"error", e.what()}
        };
        res.set_content(error.dump(), "application/json");
        res.status = 400;
    }
}

void OptimizationController::handleGetResults(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    try {
        int runId = std::stoi(req.matches[1]);
        LOG_DEBUG(LogComponent::Optimization, "Results request for run {}", runId);

        std::lock_guard<std::mutex> lock(runsMutex_);
        auto it = activeRuns_.find(runId);
        if (it == activeRuns_.end()) {
            json error = {
                {"success", false},
                {"error", "Optimization run not found"}
            };
            res.set_content(error.dump(), "application/json");
            res.status = 404;
            return;
        }

        if (it->second->status != "completed") {
            json error = {
                {"success", false},
                {"error", "Optimization not yet completed"}
            };
            res.set_content(error.dump(), "application/json");
            res.status = 400;
            return;
        }

        json response = {
            {"success", true},
            {"run", runToJson(it->second, true)}  // Include full history
        };

        res.set_content(response.dump(), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        json error = {
            {"success", false},
            {"error", e.what()}
        };
        res.set_content(error.dump(), "application/json");
        res.status = 400;
    }
}

void OptimizationController::handleGetHistory(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    try {
        std::lock_guard<std::mutex> lock(runsMutex_);

        json runs = json::array();
        for (const auto& [id, run] : activeRuns_) {
            runs.push_back(runToJson(run, false));
        }

        json response = {
            {"success", true},
            {"runs", runs},
            {"total", runs.size()}
        };

        res.set_content(response.dump(), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        json error = {
            {"success", false},
            {"error", e.what()}
        };
        res.set_content(error.dump(), "application/json");
        res.status = 500;
    }
}

void OptimizationController::handleStopOptimization(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();
    try {
        int runId = std::stoi(req.matches[1]);
        LOG_INFO(LogComponent::Optimization, "Stop request for run {}", runId);

        std::lock_guard<std::mutex> lock(runsMutex_);
        auto it = activeRuns_.find(runId);
        if (it == activeRuns_.end()) {
            json error = {
                {"success", false},
                {"error", "Optimization run not found"}
            };
            res.set_content(error.dump(), "application/json");
            res.status = 404;
            return;
        }

        if (it->second->isRunning) {
            it->second->isRunning = false;
            it->second->status = "stopped";
        }

        json response = {
            {"success", true},
            {"message", "Optimization stopped"}
        };

        res.set_content(response.dump(), "application/json");
        res.status = 200;

    } catch (const std::exception& e) {
        json error = {
            {"success", false},
            {"error", e.what()}
        };
        res.set_content(error.dump(), "application/json");
        res.status = 400;
    }
}

int OptimizationController::createOptimizationRun(const simulator::GeneticAlgorithm::Parameters& params,
                                                   int simulationSteps, double dt, int networkId) {
    std::lock_guard<std::mutex> lock(runsMutex_);

    // Create database record
    ratms::data::DatabaseManager::OptimizationRunRecord dbRecord;
    dbRecord.network_id = networkId;
    dbRecord.status = "pending";
    dbRecord.population_size = params.populationSize;
    dbRecord.generations = params.generations;
    dbRecord.mutation_rate = params.mutationRate;
    dbRecord.crossover_rate = params.crossoverRate;
    dbRecord.elitism_rate = params.elitismRate;
    dbRecord.min_green_time = params.minGreenTime;
    dbRecord.max_green_time = params.maxGreenTime;
    dbRecord.min_red_time = params.minRedTime;
    dbRecord.max_red_time = params.maxRedTime;
    dbRecord.simulation_steps = simulationSteps;
    dbRecord.dt = dt;
    dbRecord.started_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    dbRecord.created_by = "web_dashboard";
    dbRecord.notes = "";

    int dbRunId = dbManager_->createOptimizationRun(dbRecord);
    if (dbRunId < 0) {
        throw std::runtime_error("Failed to create optimization run in database");
    }

    // Create in-memory run
    auto run = std::make_shared<OptimizationRun>();
    run->id = dbRunId;  // Use database ID
    run->status = "pending";
    run->gaParams = params;
    run->simulationSteps = simulationSteps;
    run->dt = dt;
    run->startedAt = dbRecord.started_at;
    run->currentGeneration = 0;
    run->isRunning = false;

    activeRuns_[dbRunId] = run;

    // Update nextRunId to be at least dbRunId + 1
    if (dbRunId >= nextRunId_) {
        nextRunId_ = dbRunId + 1;
    }

    return dbRunId;
}

void OptimizationController::runOptimizationBackground(std::shared_ptr<OptimizationRun> run, int networkId) {
    int dbRunId = run->id;
    TIMED_SCOPE(LogComponent::Optimization, "optimization_run");
    try {
        LOG_INFO(LogComponent::Optimization, "Starting optimization run {} with {} generations",
                 dbRunId, run->gaParams.generations);
        run->status = "running";
        run->isRunning = true;
        dbManager_->updateOptimizationRunStatus(dbRunId, "running");

        // Create test network
        std::vector<simulator::Road> testNetwork = simulator::manyRandomVehicleTestMap(10);

        // Count traffic lights
        size_t totalTrafficLights = 0;
        for (const auto& road : testNetwork) {
            totalTrafficLights += road.getLanesNo();
        }

        // Run baseline simulation
        simulator::FitnessEvaluator evaluator(run->simulationSteps, run->dt);
        std::vector<simulator::Road> baselineNetwork = simulator::manyRandomVehicleTestMap(10);
        simulator::Simulator baseSim;
        for (auto& road : baselineNetwork) {
            baseSim.addRoadToMap(road);
        }

        simulator::MetricsCollector baselineCollector;
        std::vector<simulator::RoadTransition> pendingTransitions;
        for (int step = 0; step < run->simulationSteps; ++step) {
            pendingTransitions.clear();
            for (auto& roadPair : baseSim.cityMap) {
                roadPair.second.update(run->dt, baseSim.cityMap, pendingTransitions);
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
                baselineCollector.collectMetrics(baseSim.cityMap, run->dt);
            }
        }

        simulator::SimulationMetrics baselineMetrics = baselineCollector.getMetrics();
        if (baselineMetrics.sampleCount > 0) {
            baselineMetrics.averageQueueLength /= baselineMetrics.sampleCount;
            baselineMetrics.averageSpeed /= baselineMetrics.sampleCount;
        }
        run->baselineFitness = baselineMetrics.getFitness();

        // Create fitness function
        auto fitnessFunc = [&evaluator, &testNetwork](const simulator::Chromosome& chromosome) -> double {
            std::vector<simulator::Road> networkCopy = testNetwork;
            return evaluator.evaluate(chromosome, networkCopy);
        };

        // Create and run GA
        simulator::GeneticAlgorithm ga(run->gaParams, fitnessFunc);
        ga.initializePopulation(totalTrafficLights);

        // Run evolution with progress tracking
        for (size_t gen = 0; gen < run->gaParams.generations && run->isRunning; ++gen) {
            run->currentGeneration = gen;
            // Evolution happens inside the GA - we'd need to refactor GA to expose per-generation hooks
        }

        simulator::Chromosome bestSolution = ga.evolve();

        // Store results
        run->bestChromosome = bestSolution;
        run->bestFitness = bestSolution.fitness;
        run->fitnessHistory = ga.getFitnessHistory();
        run->improvementPercent = ((run->baselineFitness - run->bestFitness) / run->baselineFitness) * 100.0;
        run->status = "completed";
        run->completedAt = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;

        // Save to database
        saveOptimizationResults(run, dbRunId);

    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::Optimization, "Optimization run {} failed: {}", dbRunId, e.what());
        run->status = "failed";
        run->isRunning = false;
        dbManager_->updateOptimizationRunStatus(dbRunId, "failed");
    }

    run->isRunning = false;
    LOG_INFO(LogComponent::Optimization, "Optimization run {} finished with status: {}", dbRunId, run->status);
}

void OptimizationController::loadOptimizationHistory() {
    try {
        std::lock_guard<std::mutex> lock(runsMutex_);

        // Load all completed optimization runs from database
        std::vector<ratms::data::DatabaseManager::OptimizationRunRecord> dbRuns =
            dbManager_->getAllOptimizationRuns();

        for (const auto& dbRun : dbRuns) {
            // Only load completed runs (skip running/pending as they're stale)
            if (dbRun.status != "completed") {
                continue;
            }

            auto run = std::make_shared<OptimizationRun>();
            run->id = dbRun.id;
            run->status = dbRun.status;
            run->startedAt = dbRun.started_at;
            run->completedAt = dbRun.completed_at;
            run->baselineFitness = dbRun.baseline_fitness;
            run->bestFitness = dbRun.best_fitness;
            run->improvementPercent = dbRun.improvement_percent;
            run->simulationSteps = dbRun.simulation_steps;
            run->dt = dbRun.dt;

            // Restore GA parameters
            run->gaParams.populationSize = dbRun.population_size;
            run->gaParams.generations = dbRun.generations;
            run->gaParams.mutationRate = dbRun.mutation_rate;
            run->gaParams.crossoverRate = dbRun.crossover_rate;
            run->gaParams.elitismRate = dbRun.elitism_rate;
            run->gaParams.minGreenTime = dbRun.min_green_time;
            run->gaParams.maxGreenTime = dbRun.max_green_time;
            run->gaParams.minRedTime = dbRun.min_red_time;
            run->gaParams.maxRedTime = dbRun.max_red_time;

            // Load fitness history from generations
            std::vector<ratms::data::DatabaseManager::OptimizationGenerationRecord> generations =
                dbManager_->getOptimizationGenerations(dbRun.id);

            for (const auto& gen : generations) {
                run->fitnessHistory.push_back(gen.best_fitness);
            }

            // Load best solution
            ratms::data::DatabaseManager::OptimizationSolutionRecord solution =
                dbManager_->getBestOptimizationSolution(dbRun.id);

            if (solution.id > 0) {
                // Parse chromosome JSON
                try {
                    json chromosomeJson = json::parse(solution.chromosome_json);
                    for (const auto& geneJson : chromosomeJson) {
                        simulator::TrafficLightTiming timing;
                        timing.greenTime = geneJson["greenTime"];
                        timing.redTime = geneJson["redTime"];
                        run->bestChromosome.genes.push_back(timing);
                    }
                    run->bestChromosome.fitness = solution.fitness;
                } catch (const std::exception& e) {
                    LOG_ERROR(LogComponent::Optimization, "Failed to parse chromosome JSON for run {}: {}", dbRun.id, e.what());
                }
            }

            activeRuns_[run->id] = run;

            // Update nextRunId
            if (run->id >= nextRunId_) {
                nextRunId_ = run->id + 1;
            }
        }

        LOG_INFO(LogComponent::Optimization, "Loaded {} optimization runs from database", activeRuns_.size());

    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::Optimization, "Failed to load optimization history: {}", e.what());
    }
}

void OptimizationController::saveOptimizationResults(std::shared_ptr<OptimizationRun> run, int dbRunId) {
    try {
        // Update optimization run with results
        long durationSeconds = run->completedAt - run->startedAt;
        bool success = dbManager_->completeOptimizationRun(
            dbRunId,
            run->completedAt,
            durationSeconds,
            run->baselineFitness,
            run->bestFitness,
            run->improvementPercent
        );

        if (!success) {
            LOG_ERROR(LogComponent::Database, "Failed to update optimization run {}", dbRunId);
            return;
        }

        // Save fitness history as generation records
        std::vector<ratms::data::DatabaseManager::OptimizationGenerationRecord> generations;
        for (size_t i = 0; i < run->fitnessHistory.size(); ++i) {
            ratms::data::DatabaseManager::OptimizationGenerationRecord genRecord;
            genRecord.optimization_run_id = dbRunId;
            genRecord.generation_number = static_cast<int>(i);
            genRecord.best_fitness = run->fitnessHistory[i];
            genRecord.average_fitness = run->fitnessHistory[i];  // GA doesn't track avg, use best
            genRecord.worst_fitness = run->fitnessHistory[i];    // GA doesn't track worst, use best
            genRecord.timestamp = run->startedAt + static_cast<long>(i);
            generations.push_back(genRecord);
        }

        if (!generations.empty()) {
            bool genSuccess = dbManager_->insertOptimizationGenerationsBatch(generations);
            if (!genSuccess) {
                LOG_ERROR(LogComponent::Database, "Failed to insert generation records for run {}", dbRunId);
            }
        }

        // Save best solution
        ratms::data::DatabaseManager::OptimizationSolutionRecord solutionRecord;
        solutionRecord.optimization_run_id = dbRunId;
        solutionRecord.is_best_solution = true;
        solutionRecord.fitness = run->bestFitness;
        solutionRecord.traffic_light_count = static_cast<int>(run->bestChromosome.genes.size());
        solutionRecord.created_at = run->completedAt;

        // Convert chromosome to JSON
        json chromosomeJson = json::array();
        for (const auto& gene : run->bestChromosome.genes) {
            chromosomeJson.push_back({
                {"greenTime", gene.greenTime},
                {"redTime", gene.redTime}
            });
        }
        solutionRecord.chromosome_json = chromosomeJson.dump();

        int solutionId = dbManager_->insertOptimizationSolution(solutionRecord);
        if (solutionId < 0) {
            LOG_ERROR(LogComponent::Database, "Failed to insert solution for run {}", dbRunId);
        } else {
            LOG_INFO(LogComponent::Database, "Saved optimization results for run {} (solution ID: {})", dbRunId, solutionId);
        }

    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::Database, "Exception while saving optimization results: {}", e.what());
    }
}

json OptimizationController::runToJson(const std::shared_ptr<OptimizationRun>& run, bool includeFullHistory) {
    json j = {
        {"id", run->id},
        {"status", run->status},
        {"parameters", {
            {"populationSize", run->gaParams.populationSize},
            {"generations", run->gaParams.generations},
            {"mutationRate", run->gaParams.mutationRate},
            {"crossoverRate", run->gaParams.crossoverRate},
            {"elitismRate", run->gaParams.elitismRate},
            {"simulationSteps", run->simulationSteps},
            {"dt", run->dt}
        }},
        {"progress", {
            {"currentGeneration", run->currentGeneration.load()},
            {"totalGenerations", run->gaParams.generations},
            {"percentComplete", (static_cast<double>(run->currentGeneration) / run->gaParams.generations) * 100.0}
        }},
        {"startedAt", run->startedAt}
    };

    if (run->status == "completed") {
        j["results"] = {
            {"baselineFitness", run->baselineFitness},
            {"bestFitness", run->bestFitness},
            {"improvementPercent", run->improvementPercent},
            {"completedAt", run->completedAt},
            {"durationSeconds", run->completedAt - run->startedAt}
        };

        // Best chromosome
        json chromosomeJson = json::array();
        for (const auto& gene : run->bestChromosome.genes) {
            chromosomeJson.push_back({
                {"greenTime", gene.greenTime},
                {"redTime", gene.redTime}
            });
        }
        j["results"]["bestChromosome"] = chromosomeJson;

        if (includeFullHistory) {
            j["results"]["fitnessHistory"] = run->fitnessHistory;
        } else {
            // Just include last 10 points for status queries
            size_t historySize = run->fitnessHistory.size();
            if (historySize > 10) {
                j["results"]["fitnessHistorySample"] = std::vector<double>(
                    run->fitnessHistory.end() - 10, run->fitnessHistory.end()
                );
            } else {
                j["results"]["fitnessHistorySample"] = run->fitnessHistory;
            }
        }
    }

    return j;
}

} // namespace api
} // namespace ratms
