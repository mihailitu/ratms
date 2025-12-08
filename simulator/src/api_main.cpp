#include "api/server.h"
#include "core/simulator.h"
#include "data/storage/database_manager.h"
#include "tests/testintersection.h"
#include "utils/logger.h"
#include <atomic>
#include <csignal>
#include <memory>

using namespace ratms;

std::atomic<bool> shutdown_requested(false);

void signalHandler(int signal) {
  LOG_INFO(LogComponent::General, "Received signal {}, shutting down...", signal);
  shutdown_requested = true;
}

int main() {
  // Register signal handlers for graceful shutdown
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  ratms::Logger::init();
  LOG_INFO(LogComponent::General, "Starting RATMS API Server");

  // Initialize database
  auto database = std::make_shared<ratms::data::DatabaseManager>("ratms.db");
  if (!database->initialize()) {
    LOG_ERROR(LogComponent::Database, "Failed to initialize database");
    return 1;
  }

  // Run database migrations
  if (!database->runMigrations("../../database/migrations")) {
    LOG_ERROR(LogComponent::Database, "Failed to run database migrations");
    return 1;
  }

  LOG_INFO(LogComponent::Database, "Database initialized successfully");

  // Create default network in database
  int network_id = database->createNetwork(
      "City Grid 10x10",
      "Realistic 10x10 city grid with 100 intersections and 1000 vehicles",
      360, // road count (approximate, actual is ~360 bidirectional roads)
      100, // intersection count
      "{\"grid_size\": 10, \"block_length\": 300, \"vehicles\": 1000}" // config
                                                                       // JSON
  );

  LOG_INFO(LogComponent::Database, "Default network created with ID: {}", network_id);

  // Create simulator instance with city grid test network
  auto simulator = std::make_shared<simulator::Simulator>();
  std::vector<simulator::Road> roadMap = simulator::cityGridTestMap();
  simulator->addRoadNetToMap(roadMap);

  LOG_INFO(LogComponent::Simulation, "Simulator initialized with test network: {} roads",
           roadMap.size());

  // Create and start API server
  ratms::api::Server api_server(8080);
  api_server.setSimulator(simulator);
  api_server.setDatabase(database);
  api_server.start();

  LOG_INFO(LogComponent::API, "RATMS API Server running on http://localhost:8080");
  LOG_INFO(LogComponent::General, "Press Ctrl+C to stop");

  // Keep server running until shutdown signal
  while (!shutdown_requested) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  LOG_INFO(LogComponent::General, "Shutting down API server...");
  api_server.stop();

  LOG_INFO(LogComponent::General, "RATMS API Server stopped");
  ratms::Logger::shutdown();
  return 0;
}
