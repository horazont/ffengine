set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/")

set(ENGINE_HEADERS
  ffengine/common/qtutils.hpp
  ffengine/common/resource.hpp
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
  ffengine/math/algo.hpp
  ffengine/math/curve.hpp
  ffengine/math/intersect.hpp
  ffengine/math/matrix.hpp
  ffengine/math/octree.hpp
  ffengine/math/perlin.hpp
  ffengine/math/quaternion.hpp
  ffengine/math/rect.hpp
  ffengine/math/shapes.hpp
  ffengine/math/variadic_initializer.inc.hpp
  ffengine/math/vector.hpp
  )

set(ENGINE_SRC
  src/common/qtutils.cpp
  src/common/resource.cpp
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
  src/math/algo.cpp
  src/math/curve.cpp
  src/math/intersect.cpp
  src/math/matrix.cpp
  src/math/octree.cpp
  src/math/perlin.cpp
  src/math/quaternion.cpp
  src/math/rect.cpp
  src/math/shapes.cpp
  src/math/vector.cpp
  )

add_library(ffengine-core STATIC ${ENGINE_SRC} ${PROTO_SRCS})
setup_scc_target(ffengine-core)

target_link_libraries(
  ffengine-core
  Qt5::Core
  sigc++)
target_link_libraries(ffengine-core "atomic")
target_include_directories(
  ffengine-core
  PUBLIC ${INCLUDE_DIRS})

add_custom_target(
  dummy_ffengine-core_files
  SOURCES ${ENGINE_HEADERS})
