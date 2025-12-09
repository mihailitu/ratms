/**
 * osm_import - Standalone tool to import OpenStreetMap data
 *
 * Converts OSM XML files to JSON format for use with the RATMS traffic simulator.
 *
 * Usage:
 *   ./osm_import <input.osm> <output.json> [network_name]
 *
 * Example:
 *   ./osm_import data/osm/schwabing.osm data/maps/munich.json "Munich Schwabing"
 */

#include "mapping/osm_importer.h"

#include <iostream>
#include <string>
#include <chrono>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <input.osm> <output.json> [network_name]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  input.osm     Path to OpenStreetMap XML file" << std::endl;
    std::cout << "  output.json   Path for output JSON network file" << std::endl;
    std::cout << "  network_name  Optional name for the network (default: 'Imported Network')" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << progName << " data/osm/schwabing.osm data/maps/munich.json \"Munich Schwabing\"" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    std::string networkName = (argc >= 4) ? argv[3] : "Imported Network";

    std::cout << "=== OSM Import Tool ===" << std::endl;
    std::cout << "Input:  " << inputFile << std::endl;
    std::cout << "Output: " << outputFile << std::endl;
    std::cout << "Name:   " << networkName << std::endl;
    std::cout << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        osm::OSMImporter importer;

        // Import from OSM file
        std::cout << "Step 1: Importing from OSM file..." << std::endl;
        auto roads = importer.importFromFile(inputFile);

        // Save to JSON
        std::cout << std::endl << "Step 2: Saving to JSON..." << std::endl;
        importer.saveToJson(roads, outputFile, networkName);

        // Print statistics
        auto stats = importer.getStats();
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << std::endl;
        std::cout << "=== Import Complete ===" << std::endl;
        std::cout << "Statistics:" << std::endl;
        std::cout << "  OSM nodes read:      " << stats.nodesRead << std::endl;
        std::cout << "  OSM ways read:       " << stats.waysRead << std::endl;
        std::cout << "  Intersections found: " << stats.intersectionsFound << std::endl;
        std::cout << "  Road segments:       " << stats.roadSegmentsCreated << std::endl;
        std::cout << "  Connections:         " << stats.connectionsCreated << std::endl;
        std::cout << "  Time elapsed:        " << duration.count() << " ms" << std::endl;
        std::cout << std::endl;
        std::cout << "Output saved to: " << outputFile << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
