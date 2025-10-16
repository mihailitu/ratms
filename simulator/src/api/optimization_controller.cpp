#include "optimization_controller.h"
#include "../tests/testmap.h"
#include <chrono>
#include <sstream>

namespace ratms {
namespace api {

using json = nlohmann::json;

OptimizationController::OptimizationController(std::shared_ptr<ratms::data::DatabaseManager> dbManager)
    : dbManager_(dbManager) {
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
    try {
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

        // Create optimization run
        int runId = createOptimizationRun(gaParams, simulationSteps, dt, networkId);

        // Get the run
        std::shared_ptr<OptimizationRun> run;
        {
            std::lock_guard<std::mutex> lock(runsMutex_);
            run = activeRuns_[runId];
        }

        // Start optimization in background thread
        run->optimizationThread = std::make_unique<std::thread>(
            &OptimizationController::runOptimizationBackground, this, run, networkId
        );
        run->optimizationThread->detach();

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
    try {
        int runId = std::stoi(req.matches[1]);

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
    try {
        int runId = std::stoi(req.matches[1]);

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
    try {
        int runId = std::stoi(req.matches[1]);

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

    int runId = nextRunId_++;
    auto run = std::make_shared<OptimizationRun>();
    run->id = runId;
    run->status = "pending";
    run->gaParams = params;
    run->simulationSteps = simulationSteps;
    run->dt = dt;
    run->startedAt = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    run->currentGeneration = 0;
    run->isRunning = false;

    activeRuns_[runId] = run;

    return runId;
}

void OptimizationController::runOptimizationBackground(std::shared_ptr<OptimizationRun> run, int networkId) {
    try {
        run->status = "running";
        run->isRunning = true;

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
        saveOptimizationResults(run);

    } catch (const std::exception& e) {
        run->status = "failed";
        run->isRunning = false;
    }

    run->isRunning = false;
}

void OptimizationController::saveOptimizationResults(std::shared_ptr<OptimizationRun> run) {
    // TODO: Implement database persistence
    // This would save to the optimization_runs, optimization_generations, and optimization_solutions tables
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
