include_directories(${CMAKE_CURRENT_SOURCE_DIR}/catch/include)

find_package(SIGC++ REQUIRED)

set(TEST_SRC
  engine/io/utils.cpp
  engine/math/algo.cpp
  engine/math/intersect.cpp
  engine/math/matrix.cpp
  engine/math/quaternion.cpp
  engine/math/rect.cpp
  engine/math/shapes.cpp
  engine/math/vector.cpp
  engine/render/fancyterraindata.cpp
  engine/render/octree.cpp
  engine/sim/objects.cpp
  main.cpp
  )

add_executable(tests ${TEST_SRC})
target_link_libraries(tests ffengine-core ffengine-render ffengine-sim)
target_link_libraries(tests sigc++)
