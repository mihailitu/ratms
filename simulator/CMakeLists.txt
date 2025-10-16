project(simulator)
cmake_minimum_required(VERSION 3.10)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -W -Wall -Wextra -pedantic -Wno-unknown-pragmas -fPIC -fno-strict-aliasing")

# Include directories for modular structure
include_directories(${CMAKE_SOURCE_DIR}/src)

# Collect source files from each module
file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB UTILS_SOURCES "src/utils/*.cpp")
file(GLOB MAPPING_SOURCES "src/mapping/*.cpp")
file(GLOB OPTIMIZATION_SOURCES "src/optimization/*.cpp")
file(GLOB TEST_SOURCES "src/tests/*.cpp")

# Main simulator executable
set(SRC_LIST
    src/main.cpp
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
    ${MAPPING_SOURCES}
    ${OPTIMIZATION_SOURCES}
    ${TEST_SOURCES}
)

add_executable(${PROJECT_NAME} ${SRC_LIST})

