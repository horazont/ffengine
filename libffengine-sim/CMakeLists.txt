set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/")

set(CMAKE_INCLUDE_CURRENT_DIRS ON)

find_package(Protobuf REQUIRED)
find_package(LibNoise REQUIRED)

set(ENGINE_HEADERS
  ffengine/sim/fluid.hpp
  ffengine/sim/fluid_base.hpp
  ffengine/sim/fluid_native.hpp
  ffengine/sim/network.hpp
  ffengine/sim/networld.hpp
  ffengine/sim/objects.hpp
  ffengine/sim/server.hpp
  ffengine/sim/signals.hpp
  ffengine/sim/terrain.hpp
  ffengine/sim/world.hpp
  ffengine/sim/world_ops.hpp
  )

set(ENGINE_SRC
  src/sim/fluid.cpp
  src/sim/fluid_base.cpp
  src/sim/fluid_native.cpp
  src/sim/network.cpp
  src/sim/networld.cpp
  src/sim/objects.cpp
  src/sim/server.cpp
  src/sim/signals.cpp
  src/sim/terrain.cpp
  src/sim/world.cpp
  src/sim/world_ops.cpp
  )

set(ENGINE_PROTOS
  proto/types.proto
  proto/world_command.proto
  proto/netserver_control.proto
  )

PROTOBUF_GENERATE_CPP(
  PROTO_SRCS
  PROTO_HDRS
  ${ENGINE_PROTOS})

qt5_generate_moc(ffengine/sim/server.hpp server.moc)

add_library(ffengine-sim STATIC ${ENGINE_SRC} ${ENGINE_HEADERS} ${ENGINE_PROTOS} ${PROTO_SRCS} server.moc)
setup_scc_target(ffengine-sim)

target_link_libraries(ffengine-sim
  ffengine-core
  ${PROTOBUF_LIBRARIES}
  ${LIBNOISE_LIBRARIES}
  sigc++
  sig11
  Qt5::Network
  Qt5::Core)
target_link_libraries(ffengine-sim "atomic")
target_include_directories(
  ffengine-sim
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(
  ffengine-sim
  PUBLIC ${INCLUDE_DIRS})
target_include_directories(
  ffengine-sim
  SYSTEM PUBLIC ${LIBNOISE_INCLUDE_DIR})

