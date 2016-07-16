find_package(SIGC++ REQUIRED)

set(TEST_SRC
    engine/common/pooled_vector.cpp
    engine/common/sequence_view.cpp
    engine/common/stable_index_vector.cpp
    engine/io/utils.cpp
    engine/math/aabb.cpp
    engine/math/algo.cpp
    engine/math/curve.cpp
    engine/math/intersect.cpp
    engine/math/line.cpp
    engine/math/matrix.cpp
    engine/math/mesh.cpp
    engine/math/mixedcurve.cpp
    engine/math/octree.cpp
    engine/math/plane.cpp
    engine/math/quaternion.cpp
    engine/math/rect.cpp
    engine/math/vector.cpp
    engine/render/fancyterraindata.cpp
    engine/sim/objects.cpp
    engine/sim/network.cpp
    engine/sim/networld.cpp
    main.cpp
    )

add_executable(tests ${TEST_SRC})
setup_scc_target(tests)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/catch/include)
target_link_libraries(tests ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(tests Qt5::Network Qt5::Core ffengine-render ffengine-sim ffengine-core)
target_link_libraries(tests sigc++)
