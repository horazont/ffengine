project(scc)
cmake_minimum_required(VERSION 3.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(SIGC++ REQUIRED)
find_package(Threads REQUIRED)

function(setup_scc_target NAME)
  set_property(TARGET ${NAME} PROPERTY CXX_STANDARD 14)
  set_property(TARGET ${NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
  target_compile_options(${NAME} PRIVATE -Wall -Wextra)
  target_compile_options(${NAME} PRIVATE $<$<CONFIG:DEBUG>:-ggdb -O2>)
  target_compile_options(${NAME} PRIVATE $<$<CONFIG:RELEASE>:-O3 -NDEBUG>)
endfunction(setup_scc_target)


add_subdirectory(shader-preprocessor)
add_subdirectory(sig11)
add_subdirectory(libffengine-core)
add_subdirectory(libffengine-sim)
add_subdirectory(libffengine-render)
add_subdirectory(tests)
