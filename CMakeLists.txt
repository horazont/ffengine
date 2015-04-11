project(scc)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_CXX_FLAGS "-std=c++1y -Wall -Wextra -g -O2")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

find_package(Qt5Core REQUIRED)

add_subdirectory(surface_mesh)
add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(tests)
