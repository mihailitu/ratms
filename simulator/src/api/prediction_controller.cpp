#include "prediction_controller.h"
#include "../utils/logger.h"
#include <regex>

namespace ratms {
namespace api {

PredictionController::PredictionController(
    std::shared_ptr<prediction::TrafficPredictor> predictor
) : predictor_(predictor) {
    LOG_INFO(LogComponent::API, "PredictionController initialized");
}

void PredictionController::registerRoutes(httplib::Server& server) {
    // GET /api/prediction/current - Current time slot prediction
    server.Get("/api/prediction/current",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetCurrent(req, res);
        });

    // GET /api/prediction/forecast - Future forecast prediction
    server.Get("/api/prediction/forecast",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetForecast(req, res);
        });

    // GET /api/prediction/road/:id - Per-road prediction
    server.Get(R"(/api/prediction/road/(\d+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetRoadPrediction(req, res);
        });

    // GET /api/prediction/config - Get configuration
    server.Get("/api/prediction/config",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetConfig(req, res);
        });

    // POST /api/prediction/config - Update configuration
    server.Post("/api/prediction/config",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleSetConfig(req, res);
        });

    LOG_INFO(LogComponent::API, "Prediction routes registered: /api/prediction/*");
}

void PredictionController::handleGetCurrent(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!predictor_) {
        sendError(res, 503, "Prediction service not initialized");
        return;
    }

    try {
        auto result = predictor_->predictCurrent();
        sendSuccess(res, predictionResultToJson(result));
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Error in current prediction: {}", e.what());
        sendError(res, 500, std::string("Prediction failed: ") + e.what());
    }
}

void PredictionController::handleGetForecast(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!predictor_) {
        sendError(res, 503, "Prediction service not initialized");
        return;
    }

    // Parse horizon parameter (default: 30 minutes)
    int horizon = 30;
    if (req.has_param("horizon")) {
        try {
            horizon = std::stoi(req.get_param_value("horizon"));
        } catch (const std::exception& e) {
            sendError(res, 400, "Invalid horizon parameter: must be an integer");
            return;
        }
    }

    // Validate horizon range
    auto config = predictor_->getConfig();
    if (horizon < config.minHorizonMinutes || horizon > config.maxHorizonMinutes) {
        sendError(res, 400,
            "Horizon must be between " + std::to_string(config.minHorizonMinutes) +
            " and " + std::to_string(config.maxHorizonMinutes) + " minutes");
        return;
    }

    try {
        auto result = predictor_->predictForecast(horizon);
        sendSuccess(res, predictionResultToJson(result));
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Error in forecast prediction: {}", e.what());
        sendError(res, 500, std::string("Prediction failed: ") + e.what());
    }
}

void PredictionController::handleGetRoadPrediction(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!predictor_) {
        sendError(res, 503, "Prediction service not initialized");
        return;
    }

    // Extract road ID from URL
    int roadId;
    try {
        roadId = std::stoi(req.matches[1]);
    } catch (const std::exception& e) {
        sendError(res, 400, "Invalid road ID");
        return;
    }

    // Parse horizon parameter (default: 30 minutes)
    int horizon = 30;
    if (req.has_param("horizon")) {
        try {
            horizon = std::stoi(req.get_param_value("horizon"));
        } catch (const std::exception& e) {
            sendError(res, 400, "Invalid horizon parameter: must be an integer");
            return;
        }
    }

    try {
        auto result = predictor_->predictRoad(roadId, horizon);
        if (!result.has_value()) {
            sendError(res, 404, "Road not found: " + std::to_string(roadId));
            return;
        }

        auto [targetDay, targetSlot] = prediction::TrafficPredictor::getFutureTimeSlot(horizon);

        json response;
        response["roadId"] = roadId;
        response["horizonMinutes"] = horizon;
        response["targetDayOfWeek"] = targetDay;
        response["targetTimeSlot"] = targetSlot;
        response["targetTimeSlotString"] = prediction::TrafficPredictor::timeSlotToString(targetSlot);
        response["prediction"] = predictedMetricsToJson(result.value());

        // Add breakdown info
        json breakdown;
        breakdown["patternVehicleCount"] = result->patternVehicleCount;
        breakdown["currentVehicleCount"] = result->currentVehicleCount;
        auto config = predictor_->getConfig();
        breakdown["patternWeight"] = config.patternWeight;
        breakdown["currentWeight"] = config.currentWeight;
        response["breakdown"] = breakdown;

        sendSuccess(res, response);
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Error in road prediction: {}", e.what());
        sendError(res, 500, std::string("Prediction failed: ") + e.what());
    }
}

void PredictionController::handleGetConfig(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!predictor_) {
        sendError(res, 503, "Prediction service not initialized");
        return;
    }

    auto config = predictor_->getConfig();
    sendSuccess(res, configToJson(config));
}

void PredictionController::handleSetConfig(const httplib::Request& req, httplib::Response& res) {
    REQUEST_SCOPE();

    if (!predictor_) {
        sendError(res, 503, "Prediction service not initialized");
        return;
    }

    try {
        json body = json::parse(req.body);
        auto config = predictor_->getConfig();

        // Update config fields if present
        if (body.contains("horizonMinutes")) {
            config.horizonMinutes = body["horizonMinutes"].get<int>();
        }
        if (body.contains("patternWeight")) {
            config.patternWeight = body["patternWeight"].get<double>();
        }
        if (body.contains("currentWeight")) {
            config.currentWeight = body["currentWeight"].get<double>();
        }
        if (body.contains("minSamplesForFullConfidence")) {
            config.minSamplesForFullConfidence = body["minSamplesForFullConfidence"].get<int>();
        }
        if (body.contains("cacheDurationSeconds")) {
            config.cacheDurationSeconds = body["cacheDurationSeconds"].get<int>();
        }

        // Validate
        if (config.patternWeight < 0.0 || config.patternWeight > 1.0) {
            sendError(res, 400, "patternWeight must be between 0.0 and 1.0");
            return;
        }
        if (config.currentWeight < 0.0 || config.currentWeight > 1.0) {
            sendError(res, 400, "currentWeight must be between 0.0 and 1.0");
            return;
        }
        if (config.horizonMinutes < config.minHorizonMinutes ||
            config.horizonMinutes > config.maxHorizonMinutes) {
            sendError(res, 400,
                "horizonMinutes must be between " + std::to_string(config.minHorizonMinutes) +
                " and " + std::to_string(config.maxHorizonMinutes));
            return;
        }

        predictor_->setConfig(config);

        LOG_INFO(LogComponent::API, "Prediction config updated: horizon={}min, patternWeight={:.2f}",
                 config.horizonMinutes, config.patternWeight);

        sendSuccess(res, configToJson(predictor_->getConfig()));

    } catch (const json::exception& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Error updating config: {}", e.what());
        sendError(res, 500, std::string("Failed to update config: ") + e.what());
    }
}

json PredictionController::predictionResultToJson(const prediction::PredictionResult& result) {
    json j;
    j["predictionTimestamp"] = result.predictionTimestamp;
    j["targetTimestamp"] = result.targetTimestamp;
    j["horizonMinutes"] = result.horizonMinutes;
    j["targetDayOfWeek"] = result.targetDayOfWeek;
    j["targetTimeSlot"] = result.targetTimeSlot;
    j["targetTimeSlotString"] = result.targetTimeSlotString;
    j["averageConfidence"] = result.averageConfidence;

    json predictions = json::array();
    for (const auto& pm : result.roadPredictions) {
        predictions.push_back(predictedMetricsToJson(pm));
    }
    j["roadPredictions"] = predictions;

    j["config"] = configToJson(result.configUsed);

    return j;
}

json PredictionController::predictedMetricsToJson(const prediction::PredictedMetrics& metrics) {
    json j;
    j["roadId"] = metrics.roadId;
    j["vehicleCount"] = metrics.vehicleCount;
    j["queueLength"] = metrics.queueLength;
    j["avgSpeed"] = metrics.avgSpeed;
    j["flowRate"] = metrics.flowRate;
    j["confidence"] = metrics.confidence;
    j["historicalSampleCount"] = metrics.historicalSampleCount;
    j["hasCurrentData"] = metrics.hasCurrentData;
    j["hasHistoricalPattern"] = metrics.hasHistoricalPattern;
    return j;
}

json PredictionController::configToJson(const prediction::PredictionConfig& config) {
    json j;
    j["horizonMinutes"] = config.horizonMinutes;
    j["minHorizonMinutes"] = config.minHorizonMinutes;
    j["maxHorizonMinutes"] = config.maxHorizonMinutes;
    j["patternWeight"] = config.patternWeight;
    j["currentWeight"] = config.currentWeight;
    j["minSamplesForFullConfidence"] = config.minSamplesForFullConfidence;
    j["cacheDurationSeconds"] = config.cacheDurationSeconds;
    return j;
}

void PredictionController::sendError(httplib::Response& res, int status, const std::string& message) {
    json response;
    response["success"] = false;
    response["error"] = message;
    res.status = status;
    res.set_content(response.dump(2), "application/json");
}

void PredictionController::sendSuccess(httplib::Response& res, const json& data) {
    json response;
    response["success"] = true;
    response["data"] = data;
    res.status = 200;
    res.set_content(response.dump(2), "application/json");
}

} // namespace api
} // namespace ratms
