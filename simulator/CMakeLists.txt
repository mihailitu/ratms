cmake_minimum_required(VERSION 3.25)
project(simulator)

include(FetchContent)

# Fetch spdlog for logging
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Fetch Google Test for unit testing
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# Prevent overriding the parent project's compiler/linker settings (Windows)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -W -Wall -Wextra -pedantic -Wno-unknown-pragmas -fPIC -fno-strict-aliasing")

# Include directories for modular structure
include_directories(${CMAKE_SOURCE_DIR}/src)

# Collect source files from each module
file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB UTILS_SOURCES "src/utils/*.cpp")
# Note: osm_importer.cpp is excluded from MAPPING_SOURCES because it requires expat
# It's only included in the osm_import target
file(GLOB MAPPING_SOURCES "src/mapping/*.cpp")
list(FILTER MAPPING_SOURCES EXCLUDE REGEX "osm_importer\\.cpp$")
file(GLOB OPTIMIZATION_SOURCES "src/optimization/*.cpp")
file(GLOB API_SOURCES "src/api/*.cpp")
file(GLOB DATA_SOURCES "src/data/storage/*.cpp")
file(GLOB TEST_SOURCES "src/tests/*.cpp")

# Shared library sources (used by both executables)
set(SHARED_SOURCES
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
    ${MAPPING_SOURCES}
    ${OPTIMIZATION_SOURCES}
    ${DATA_SOURCES}
    ${TEST_SOURCES}
)

# Main simulator executable (original CLI version)
set(SIMULATOR_SOURCES
    src/main.cpp
    ${SHARED_SOURCES}
)

add_executable(${PROJECT_NAME} ${SIMULATOR_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog Threads::Threads SQLite::SQLite3)

# API server executable
set(API_SERVER_SOURCES
    src/api_main.cpp
    ${API_SOURCES}
    ${SHARED_SOURCES}
)

add_executable(api_server ${API_SERVER_SOURCES})

# Find required libraries
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

# Link libraries for API server
target_link_libraries(api_server
    spdlog::spdlog
    Threads::Threads
    SQLite::SQLite3
)

# GA Optimizer executable
set(GA_OPTIMIZER_SOURCES
    src/ga_optimizer.cpp
    ${SHARED_SOURCES}
)

add_executable(ga_optimizer ${GA_OPTIMIZER_SOURCES})

# Link libraries for GA optimizer
target_link_libraries(ga_optimizer
    spdlog::spdlog
    Threads::Threads
    SQLite::SQLite3
)

# =============================================================================
# OSM Import Tool
# =============================================================================

# Find expat for XML parsing
find_package(EXPAT REQUIRED)

# OSM Import executable
set(OSM_IMPORT_SOURCES
    src/tools/osm_import_main.cpp
    src/mapping/osm_importer.cpp
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
)

add_executable(osm_import ${OSM_IMPORT_SOURCES})

target_link_libraries(osm_import
    spdlog::spdlog
    Threads::Threads
    EXPAT::EXPAT
)

# =============================================================================
# Unit Tests
# =============================================================================

# Enable testing
enable_testing()

# Collect unit test source files
file(GLOB UNIT_TEST_SOURCES "src/unit_tests/*.cpp")

# Unit test executable
add_executable(unit_tests
    ${UNIT_TEST_SOURCES}
    ${SHARED_SOURCES}
)

target_link_libraries(unit_tests
    GTest::gtest_main
    GTest::gmock
    spdlog::spdlog
    Threads::Threads
    SQLite::SQLite3
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
