#include <catch.hpp>

#include "engine/sim/quadterrain.hpp"

#include <iostream>

using namespace sim;


TEST_CASE("sim/quadterrain/QuadNode/init_leaf",
          "Set up a leaf")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 4096, 10);
    CHECK(node.type() == QuadNode::Type::LEAF);
    CHECK(node.height() == 10);
    CHECK(node.x0() == 0);
    CHECK(node.y0() == 0);
    CHECK(node.size() == 4096);
    CHECK(node.parent() == nullptr);
}

TEST_CASE("sim/quadterrain/QuadNode/init_normal",
          "Set up a non-leaf non-heightmap node")
{
    QuadNode node(nullptr, QuadNode::Type::NORMAL, 0, 0, 4096, 10);
    CHECK(node.type() == QuadNode::Type::NORMAL);
    CHECK(node.height() == 10);
    CHECK(node.x0() == 0);
    CHECK(node.y0() == 0);
    CHECK(node.size() == 4096);
    CHECK(node.parent() == nullptr);

    QuadNode *child = node.child(QuadNode::NORTHWEST);
    REQUIRE(child != nullptr);
    CHECK(child->x0() == 0);
    CHECK(child->y0() == 0);
    CHECK(child->size() == 2048);
    CHECK(child->parent() == &node);
    CHECK(child->height() == 10);

    child = node.child(QuadNode::NORTHEAST);
    REQUIRE(child != nullptr);
    CHECK(child->x0() == 2048);
    CHECK(child->y0() == 0);
    CHECK(child->size() == 2048);
    CHECK(child->parent() == &node);
    CHECK(child->height() == 10);

    child = node.child(QuadNode::SOUTHWEST);
    REQUIRE(child != nullptr);
    CHECK(child->x0() == 0);
    CHECK(child->y0() == 2048);
    CHECK(child->size() == 2048);
    CHECK(child->parent() == &node);
    CHECK(child->height() == 10);

    child = node.child(QuadNode::SOUTHEAST);
    REQUIRE(child != nullptr);
    CHECK(child->x0() == 2048);
    CHECK(child->y0() == 2048);
    CHECK(child->size() == 2048);
    CHECK(child->parent() == &node);
    CHECK(child->height() == 10);
}

TEST_CASE("sim/quadterrain/QuadNode/init_normal/fail_with_odd_size")
{
    REQUIRE_THROWS_AS(
                new QuadNode(nullptr, QuadNode::Type::NORMAL, 0, 0, 4097, 10),
                std::runtime_error);
}

TEST_CASE("sim/quadterrain/QuadNode/init_heightmap")
{
    QuadNode node(nullptr, QuadNode::Type::HEIGHTMAP, 0, 0, 128, 10);
    CHECK(node.type() == QuadNode::Type::HEIGHTMAP);
    CHECK(node.x0() == 0);
    CHECK(node.y0() == 0);
    CHECK(node.size() == 128);
    CHECK(node.parent() == nullptr);

    QuadNode::Heightmap heightmap(128*128, 10);
    CHECK(heightmap == *node.heightmap());
}

TEST_CASE("sim/quadterrain/QuadNode/subdivide_leaf")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 4096, 10);
    node.subdivide();
    CHECK(node.type() == QuadNode::Type::NORMAL);
    CHECK_FALSE(node.dirty());

    QuadNode *child = node.child(QuadNode::NORTHWEST);
    REQUIRE(child != nullptr);
    CHECK(child->x0() == 0);
    CHECK(child->y0() == 0);
    CHECK(child->size() == 2048);
    CHECK(child->parent() == &node);
    CHECK(child->height() == 10);

    child = node.child(QuadNode::NORTHEAST);
    REQUIRE(child != nullptr);
    CHECK(child->x0() == 2048);
    CHECK(child->y0() == 0);
    CHECK(child->size() == 2048);
    CHECK(child->parent() == &node);
    CHECK(child->height() == 10);

    child = node.child(QuadNode::SOUTHWEST);
    REQUIRE(child != nullptr);
    CHECK(child->x0() == 0);
    CHECK(child->y0() == 2048);
    CHECK(child->size() == 2048);
    CHECK(child->parent() == &node);
    CHECK(child->height() == 10);

    child = node.child(QuadNode::SOUTHEAST);
    REQUIRE(child != nullptr);
    CHECK(child->x0() == 2048);
    CHECK(child->y0() == 2048);
    CHECK(child->size() == 2048);
    CHECK(child->parent() == &node);
    CHECK(child->height() == 10);
}

TEST_CASE("sim/quadterrain/QuadNode/merge_normal")
{
    QuadNode node(nullptr, QuadNode::Type::NORMAL, 0, 0, 4096, 10);
    node.merge();
    CHECK(node.type() == QuadNode::Type::LEAF);
    CHECK(node.height() == 10);
    CHECK_FALSE(node.dirty());
}

TEST_CASE("sim/quadterrain/QuadNode/set_height_rect/top_level_leaf_selected")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 128, 0);
    node.set_height_rect(TerrainRect(0, 0, 128, 128), 10);
    CHECK(node.type() == QuadNode::Type::LEAF);
    CHECK(node.height() == 10);
    CHECK(node.dirty());
}

TEST_CASE("sim/quadterrain/QuadNode/set_height_rect/full_child_selected")
{
    QuadNode node(nullptr, QuadNode::Type::NORMAL, 0, 0, 128, 0);
    node.set_height_rect(TerrainRect(0, 0, 64, 64), 10);
    CHECK(node.type() == QuadNode::Type::NORMAL);
    for (unsigned int i = 0; i < 4; i++) {
        QuadNode *child = node.child(i);
        if (i == QuadNode::NORTHWEST) {
            CHECK(child->height() == 10);
        } else {
            CHECK(child->height() == 0);
        }
        CHECK(child->type() == QuadNode::Type::LEAF);
    }
    node.cleanup();
    CHECK(node.height() == 3);
}

TEST_CASE("sim/quadterrain/QuadNode/set_height_rect/partial_children_selected")
{
    static const std::array<unsigned int, 4> grandchildren_with_height({
                                                                           QuadNode::SOUTHEAST,
                                                                           QuadNode::SOUTHWEST,
                                                                           QuadNode::NORTHEAST,
                                                                           QuadNode::NORTHWEST
                                                                       });

    QuadNode node(nullptr, QuadNode::Type::NORMAL, 0, 0, 128, 0);
    node.set_height_rect(TerrainRect(32, 32, 96, 96), 10);
    node.cleanup();
    CHECK(node.subtree_changed());
    CHECK(node.changed());  // avg. height changed
    CHECK(node.subtree_changed());  // subtree changed
    CHECK(node.type() == QuadNode::Type::NORMAL);
    for (unsigned int i = 0; i < 4; i++) {
        QuadNode *child = node.child(i);
        REQUIRE(child->type() == QuadNode::Type::NORMAL);
        for (unsigned int j = 0; j < 4; j++) {
            QuadNode *grandchild = child->child(j);
            REQUIRE(grandchild->type() == QuadNode::Type::LEAF);
            if (j == grandchildren_with_height[i]) {
                CHECK(grandchild->height() == 10);
            } else {
                CHECK(grandchild->height() == 0);
            }
        }
        CHECK(child->height() == 3);
    }
    CHECK(node.height() == 3);
}

TEST_CASE("sim/quadterrain/QuadNode/set_height_rect/random_rect")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 128, 0);
    node.set_height_rect(TerrainRect(3, 3, 8, 13), 10);
    CHECK(node.type() == QuadNode::Type::NORMAL);

    for (unsigned int i = 1; i < 4; i++) {
        QuadNode *child = node.child(i);
        CHECK(child->type() == QuadNode::Type::LEAF);
        CHECK(child->height() == 0);
    }

    QuadNode *child = node.child(0);
    REQUIRE(child->type() == QuadNode::Type::NORMAL);

    for (unsigned int i = 1; i < 4; i++) {
        QuadNode *grandchild = child->child(i);
        CHECK(grandchild->type() == QuadNode::Type::LEAF);
        CHECK(grandchild->height() == 0);
    }

    QuadNode *gchild = child->child(0);
    REQUIRE(gchild->type() == QuadNode::Type::NORMAL);

    for (unsigned int i = 1; i < 4; i++) {
        QuadNode *ggchild = gchild->child(i);
        CHECK(ggchild->type() == QuadNode::Type::LEAF);
        CHECK(ggchild->height() == 0);
    }

    QuadNode *ggchild = gchild->child(0);
    REQUIRE(ggchild->type() == QuadNode::Type::NORMAL);
    REQUIRE(ggchild->child(QuadNode::NORTHWEST)->type() == QuadNode::Type::NORMAL);
    REQUIRE(ggchild->child(QuadNode::SOUTHWEST)->type() == QuadNode::Type::NORMAL);
    CHECK(ggchild->child(QuadNode::NORTHEAST)->type() == QuadNode::Type::LEAF);
    CHECK(ggchild->child(QuadNode::SOUTHEAST)->type() == QuadNode::Type::LEAF);


    // from here on, only partial checks are done. we trace down to a downmost
    // leaf node though, as the code for each cell should be the same, we
    // should be fine
    QuadNode *gggchild = ggchild->child(QuadNode::NORTHWEST);
    CHECK(gggchild->child(QuadNode::SOUTHEAST)->type()
          == QuadNode::Type::LEAF);
    CHECK(gggchild->child(QuadNode::SOUTHEAST)->height()
          == 10);

    for (unsigned int i = 0; i < 3; i++) {
        QuadNode *ggggchild = gggchild->child(i);
        REQUIRE(ggggchild->type() == QuadNode::Type::NORMAL);
    }

    QuadNode *ggggchild = gggchild->child(QuadNode::NORTHWEST);
    for (unsigned int i = 0; i < 3; i++) {
        QuadNode *gggggchild = ggggchild->child(i);
        CHECK(gggggchild->type() == QuadNode::Type::LEAF);
        CHECK(gggggchild->height() == 0);
    }

    QuadNode *gggggchild = ggggchild->child(QuadNode::SOUTHEAST);
    REQUIRE(gggggchild->type() == QuadNode::Type::NORMAL);
    for (unsigned int i = 0; i < 4; i++) {
        QuadNode *ggggggchild = gggggchild->child(i);
        CHECK(ggggggchild->type() == QuadNode::Type::LEAF);
        if (i == QuadNode::SOUTHEAST) {
            CHECK(ggggggchild->height() == 10);
        } else {
            CHECK(ggggggchild->height() == 0);
        }
    }
}

TEST_CASE("sim/quadterrain/QuadNode/heightmapify/flat")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 128, 10);
    node.heightmapify();
    REQUIRE(node.type() == QuadNode::Type::HEIGHTMAP);
    const QuadNode::Heightmap ref(128*128, 10);
    CHECK(*node.heightmap() == ref);
}

TEST_CASE("sim/quadterrain/QuadNode/heightmapify/complex")
{
    QuadNode::Heightmap map(128*128, 0);

    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 128, 0);
    for (unsigned int x = 0; x < 64; x++) {
        TerrainRect r(x, 0, x+1, 64);
        node.set_height_rect(r, x);
        for (unsigned int y = 0; y < 64; y++) {
            (&map.front())[y*128+x] = x;
        }
    }

    node.set_height_rect(TerrainRect(64, 64, 128, 128), 2);
    node.set_height_rect(TerrainRect(0, 64, 64, 128), 3);
    node.set_height_rect(TerrainRect(64, 0, 128, 64), 4);

    for (unsigned int y = 64; y < 128; y++) {
        for (unsigned int x = 64; x < 128; x++) {
            (&map.front())[y*128+x] = 2;
        }
    }

    for (unsigned int y = 64; y < 128; y++) {
        for (unsigned int x = 0; x < 64; x++) {
            (&map.front())[y*128+x] = 3;
        }
    }

    for (unsigned int y = 0; y < 64; y++) {
        for (unsigned int x = 64; x < 128; x++) {
            (&map.front())[y*128+x] = 4;
        }
    }

    node.heightmapify();
    REQUIRE(node.type() == QuadNode::Type::HEIGHTMAP);
    CHECK(*node.heightmap() == map);
}

TEST_CASE("sim/quadterrain/QuadNode/quadtreeify/flat")
{
    QuadNode node(nullptr, QuadNode::Type::HEIGHTMAP, 0, 0, 8, 3);
    node.quadtreeify();
    CHECK(node.type() == QuadNode::Type::LEAF);
    CHECK(node.height() == 3);
}

TEST_CASE("sim/quadterrain/QuadNode/quadtreeify/four_flats")
{
    QuadNode node(nullptr, QuadNode::Type::HEIGHTMAP, 0, 0, 16, 0);
    terrain_height_t *const base = &node.heightmap()->front();
    terrain_height_t *dest = base;
    for (unsigned int y = 0; y < 16; y++) {
        for (unsigned int x = 0; x < 16; x++) {
            const terrain_height_t value = (y >= 8
                                            ? (x >= 8) ? 3 : 2
                                            : (x >= 8) ? 1 : 0);
            *dest++ = value;
        }
    }

    node.quadtreeify();
    REQUIRE(node.type() == QuadNode::Type::NORMAL);
    for (unsigned int i = 0 ; i < 4; i++) {
        CHECK(node.child(i)->type() == QuadNode::Type::LEAF);
        CHECK(node.child(i)->height() == i);
    }
}

TEST_CASE("sim/quadterrain/QuadNode/mark_heightmap_dirty")
{
    QuadNode node(nullptr, QuadNode::Type::LEAF, 0, 0, 16, 0);
    node.subdivide();
    node.child(0)->heightmapify();
    node.cleanup();

    (*node.child(0)->heightmap())[0] = 64;
    node.child(0)->mark_heightmap_dirty();
    node.cleanup();
    CHECK_FALSE(node.changed());
    CHECK(node.subtree_changed());
    CHECK(node.child(0)->changed());
    CHECK(node.child(0)->height() == 1);

    for (unsigned int i = 1; i < 4; i++) {
        CHECK_FALSE(node.child(i)->changed());
    }
}

TEST_CASE("sim/quadterrain/QuadNode/set_height_rect/with_heightmap_node")
{
    QuadNode::Heightmap ref(8*8, 10);

    for (unsigned int y = 2; y < 6; y++) {
        for (unsigned int x = 1; x < 5; x++) {
            (&ref.front())[y*8+x] = 5;
        }
    }

    QuadNode node(nullptr, QuadNode::Type::NORMAL, 0, 0, 16, 0);
    node.child(0)->heightmapify();

    node.set_height_rect(TerrainRect(0, 0, 8, 8), 10);
    node.set_height_rect(TerrainRect(1, 2, 5, 6), 5);

    QuadNode::Heightmap &node_heightmap = *node.child(QuadNode::NORTHWEST)->heightmap();
    CHECK(*node.child(QuadNode::NORTHWEST)->heightmap() == ref);
    CHECK(node.child(QuadNode::NORTHWEST)->dirty());
}
