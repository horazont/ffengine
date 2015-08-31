set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/")

set(ENGINE_HEADERS
  ffengine/gl/2darray.hpp
  ffengine/gl/array.hpp
  ffengine/gl/fbo.hpp
  ffengine/gl/ibo.hpp
  ffengine/gl/material.hpp
  ffengine/gl/mesh.hpp
  ffengine/gl/object.hpp
  ffengine/gl/resource.hpp
  ffengine/gl/shader.hpp
  ffengine/gl/texture.hpp
  ffengine/gl/ubo.hpp
  ffengine/gl/ubo_tuple_utils.inc.hpp
  ffengine/gl/ubo_type_wrappers.inc.hpp
  ffengine/gl/util.hpp
  ffengine/gl/vao.hpp
  ffengine/gl/vbo.hpp
  ffengine/render/aabb.hpp
  ffengine/render/camera.hpp
  ffengine/render/curve.hpp
  ffengine/render/fancyterrain.hpp
  ffengine/render/fancyterraindata.hpp
  ffengine/render/fluid.hpp
  ffengine/render/frustum.hpp
  ffengine/render/grid.hpp
  ffengine/render/oct_sphere.hpp
  ffengine/render/overlay.hpp
  ffengine/render/pointer.hpp
  ffengine/render/rendergraph.hpp
  ffengine/render/scenegraph.hpp
  )

set(ENGINE_SRC
  src/gl/2darray.cpp
  src/gl/array.cpp
  src/gl/fbo.cpp
  src/gl/ibo.cpp
  src/gl/material.cpp
  src/gl/object.cpp
  src/gl/resource.cpp
  src/gl/shader.cpp
  src/gl/texture.cpp
  src/gl/ubo.cpp
  src/gl/util.cpp
  src/gl/vao.cpp
  src/gl/vbo.cpp
  src/render/aabb.cpp
  src/render/camera.cpp
  src/render/curve.cpp
  src/render/fancyterrain.cpp
  src/render/fancyterraindata.cpp
  src/render/fluid.cpp
  src/render/frustum.cpp
  src/render/grid.cpp
  src/render/oct_sphere.cpp
  src/render/overlay.cpp
  src/render/pointer.cpp
  src/render/rendergraph.cpp
  src/render/scenegraph.cpp
  )

add_library(ffengine-render STATIC ${ENGINE_SRC})

target_link_libraries(
  ffengine-render
  ffengine-core
  ffengine-sim
  Qt5::Core
  sigc++
  spp)
target_include_directories(
  ffengine-render
  PUBLIC ${INCLUDE_DIRS})

add_custom_target(
  dummy_ffengine-render_files
  SOURCES ${ENGINE_HEADERS}
  )
