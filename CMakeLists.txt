project(scc)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

find_package(Qt5Core REQUIRED)
find_package(SIGC++ REQUIRED)

# add_subdirectory(surface_mesh)
add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(tests)
