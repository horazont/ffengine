set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/")

set(ENGINE_HEADERS
  ffengine/common/pooled_vector.hpp
  ffengine/common/qtutils.hpp
  ffengine/common/resource.hpp
  ffengine/common/sequence_view.hpp
  ffengine/common/stable_index_vector.hpp
  ffengine/common/types.hpp
  ffengine/common/utils.hpp
  ffengine/io/common.hpp
  ffengine/io/errors.hpp
  ffengine/io/filestream.hpp
  ffengine/io/filesystem.hpp
  ffengine/io/log.hpp
  ffengine/io/mount.hpp
  ffengine/io/stdiostream.hpp
  ffengine/io/stream.hpp
  ffengine/io/stream_operators.inc.hpp
  ffengine/io/utils.hpp
  ffengine/math/aabb.hpp
  ffengine/math/algo.hpp
  ffengine/math/curve.hpp
  ffengine/math/intersect.hpp
  ffengine/math/line.hpp
  ffengine/math/matrix.hpp
  ffengine/math/mesh.hpp
  ffengine/math/mixedcurve.hpp
  ffengine/math/octree.hpp
  ffengine/math/perlin.hpp
  ffengine/math/plane.hpp
  ffengine/math/quaternion.hpp
  ffengine/math/ray.hpp
  ffengine/math/rect.hpp
  ffengine/math/sphere.hpp
  ffengine/math/tikz.hpp
  ffengine/math/variadic_initializer.inc.hpp
  ffengine/math/vector.hpp
  )

set(ENGINE_SRC
  src/common/pooled_vector.cpp
  src/common/qtutils.cpp
  src/common/resource.cpp
  src/common/sequence_view.cpp
  src/common/stable_index_vector.cpp
  src/common/utils.cpp
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
  src/math/aabb.cpp
  src/math/algo.cpp
  src/math/curve.cpp
  src/math/intersect.cpp
  src/math/line.cpp
  src/math/matrix.cpp
  src/math/mesh.cpp
  src/math/mixedcurve.cpp
  src/math/octree.cpp
  src/math/perlin.cpp
  src/math/plane.cpp
  src/math/quaternion.cpp
  src/math/ray.cpp
  src/math/rect.cpp
  src/math/sphere.cpp
  src/math/tikz.cpp
  src/math/vector.cpp
  )

add_library(ffengine-core STATIC ${ENGINE_SRC} ${ENGINE_HEADERS})
setup_scc_target(ffengine-core)

target_link_libraries(
  ffengine-core
  Qt5::Core
  sigc++)
target_link_libraries(ffengine-core "atomic")
target_include_directories(
  ffengine-core
  PUBLIC ${INCLUDE_DIRS})
