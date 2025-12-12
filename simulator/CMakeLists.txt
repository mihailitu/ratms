cmake_minimum_required(VERSION 3.25)
project(simulator)

include(FetchContent)

# Fetch spdlog for logging
FetchContent_Declare(
  spdlog
  SYSTEM
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
file(GLOB METRICS_SOURCES "src/metrics/*.cpp")
file(GLOB PREDICTION_SOURCES "src/prediction/*.cpp")
file(GLOB VALIDATION_SOURCES "src/validation/*.cpp")
file(GLOB FEED_SOURCES "src/feed/*.cpp")
file(GLOB TEST_SOURCES "src/tests/*.cpp")

# Shared library sources (used by both executables)
set(SHARED_SOURCES
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
    ${MAPPING_SOURCES}
    ${OPTIMIZATION_SOURCES}
    ${PREDICTION_SOURCES}
    ${VALIDATION_SOURCES}
    ${METRICS_SOURCES}
    ${DATA_SOURCES}
    ${FEED_SOURCES}
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
find_package(OpenMP)

# Link libraries for API server
target_link_libraries(api_server
    spdlog::spdlog
    Threads::Threads
    SQLite::SQLite3
)

# Enable OpenMP for parallel road updates
if(OpenMP_CXX_FOUND)
    target_link_libraries(api_server OpenMP::OpenMP_CXX)
    message(STATUS "OpenMP found - parallel simulation enabled")
else()
    message(STATUS "OpenMP not found - simulation will run single-threaded")
endif()

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

# Enable OpenMP for parallel fitness evaluation
if(OpenMP_CXX_FOUND)
    target_link_libraries(ga_optimizer OpenMP::OpenMP_CXX)
endif()

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
file(GLOB FIXTURE_SOURCES "src/unit_tests/fixtures/*.cpp")
file(GLOB HELPER_SOURCES "src/unit_tests/helpers/*.cpp")
file(GLOB INTEGRATION_TEST_SOURCES "src/unit_tests/integration/*.cpp")

# Unit test executable (fast tests - no API server)
add_executable(unit_tests
    ${UNIT_TEST_SOURCES}
    ${FIXTURE_SOURCES}
    ${HELPER_SOURCES}
    ${SHARED_SOURCES}
)

target_link_libraries(unit_tests
    GTest::gtest_main
    GTest::gmock
    spdlog::spdlog
    Threads::Threads
    SQLite::SQLite3
)

# Integration test executable (includes API tests)
add_executable(integration_tests
    ${INTEGRATION_TEST_SOURCES}
    ${FIXTURE_SOURCES}
    ${HELPER_SOURCES}
    ${API_SOURCES}
    ${SHARED_SOURCES}
)

target_link_libraries(integration_tests
    GTest::gtest_main
    GTest::gmock
    spdlog::spdlog
    Threads::Threads
    SQLite::SQLite3
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)

# Custom targets for running tests
add_custom_target(test_unit
    COMMAND unit_tests
    DEPENDS unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
)

add_custom_target(test_integration
    COMMAND integration_tests
    DEPENDS integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests"
)

add_custom_target(test_all
    COMMAND unit_tests && integration_tests
    DEPENDS unit_tests integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests"
)
