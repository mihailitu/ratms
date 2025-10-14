# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

RATMS is a traffic simulation project built in C++20 that implements the Intelligent Driver Model (IDM) for realistic vehicle behavior simulation. The simulator outputs data that is visualized using Python scripts with matplotlib animations.

## Build System

The project uses both CMake and GNU Make build systems:

### CMake Build (Recommended)
```bash
cd simulator
mkdir -p build
cd build
cmake ..
make
```

### GNU Make Build
```bash
cd simulator
make clean
make
```

The executable is output as `simulator` in the working directory.

## Running the Simulator

### Basic execution:
```bash
cd simulator
./build/simulator  # or ./simulator if using GNU make
```

### Running with Python visualization:
```bash
cd simulator
./run.sh
```

The `run.sh` script:
1. Builds the simulator using GNU make
2. Runs the simulator (outputs `.dat` files)
3. Copies output to `pytest/` directory
4. Launches Python visualization script

## Core Architecture

### C++ Simulator (`simulator/src/`)

**Main Components:**

- **Simulator** (`simulator.h/cpp`): Main simulation engine
  - Manages the city road network as a `CityMap` (map of roadID to Road objects)
  - Runs discrete time-step simulation (configurable via `Config::DT`)
  - Serializes simulation state to output files for visualization
  - Two serialization versions: v1 (simple) and v2 (with traffic lights)

- **Road** (`road.h/cpp`): Represents one-way road segments
  - Roads are unidirectional segments between intersections/traffic lights
  - Contains multiple lanes (right-to-left: lane 0 is rightmost/slow, lane N is leftmost/fast)
  - Each lane has its own traffic light and connection probabilities to other roads
  - Manages vehicle collections per lane using `std::list<Vehicle>`
  - Handles lane change logic and road-to-road transitions
  - Geographic (lat/lon) and Cartesian (x,y in meters) coordinate systems

- **Vehicle** (`vehicle.h/cpp`): Individual vehicle entities
  - Implements IDM (Intelligent Driver Model) physics
  - Configurable aggressivity factor affects acceleration, desired velocity, safe distance
  - Tracks position, velocity, acceleration, and itinerary
  - Can also represent traffic lights (zero/negative length, zero velocity)
  - Each vehicle has unique ID (auto-generated)

- **TrafficLight** (`trafficlight.h/cpp`): Traffic signal management
  - States: Red (R), Yellow (Y), Green (G)
  - One traffic light per lane per road

- **Config** (`config.h/cpp`): Simulation configuration
  - `DT`: Time step for simulation updates
  - `simulationTime`: Total simulation iterations
  - Output file paths for serialization

**Test Maps** (`src/tests/testmap.cpp`):
Pre-built road networks for testing:
- `sigleVehicleTestMap()`: Free road acceleration test
- `followingVehicleTestMap()`: Vehicle following behavior
- `laneChangeTest()`: Multi-lane vehicle interactions
- `semaphoreTest()`: Traffic light behavior
- `testIntersectionTest()`: Intersection logic
- `manyRandomVehicleTestMap(int)`: Stress testing with random vehicles

### Python Visualization (`simulator/pytest/`)

**Visualization Scripts:**

- **simple_road.py**: Animated 2D visualization of single road simulation
  - Reads serialized `.dat` files from C++ simulator
  - Displays vehicles as colored dots (blue=normal, red=selected)
  - Shows real-time speed, acceleration, lane info
  - Interactive: click vehicles to inspect their data
  - Animation interval synced with simulation DT

- **munich.py**: OpenStreetMap data fetcher
  - Uses `osmnx` library to download real city street networks
  - Converts to GeoJSON format for map import

- **mapgenerator.py**: Generates map configurations

**Map Import (`src/newmap.hpp`, `src/newroad.hpp`):**
- JSON-based map loading using nlohmann/json library
- Supports real-world map data (GeoJSON from OSM)

## Development Workflow

### Creating New Test Scenarios

1. Add test function to `simulator/src/tests/testmap.cpp`
2. Construct Road objects with desired parameters (length, lanes, maxSpeed)
3. Add Vehicle objects to roads at specific positions/speeds
4. Set up road connections and traffic lights
5. Update `main.cpp` to call your test function
6. Build and run

### Modifying Vehicle Behavior

Vehicle dynamics are governed by IDM parameters in `vehicle.h/cpp`:
- `v0`: Desired velocity (influenced by road max speed and aggressivity)
- `T`: Safe time headway
- `a`: Maximum acceleration
- `b`: Desired deceleration
- `s0`: Minimum safe distance
- `delta`: Acceleration exponent
- `aggressivity`: Driver behavior modifier (0.5 = normal, <0.5 = cautious, >0.5 = aggressive)

Modify `getNewAcceleration()` to change following/free-road behavior.

### Output Data Format

**Version 2 (current):**
- Road map file: `roadID | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes_no`
- Vehicle data file: `time roadID lanes_no [light_states] [x v a lane]...`
  - Each line represents one road at one timestep
  - Vehicle data repeats for each vehicle: position, velocity, acceleration, lane

## Key Concepts

### Intelligent Driver Model (IDM)
The simulator implements IDM for realistic acceleration/deceleration:
- Free road: vehicles accelerate to desired velocity
- Following: maintains safe distance based on current speed and relative velocity
- See: https://en.wikipedia.org/wiki/Intelligent_driver_model

### Road Network Design
- Roads are one-way segments (bidirectional real roads = 2 separate Road objects)
- Each lane can connect to different destination roads with probability weights
- Traffic lights control each lane independently
- Vehicles choose next road based on connection probabilities

### Lane Changing
- Vehicles attempt lane changes when blocked by slower leader
- Safety checks: sufficient gap with new leader and new follower
- Acceleration incentive: only change if it improves acceleration
- Distance thresholds defined in `Road::maxChangeLaneDist` and `Road::minChangeLaneDist`

## Known Limitations & TODOs

From `simulator/TODO.txt`:
- Road connections between roads need refinement
- Lane selection when multiple lanes exist on a section
- Right turn on red light (yield to left traffic) not implemented
- Pedestrian obstacles for right turns not implemented
- Consider linked lists instead of vectors for sorted vehicle storage

## Dependencies

**C++:**
- C++20 compiler (g++)
- nlohmann/json (header-only, included in `src/nlohmann/`)

**Python:**
- numpy
- matplotlib
- osmnx (for map data fetching)
- shapely (for geometry operations)
- tqdm (progress bars)

## File Structure

```
simulator/
├── src/                    # C++ source files
│   ├── main.cpp           # Entry point
│   ├── simulator.*        # Simulation engine
│   ├── road.*             # Road model
│   ├── vehicle.*          # Vehicle/IDM implementation
│   ├── trafficlight.*     # Traffic light logic
│   ├── config.*           # Configuration constants
│   ├── logger.*           # Logging utilities
│   ├── newmap.hpp         # JSON map loader
│   ├── tests/             # Test scenarios
│   └── nlohmann/          # JSON library
├── pytest/                # Python visualization scripts
├── build/                 # CMake build directory (generated)
├── bin/                   # Alternative build output (generated)
├── CMakeLists.txt         # CMake configuration
└── makefile               # GNU Make configuration
```
