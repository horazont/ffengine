#include <catch.hpp>

#include "engine/render/quadterrain.hpp"

using namespace engine;
using namespace sim;
/*

#define VECTORS std::vector<Vector3f> position, normal, tangent; std::vector<unsigned int> indicies
#define NEIGHBOUR_LOD std::array<unsigned int, 4> neighbour_lod({1, 1, 1, 1})

template <typename element_t>
std::vector<element_t> resolve_indicies(
        const std::vector<unsigned int> &indicies,
        const std::vector<element_t> &elements)
{
    std::vector<element_t> result;
    for (auto index: indicies)
    {
        result.emplace_back(elements[index]);
    }
    return result;
}


TEST_CASE("render/quadterrain/create_geometry/leaf_at_root")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 128, 1);
    VECTORS;
    NEIGHBOUR_LOD;

    create_geometry(&node, &node,
                    1, neighbour_lod,
                    indicies,
                    position, normal, tangent);

    std::vector<Vector3f> expected_positions({
                                                 Vector3f(0, 0, 1),
                                                 Vector3f(0, 127, 1),
                                                 Vector3f(127, 0, 1),
                                                 Vector3f(127, 0, 1),
                                                 Vector3f(0, 127, 1),
                                                 Vector3f(127, 127, 1)
                                             });
    CHECK(expected_positions == resolve_indicies(indicies, position));
}

TEST_CASE("render/quadterrain/create_geometry/normal_node_at_root")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 128, 1);
    node.subdivide();

    VECTORS;
    NEIGHBOUR_LOD;

    create_geometry(&node, &node,
                    1, neighbour_lod,
                    indicies,
                    position, normal, tangent);

    std::vector<Vector3f> expected_positions({
                                                 Vector3f(0, 0, 1),
                                                 Vector3f(0, 64, 1),
                                                 Vector3f(64, 0, 1),
                                                 Vector3f(64, 0, 1),
                                                 Vector3f(0, 64, 1),
                                                 Vector3f(64, 64, 1),

                                                 Vector3f(64, 0, 1),
                                                 Vector3f(64, 64, 1),
                                                 Vector3f(127, 0, 1),
                                                 Vector3f(127, 0, 1),
                                                 Vector3f(64, 64, 1),
                                                 Vector3f(127, 64, 1),

                                                 Vector3f(0, 64, 1),
                                                 Vector3f(0, 127, 1),
                                                 Vector3f(64, 64, 1),
                                                 Vector3f(64, 64, 1),
                                                 Vector3f(0, 127, 1),
                                                 Vector3f(64, 127, 1),

                                                 Vector3f(64, 64, 1),
                                                 Vector3f(64, 127, 1),
                                                 Vector3f(127, 64, 1),
                                                 Vector3f(127, 64, 1),
                                                 Vector3f(64, 127, 1),
                                                 Vector3f(127, 127, 1)
                                             });
    CHECK(expected_positions == resolve_indicies(indicies, position));
}

TEST_CASE("render/quadterrain/create_geometry"
          "/normal_node_at_root_with_nonequal_children")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 128, 0);
    node.set_height_rect(TerrainRect(0, 0, 64, 64), 1);
    node.set_height_rect(TerrainRect(64, 0, 128, 64), 2);
    node.set_height_rect(TerrainRect(0, 64, 64, 128), 3);
    node.set_height_rect(TerrainRect(64, 64, 128, 128), 4);

    VECTORS;
    NEIGHBOUR_LOD;

    create_geometry(&node, &node,
                    1, neighbour_lod,
                    indicies,
                    position, normal, tangent);

    std::vector<Vector3f> expected_positions({
                                                 Vector3f(0, 0, 1),
                                                 Vector3f(0, 63, 1),
                                                 Vector3f(63, 0, 1),
                                                 Vector3f(63, 0, 1),
                                                 Vector3f(0, 63, 1),
                                                 Vector3f(63, 63, 1),

                                                 Vector3f(63, 0, 1),
                                                 Vector3f(63, 63, 1),
                                                 Vector3f(64, 0, 2),
                                                 Vector3f(64, 0, 2),
                                                 Vector3f(63, 63, 1),
                                                 Vector3f(64, 63, 2),

                                                 Vector3f(0, 63, 1),
                                                 Vector3f(0, 64, 3),
                                                 Vector3f(63, 63, 1),
                                                 Vector3f(63, 63, 1),
                                                 Vector3f(0, 64, 3),
                                                 Vector3f(63, 64, 3),

                                                 Vector3f(63, 63, 1),
                                                 Vector3f(63, 64, 3),
                                                 Vector3f(64, 63, 2),
                                                 Vector3f(64, 63, 2),
                                                 Vector3f(63, 64, 3),
                                                 Vector3f(64, 64, 4),

                                                 Vector3f(64, 0, 2),
                                                 Vector3f(64, 63, 2),
                                                 Vector3f(127, 0, 2),
                                                 Vector3f(127, 0, 2),
                                                 Vector3f(64, 63, 2),
                                                 Vector3f(127, 63, 2),

                                                 Vector3f(64, 63, 2),
                                                 Vector3f(64, 64, 4),
                                                 Vector3f(127, 63, 2),
                                                 Vector3f(127, 63, 2),
                                                 Vector3f(64, 64, 4),
                                                 Vector3f(127, 64, 4),

                                                 Vector3f(0, 64, 3),
                                                 Vector3f(0, 127, 3),
                                                 Vector3f(63, 64, 3),
                                                 Vector3f(63, 64, 3),
                                                 Vector3f(0, 127, 3),
                                                 Vector3f(63, 127, 3),

                                                 Vector3f(63, 64, 3),
                                                 Vector3f(63, 127, 3),
                                                 Vector3f(64, 64, 4),
                                                 Vector3f(64, 64, 4),
                                                 Vector3f(63, 127, 3),
                                                 Vector3f(64, 127, 4),

                                                 Vector3f(64, 64, 4),
                                                 Vector3f(64, 127, 4),
                                                 Vector3f(127, 64, 4),
                                                 Vector3f(127, 64, 4),
                                                 Vector3f(64, 127, 4),
                                                 Vector3f(127, 127, 4)
                                             });

    CHECK(expected_positions == resolve_indicies(indicies, position));
}
*/
