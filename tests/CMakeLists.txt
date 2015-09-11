include_directories(${CMAKE_CURRENT_SOURCE_DIR}/catch/include)

find_package(SIGC++ REQUIRED)

set(TEST_SRC
  engine/io/utils.cpp
  engine/math/algo.cpp
  engine/math/curve.cpp
  engine/math/intersect.cpp
  engine/math/matrix.cpp
  engine/math/octree.cpp
  engine/math/quaternion.cpp
  engine/math/rect.cpp
  engine/math/shapes.cpp
  engine/math/vector.cpp
  engine/render/fancyterraindata.cpp
  engine/sim/objects.cpp
  engine/sim/networld.cpp
  main.cpp
  )

add_executable(tests ${TEST_SRC})
target_link_libraries(tests ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(tests Qt5::Network Qt5::Core ffengine-render ffengine-sim ffengine-core)
target_link_libraries(tests sigc++)
