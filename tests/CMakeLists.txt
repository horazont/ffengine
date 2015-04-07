include_directories(${CMAKE_CURRENT_SOURCE_DIR}/catch/include)

set(TEST_SRC
  engine/math/matrix.cpp
  engine/math/ray.cpp
  engine/math/shapes.cpp
  engine/math/vector.cpp
  engine/render/quadterrain.cpp
  engine/sim/quadterrain.cpp
  main.cpp
  )

add_executable(tests ${TEST_SRC})
target_link_libraries(tests engine)
