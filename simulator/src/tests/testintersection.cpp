#include "testintersection.h"
#include "testmap.h"
#include "../utils/logger.h"
#include <random>
#include <map>
#include <tuple>

namespace simulator {

/*
 *
 *
 */

std::vector<Road> testIntersectionTest()
{
    unsigned rId = 0;
    std::vector<Road> cmap;
    Road r0(rId++, 1500, 3, 16.7);
    r0.setCardinalCoordinates({10, 1100}, {1510, 1100});
    cmap.push_back(r0);

    Road r00(rId++, 1500, 3, 16.7);
    r00.setCardinalCoordinates({1510, 1000}, {10, 1000});
    cmap.push_back(r00);

    Road r1(rId++, 1500, 3, 16.7);
    r1.setCardinalCoordinates({1610, 1100}, {1610 + 1500, 1100});
    cmap.push_back(r1);

    Road r11(rId++, 1500, 3, 16.7);
    r11.setCardinalCoordinates({1610 + 1500, 1000}, {1610, 1000});
    cmap.push_back(r11);

    Road r2(rId++, 1000, 2, 16.7);
    r2.setCardinalCoordinates({1500, 0}, {1500,1000});
    cmap.push_back(r2);

//    Road r3(2, 1000, 2, 16.7);
//    r3.setCardinalCoordinates({1500, 1100}, {1500,2100});
//    cmap.push_back(r3);



    return cmap;
}

std::vector<Road> singleLaneIntersectionTest()
{
    std::vector<Road> cmap;

    Road r0(0, 500, 1, 20);  // Shorter road for faster transition
    r0.setCardinalCoordinates({0, 100}, {500, 100});
    // Start vehicles closer to end so they transition while light is still green
    r0.addVehicle(Vehicle(350.0, 5.0, 20.0), 0);  // Will reach end in ~7.5 seconds
    r0.addVehicle(Vehicle(380.0, 5.0, 15.0), 0);  // Will reach end in ~8 seconds

    Road r1(1, 1500, 1, 20);
    r1.setCardinalCoordinates({500, 100}, {2000, 100});

    // Connect road 0 lane 0 to road 1 with 100% probability
    r0.addLaneConnection(0, r1.getId(), 1.0);

    cmap.push_back(r0);
    cmap.push_back(r1);

    setDummyMapSize(2500, 500, cmap);

    return cmap;
}

/*
 * Four-way intersection test with probabilistic routing
 *
 * Layout:
 *                  Road 1
 *                    ↓
 *      Road 0  →  [CENTER]  →  Road 2
 *                    ↓
 *                  Road 3
 *
 * - Road 0 (West approach): vehicles can go straight (Road 2) or turn right (Road 3)
 * - Road 1 (North approach): vehicles go straight (Road 3)
 */
std::vector<Road> fourWayIntersectionTest()
{
    std::vector<Road> cmap;

    // Road 0: West approach (0, 1000) → (900, 1000)
    Road r0(0, 400, 1, 20);
    r0.setCardinalCoordinates({0, 1000}, {400, 1000});
    // Add vehicles starting close to end so they reach intersection quickly
    r0.addVehicle(Vehicle(250.0, 5.0, 20.0), 0);
    r0.addVehicle(Vehicle(280.0, 5.0, 18.0), 0);
    r0.addVehicle(Vehicle(310.0, 5.0, 15.0), 0);

    // Road 1: North approach (1000, 0) → (1000, 900)
    Road r1(1, 400, 1, 20);
    r1.setCardinalCoordinates({1000, 0}, {1000, 400});
    r1.addVehicle(Vehicle(250.0, 5.0, 20.0), 0);
    r1.addVehicle(Vehicle(280.0, 5.0, 16.0), 0);

    // Road 2: East exit (1100, 1000) → (2100, 1000)
    Road r2(2, 1000, 1, 20);
    r2.setCardinalCoordinates({1100, 1000}, {2100, 1000});

    // Road 3: South exit (1000, 1100) → (1000, 2100)
    Road r3(3, 1000, 1, 20);
    r3.setCardinalCoordinates({1000, 1100}, {1000, 2100});

    // Road 4: Return from east (2100, 900) → (1100, 900) - parallel to road 2
    Road r4(4, 1000, 1, 20);
    r4.setCardinalCoordinates({2100, 900}, {1100, 900});

    // Road 5: Return from south (900, 2100) → (900, 1100) - parallel to road 3
    Road r5(5, 1000, 1, 20);
    r5.setCardinalCoordinates({900, 2100}, {900, 1100});

    // Set up circular connections
    // Road 0 (West) → Road 2 (East exit) or Road 3 (South exit)
    r0.addLaneConnection(0, r2.getId(), 0.7);
    r0.addLaneConnection(0, r3.getId(), 0.3);

    // Road 1 (North) → Road 3 (South exit)
    r1.addLaneConnection(0, r3.getId(), 1.0);

    // Road 2 (East exit) → Road 4 (Return west)
    r2.addLaneConnection(0, r4.getId(), 1.0);

    // Road 3 (South exit) → Road 5 (Return north)
    r3.addLaneConnection(0, r5.getId(), 1.0);

    // Road 4 (Return from east) → Road 0 (West) or Road 1 (North)
    r4.addLaneConnection(0, r0.getId(), 0.5);
    r4.addLaneConnection(0, r1.getId(), 0.5);

    // Road 5 (Return from south) → Road 0 (West) or Road 1 (North)
    r5.addLaneConnection(0, r0.getId(), 0.5);
    r5.addLaneConnection(0, r1.getId(), 0.5);

    cmap.push_back(r0);
    cmap.push_back(r1);
    cmap.push_back(r2);
    cmap.push_back(r3);
    cmap.push_back(r4);
    cmap.push_back(r5);

    setDummyMapSize(2500, 2500, cmap);

    return cmap;
}

/*
 * Realistic 10x10 city grid with 100 intersections and 1000 vehicles
 *
 * Layout:
 *   - 100 intersections in 10x10 grid
 *   - 440 one-way road segments (~220 bidirectional)
 *   - Block spacing: 300m
 *   - Total area: 3km x 3km
 *   - 1000 vehicles with realistic distribution
 *   - Continuous circular routing for traffic flow
 */
std::vector<Road> cityGridTestMap()
{
    std::vector<Road> cmap;
    std::random_device rd;
    std::mt19937 rng(rd());

    const int GRID_SIZE = 10;           // 10x10 grid
    const double BLOCK_LENGTH = 300.0;  // 300m per block
    const int TOTAL_VEHICLES = 1000;    // Target vehicle count

    // Road configuration
    const unsigned ARTERIAL_LANES = 2;
    const unsigned SECONDARY_LANES = 1;
    const unsigned ARTERIAL_SPEED = 14;  // 50 km/h
    const unsigned SECONDARY_SPEED = 8;  // 30 km/h

    log_info("Generating 10x10 city grid: 100 intersections, ~440 roads, 1000 vehicles");

    // Helper lambda to determine if a road is arterial (perimeter or every 3rd)
    auto isArterial = [](int i, int j, bool isHorizontal) -> bool {
        if (isHorizontal) {
            return i == 0 || i == 9 || i % 3 == 0;
        } else {
            return j == 0 || j == 9 || j % 3 == 0;
        }
    };

    // Store road IDs in a grid for easy lookup during connection phase
    // Format: roadGrid[row][col][direction] where direction: 0=East, 1=South, 2=West, 3=North
    std::map<std::tuple<int, int, int>, unsigned> roadIdGrid;
    unsigned nextRoadId = 0;

    // PHASE 1: Create all horizontal roads (East-West)
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            // Eastbound road from (row, col) to (row, col+1)
            if (col < GRID_SIZE - 1) {
                bool arterial = isArterial(row, col, true);
                unsigned lanes = arterial ? ARTERIAL_LANES : SECONDARY_LANES;
                unsigned speed = arterial ? ARTERIAL_SPEED : SECONDARY_SPEED;

                Road eastbound(nextRoadId, BLOCK_LENGTH, lanes, speed);
                double startX = col * BLOCK_LENGTH;
                double startY = row * BLOCK_LENGTH;
                double endX = (col + 1) * BLOCK_LENGTH;
                double endY = row * BLOCK_LENGTH;
                eastbound.setCardinalCoordinates({startX, startY}, {endX, endY});

                roadIdGrid[{row, col, 0}] = nextRoadId;  // 0 = East
                cmap.push_back(eastbound);
                nextRoadId++;
            }

            // Westbound road from (row, col+1) to (row, col)
            if (col < GRID_SIZE - 1) {
                bool arterial = isArterial(row, col, true);
                unsigned lanes = arterial ? ARTERIAL_LANES : SECONDARY_LANES;
                unsigned speed = arterial ? ARTERIAL_SPEED : SECONDARY_SPEED;

                Road westbound(nextRoadId, BLOCK_LENGTH, lanes, speed);
                double startX = (col + 1) * BLOCK_LENGTH;
                double startY = row * BLOCK_LENGTH + 20;  // Offset 20m for parallel road
                double endX = col * BLOCK_LENGTH;
                double endY = row * BLOCK_LENGTH + 20;
                westbound.setCardinalCoordinates({startX, startY}, {endX, endY});

                roadIdGrid[{row, col, 2}] = nextRoadId;  // 2 = West
                cmap.push_back(westbound);
                nextRoadId++;
            }
        }
    }

    // PHASE 2: Create all vertical roads (North-South)
    for (int col = 0; col < GRID_SIZE; col++) {
        for (int row = 0; row < GRID_SIZE; row++) {
            // Southbound road from (row, col) to (row+1, col)
            if (row < GRID_SIZE - 1) {
                bool arterial = isArterial(row, col, false);
                unsigned lanes = arterial ? ARTERIAL_LANES : SECONDARY_LANES;
                unsigned speed = arterial ? ARTERIAL_SPEED : SECONDARY_SPEED;

                Road southbound(nextRoadId, BLOCK_LENGTH, lanes, speed);
                double startX = col * BLOCK_LENGTH;
                double startY = row * BLOCK_LENGTH;
                double endX = col * BLOCK_LENGTH;
                double endY = (row + 1) * BLOCK_LENGTH;
                southbound.setCardinalCoordinates({startX, startY}, {endX, endY});

                roadIdGrid[{row, col, 1}] = nextRoadId;  // 1 = South
                cmap.push_back(southbound);
                nextRoadId++;
            }

            // Northbound road from (row+1, col) to (row, col)
            if (row < GRID_SIZE - 1) {
                bool arterial = isArterial(row, col, false);
                unsigned lanes = arterial ? ARTERIAL_LANES : SECONDARY_LANES;
                unsigned speed = arterial ? ARTERIAL_SPEED : SECONDARY_SPEED;

                Road northbound(nextRoadId, BLOCK_LENGTH, lanes, speed);
                double startX = col * BLOCK_LENGTH + 20;  // Offset 20m for parallel road
                double startY = (row + 1) * BLOCK_LENGTH;
                double endX = col * BLOCK_LENGTH + 20;
                double endY = row * BLOCK_LENGTH;
                northbound.setCardinalCoordinates({startX, startY}, {endX, endY});

                roadIdGrid[{row, col, 3}] = nextRoadId;  // 3 = North
                cmap.push_back(northbound);
                nextRoadId++;
            }
        }
    }

    log_info("Created %u road segments", nextRoadId);

    // PHASE 3: Set up probabilistic connections at each intersection
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            // For each road arriving at this intersection, connect to departing roads

            // Eastbound arriving roads can go: straight (East), left (North), right (South)
            if (col > 0) {
                auto it = roadIdGrid.find({row, col-1, 0});
                if (it != roadIdGrid.end()) {
                    unsigned arrivalRoad = it->second;

                    // Straight (continue East)
                    if (col < GRID_SIZE - 1) {
                        auto straight = roadIdGrid.find({row, col, 0});
                        if (straight != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, straight->second, 0.5);
                        }
                    }
                    // Left turn (go North)
                    if (row > 0) {
                        auto left = roadIdGrid.find({row-1, col, 3});
                        if (left != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, left->second, 0.25);
                        }
                    }
                    // Right turn (go South)
                    if (row < GRID_SIZE - 1) {
                        auto right = roadIdGrid.find({row, col, 1});
                        if (right != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, right->second, 0.25);
                        }
                    }
                }
            }

            // Westbound arriving roads
            if (col < GRID_SIZE - 1) {
                auto it = roadIdGrid.find({row, col, 2});
                if (it != roadIdGrid.end()) {
                    unsigned arrivalRoad = it->second;

                    // Straight (continue West)
                    if (col > 0) {
                        auto straight = roadIdGrid.find({row, col-1, 2});
                        if (straight != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, straight->second, 0.5);
                        }
                    }
                    // Left turn (go South)
                    if (row < GRID_SIZE - 1) {
                        auto left = roadIdGrid.find({row, col, 1});
                        if (left != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, left->second, 0.25);
                        }
                    }
                    // Right turn (go North)
                    if (row > 0) {
                        auto right = roadIdGrid.find({row-1, col, 3});
                        if (right != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, right->second, 0.25);
                        }
                    }
                }
            }

            // Southbound arriving roads
            if (row > 0) {
                auto it = roadIdGrid.find({row-1, col, 1});
                if (it != roadIdGrid.end()) {
                    unsigned arrivalRoad = it->second;

                    // Straight (continue South)
                    if (row < GRID_SIZE - 1) {
                        auto straight = roadIdGrid.find({row, col, 1});
                        if (straight != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, straight->second, 0.5);
                        }
                    }
                    // Left turn (go East)
                    if (col < GRID_SIZE - 1) {
                        auto left = roadIdGrid.find({row, col, 0});
                        if (left != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, left->second, 0.25);
                        }
                    }
                    // Right turn (go West)
                    if (col > 0) {
                        auto right = roadIdGrid.find({row, col-1, 2});
                        if (right != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, right->second, 0.25);
                        }
                    }
                }
            }

            // Northbound arriving roads
            if (row < GRID_SIZE - 1) {
                auto it = roadIdGrid.find({row, col, 3});
                if (it != roadIdGrid.end()) {
                    unsigned arrivalRoad = it->second;

                    // Straight (continue North)
                    if (row > 0) {
                        auto straight = roadIdGrid.find({row-1, col, 3});
                        if (straight != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, straight->second, 0.5);
                        }
                    }
                    // Left turn (go West)
                    if (col > 0) {
                        auto left = roadIdGrid.find({row, col-1, 2});
                        if (left != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, left->second, 0.25);
                        }
                    }
                    // Right turn (go East)
                    if (col < GRID_SIZE - 1) {
                        auto right = roadIdGrid.find({row, col, 0});
                        if (right != roadIdGrid.end()) {
                            cmap[arrivalRoad].addLaneConnection(0, right->second, 0.25);
                        }
                    }
                }
            }
        }
    }

    log_info("Set up probabilistic routing at all intersections");

    // PHASE 4: Place 1000 vehicles randomly across the network
    std::uniform_int_distribution<unsigned> roadDist(0, cmap.size() - 1);
    std::uniform_real_distribution<double> posDist(10.0, BLOCK_LENGTH - 30.0);
    std::uniform_real_distribution<double> speedDist(5.0, 12.0);
    std::uniform_real_distribution<double> aggressivityDist(0.4, 0.6);

    int vehiclesPlaced = 0;
    int attempts = 0;
    const int MAX_ATTEMPTS = TOTAL_VEHICLES * 3;

    while (vehiclesPlaced < TOTAL_VEHICLES && attempts < MAX_ATTEMPTS) {
        unsigned roadIdx = roadDist(rng);
        double position = posDist(rng);
        double velocity = speedDist(rng);
        double aggressivity = aggressivityDist(rng);

        // Vehicle constructor: (position, velocity, aggressivity)
        Vehicle v(position, velocity, aggressivity);

        // Try to add to a random lane on the selected road
        if (cmap[roadIdx].getLanesNo() > 0) {
            std::uniform_int_distribution<unsigned> laneDist(0, cmap[roadIdx].getLanesNo() - 1);
            unsigned lane = laneDist(rng);

            if (cmap[roadIdx].addVehicle(v, lane)) {
                vehiclesPlaced++;
            }
        }
        attempts++;
    }

    log_info("Placed %d vehicles across the network (target: %d)", vehiclesPlaced, TOTAL_VEHICLES);

    // Set dummy map size for visualization bounds
    setDummyMapSize(GRID_SIZE * BLOCK_LENGTH, GRID_SIZE * BLOCK_LENGTH, cmap);

    return cmap;
}

} // namespace simulator
