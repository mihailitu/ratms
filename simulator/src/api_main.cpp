#include "api/server.h"
#include "core/simulator.h"
#include "data/storage/database_manager.h"
#include "tests/testintersection.h"
#include "utils/logger.h"
#include <memory>
#include <csignal>
#include <atomic>

std::atomic<bool> shutdown_requested(false);

void signalHandler(int signal) {
    log_info("Received signal %d, shutting down...", signal);
    shutdown_requested = true;
}

int main() {
    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    log_info("Starting RATMS API Server");

    // Initialize database
    auto database = std::make_shared<ratms::data::DatabaseManager>("ratms.db");
    if (!database->initialize()) {
        log_error("Failed to initialize database");
        return 1;
    }

    // Run database migrations
    if (!database->runMigrations("../../database/migrations")) {
        log_error("Failed to run database migrations");
        return 1;
    }

    log_info("Database initialized successfully");

    // Create default network in database
    int network_id = database->createNetwork(
        "Test 4-Way Intersection",
        "Simple 4-way intersection for testing",
        5,  // road count
        1,  // intersection count
        "{}" // config JSON
    );

    log_info("Default network created with ID: %d", network_id);

    // Create simulator instance with test network
    auto simulator = std::make_shared<simulator::Simulator>();
    std::vector<simulator::Road> roadMap = simulator::fourWayIntersectionTest();
    simulator->addRoadNetToMap(roadMap);

    log_info("Simulator initialized with test network");

    // Create and start API server
    ratms::api::Server api_server(8080);
    api_server.setSimulator(simulator);
    api_server.setDatabase(database);
    api_server.start();

    log_info("RATMS API Server running on http://localhost:8080");
    log_info("Press Ctrl+C to stop");

    // Keep server running until shutdown signal
    while (!shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    log_info("Shutting down API server...");
    api_server.stop();

    log_info("RATMS API Server stopped");
    return 0;
}
