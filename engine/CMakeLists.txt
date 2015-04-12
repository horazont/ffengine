set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/")

set(ENGINE_HEADERS
  engine/common/resource.hpp
  engine/common/types.hpp
  engine/common/utils.hpp
  engine/gl/2darray.hpp
  engine/gl/array.hpp
  engine/gl/fbo.hpp
  engine/gl/ibo.hpp
  engine/gl/material.hpp
  engine/gl/mesh.hpp
  engine/gl/object.hpp
  engine/gl/shader.hpp
  engine/gl/texture.hpp
  engine/gl/ubo.hpp
  engine/gl/ubo_tuple_utils.inc.hpp
  engine/gl/ubo_type_wrappers.inc.hpp
  engine/gl/util.hpp
  engine/gl/vao.hpp
  engine/gl/vbo.hpp
  engine/io/common.hpp
  engine/io/errors.hpp
  engine/io/filestream.hpp
  engine/io/filesystem.hpp
  engine/io/log.hpp
  engine/io/mount.hpp
  engine/io/stdiostream.hpp
  engine/io/stream.hpp
  engine/io/stream_operators.inc.hpp
  engine/io/utils.hpp
  engine/math/algo.hpp
  engine/math/intersect.hpp
  engine/math/matrix.hpp
  engine/math/perlin.hpp
  engine/math/rect.hpp
  engine/math/shapes.hpp
  engine/math/variadic_initializer.inc.hpp
  engine/math/vector.hpp
  engine/render/camera.hpp
  engine/render/fancyterrain.hpp
  engine/render/grid.hpp
  engine/render/pointer.hpp
  engine/render/rendergraph.hpp
  engine/render/scenegraph.hpp
  engine/sim/terrain.hpp
  )

set(ENGINE_SRC
  src/common/resource.cpp
  src/common/utils.cpp
  src/gl/2darray.cpp
  src/gl/array.cpp
  src/gl/fbo.cpp
  src/gl/ibo.cpp
  src/gl/material.cpp
  src/gl/object.cpp
  src/gl/shader.cpp
  src/gl/texture.cpp
  src/gl/ubo.cpp
  src/gl/util.cpp
  src/gl/vao.cpp
  src/gl/vbo.cpp
  src/io/common.cpp
  src/io/errors.cpp
  src/io/filestream.cpp
  src/io/filesystem.cpp
  src/io/log.cpp
  src/io/mount.cpp
  src/io/stdiostream.cpp
  src/io/stream.cpp
  src/io/stream_operators.inc.cpp
  src/io/utils.cpp
  src/math/algo.cpp
  src/math/intersect.cpp
  src/math/matrix.cpp
  src/math/perlin.cpp
  src/math/rect.cpp
  src/math/shapes.cpp
  src/math/vector.cpp
  src/render/camera.cpp
  src/render/fancyterrain.cpp
  src/render/grid.cpp
  src/render/pointer.cpp
  src/render/rendergraph.cpp
  src/render/scenegraph.cpp
  src/sim/terrain.cpp
  )

include_directories(${INCLUDE_DIRS})

add_library(engine STATIC ${ENGINE_SRC})
target_link_libraries(engine surface_mesh)
target_link_libraries(engine Qt5::Core)
target_link_libraries(engine sigc++)
set_property(
  TARGET engine
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  ${INCLUDE_DIRS}
  )
set_property(
  TARGET engine
  PROPERTY INTERFACE_COMPILE_DEFINITIONS)

add_custom_target(dummy_engine_files SOURCES ${ENGINE_HEADERS})
