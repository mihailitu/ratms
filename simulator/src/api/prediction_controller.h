#ifndef PREDICTION_CONTROLLER_H
#define PREDICTION_CONTROLLER_H

#include "../external/httplib.h"
#include "../prediction/traffic_predictor.h"
#include <nlohmann/json.hpp>
#include <memory>

namespace ratms {
namespace api {

using json = nlohmann::json;

/**
 * @brief PredictionController - REST API controller for traffic prediction endpoints
 *
 * Provides endpoints for:
 * - Current time slot prediction
 * - Future forecast prediction (T+N minutes)
 * - Per-road predictions
 * - Configuration management
 */
class PredictionController {
public:
    explicit PredictionController(
        std::shared_ptr<prediction::TrafficPredictor> predictor
    );
    ~PredictionController() = default;

    // Register routes with the HTTP server
    void registerRoutes(httplib::Server& server);

private:
    // Route handlers
    void handleGetCurrent(const httplib::Request& req, httplib::Response& res);
    void handleGetForecast(const httplib::Request& req, httplib::Response& res);
    void handleGetRoadPrediction(const httplib::Request& req, httplib::Response& res);
    void handleGetConfig(const httplib::Request& req, httplib::Response& res);
    void handleSetConfig(const httplib::Request& req, httplib::Response& res);

    // JSON conversion helpers
    json predictionResultToJson(const prediction::PredictionResult& result);
    json predictedMetricsToJson(const prediction::PredictedMetrics& metrics);
    json configToJson(const prediction::PredictionConfig& config);

    // Error response helper
    void sendError(httplib::Response& res, int status, const std::string& message);
    void sendSuccess(httplib::Response& res, const json& data);

    std::shared_ptr<prediction::TrafficPredictor> predictor_;
};

} // namespace api
} // namespace ratms

#endif // PREDICTION_CONTROLLER_H
