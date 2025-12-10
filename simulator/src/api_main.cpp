#include "api/server.h"
#include "core/simulator.h"
#include "data/storage/database_manager.h"
#include "mapping/network_loader.h"
#include "tests/testintersection.h"
#include "utils/logger.h"
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <cstring>

using namespace ratms;

std::atomic<bool> shutdown_requested(false);

void signalHandler(int signal) {
  LOG_INFO(LogComponent::General, "Received signal {}, shutting down...", signal);
  shutdown_requested = true;
}

void printUsage(const char* progName) {
  std::cout << "Usage: " << progName << " [options]" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --network <file.json>  Load road network from JSON file" << std::endl;
  std::cout << "  --port <port>          Server port (default: 8080)" << std::endl;
  std::cout << "  --help                 Show this help message" << std::endl;
  std::cout << std::endl;
  std::cout << "If no network file is specified, uses default test network." << std::endl;
}

int main(int argc, char* argv[]) {
  // Initialize random seed for traffic light phase randomization
  std::srand(static_cast<unsigned>(std::time(nullptr)));

  std::string networkFile;
  int port = 8080;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--network") == 0 && i + 1 < argc) {
      networkFile = argv[++i];
    } else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--help") == 0) {
      printUsage(argv[0]);
      return 0;
    }
  }
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

  // Create simulator instance
  auto simulator = std::make_shared<simulator::Simulator>();
  std::vector<simulator::Road> roadMap;

  if (!networkFile.empty()) {
    // Load network from JSON file
    LOG_INFO(LogComponent::Simulation, "Loading network from: {}", networkFile);
    try {
      roadMap = mapping::NetworkLoader::loadFromJson(networkFile);
      LOG_INFO(LogComponent::Simulation, "Loaded {} roads from JSON", roadMap.size());
    } catch (const std::exception& e) {
      LOG_ERROR(LogComponent::Simulation, "Failed to load network: {}", e.what());
      return 1;
    }
  } else {
    // Use default test network
    roadMap = simulator::cityGridTestMap();
    LOG_INFO(LogComponent::Simulation, "Using default test network");
  }

  simulator->addRoadNetToMap(roadMap);
  LOG_INFO(LogComponent::Simulation, "Simulator initialized with {} roads", roadMap.size());

  // Create and start API server
  ratms::api::Server api_server(port);
  api_server.setSimulator(simulator);
  api_server.setDatabase(database);
  api_server.start();

  // Initialize default spawn rates for entry roads (10 vehicles/minute)
  // This enables automatic vehicle spawning when loading map files
  api_server.initializeDefaultSpawnRates(10.0);

  LOG_INFO(LogComponent::API, "RATMS API Server running on http://localhost:{}", port);
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
