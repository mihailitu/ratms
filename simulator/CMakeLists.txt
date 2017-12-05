project(simulator)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "-std=c++1z ${CMAKE_CXX_FLAGS} -g -W -Wall -Wextra -pedantic -Wno-unknown-pragmas -fPIC -fno-strict-aliasing")
aux_source_directory(src SRC_LIST)
aux_source_directory(src/tests SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

