#include <catch.hpp>

#include "engine/render/fancyterraindata.hpp"

using namespace engine;
using namespace sim;

TEST_CASE("render/fancyterraindata/isect_terrain_quadtree_ray/1x1")
{
    Ray r{Vector3f(-1, 10, 0), Vector3f(1, 0, 0)};

    MinMaxMapGenerator::MinMaxFieldLODs lods;
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({std::make_tuple(-1, 1)});

    Vector3f enters_at, leaves_at;
    bool hit = false;
    std::tie(enters_at, leaves_at, hit) = isect_terrain_quadtree_ray(r, 2048, lods);

    CHECK(hit);
    CHECK(enters_at == Vector3f(0, 10, 0));
    CHECK(leaves_at == Vector3f(2048, 10, 0));
}

#ifndef DISABLE_QUADTREE

TEST_CASE("render/fancyterraindata/isect_terrain_quadtree_ray/2x2/recursed_full_hit")
{
    Ray r{Vector3f(-1, 10, 0), Vector3f(1, 0, 0)};

    MinMaxMapGenerator::MinMaxFieldLODs lods;
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({
                                                                             std::make_tuple(-1, 1),
                                                                             std::make_tuple(0, 0.5),
                                                                             std::make_tuple(-1, 1),
                                                                             std::make_tuple(0, 0.5)
                                                                         });
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({std::make_tuple(-1, 1)});

    Vector3f enters_at, leaves_at;
    bool hit = false;
    std::tie(enters_at, leaves_at, hit) = isect_terrain_quadtree_ray(r, 2048, lods);

    CHECK(hit);
    CHECK(enters_at == Vector3f(0, 10, 0));
    CHECK(leaves_at == Vector3f(2048, 10, 0));
}

TEST_CASE("render/fancyterraindata/isect_terrain_quadtree_ray/2x2/recursed_partial_hit")
{
    Ray r{Vector3f(-1, 10, -0.5), Vector3f(1, 0, 0)};

    MinMaxMapGenerator::MinMaxFieldLODs lods;
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({
                                                                             std::make_tuple(-1, 1),
                                                                             std::make_tuple(0, 0.5),
                                                                             std::make_tuple(-1, 1),
                                                                             std::make_tuple(0, 0.5)
                                                                         });
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({std::make_tuple(-1, 1)});

    Vector3f enters_at, leaves_at;
    bool hit = false;
    std::tie(enters_at, leaves_at, hit) = isect_terrain_quadtree_ray(r, 2048, lods);

    CHECK(hit);
    CHECK(enters_at == Vector3f(0, 10, -0.5));
    CHECK(leaves_at == Vector3f(1024, 10, -0.5));
}

TEST_CASE("render/fancyterraindata/isect_terrain_quadtree_ray/2x2/recursed_miss")
{
    Ray r{Vector3f(1034, -1, -0.5), Vector3f(0, 1, 0)};

    MinMaxMapGenerator::MinMaxFieldLODs lods;
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({
                                                                             std::make_tuple(-1, 1),
                                                                             std::make_tuple(0, 0.5),
                                                                             std::make_tuple(-1, 1),
                                                                             std::make_tuple(0, 0.5)
                                                                         });
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({std::make_tuple(-1, 1)});

    Vector3f enters_at, leaves_at;
    bool hit = false;
    std::tie(enters_at, leaves_at, hit) = isect_terrain_quadtree_ray(r, 2048, lods);

    CHECK_FALSE(hit);
    /* CHECK(enters_at == Vector3f(0, 10, -0.5));
    CHECK(leaves_at == Vector3f(1024, 10, -0.5)); */
}

TEST_CASE("render/fancyterraindata/isect_terrain_quadtree_ray/2x2/partial_hit_non_aa")
{
    Ray r{Vector3f(1034, -1, -0.5), Vector3f(-0.5, 1, 0)};

    MinMaxMapGenerator::MinMaxFieldLODs lods;
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({
                                                                             std::make_tuple(-1, 1),
                                                                             std::make_tuple(0, 0.5),
                                                                             std::make_tuple(-1, 1),
                                                                             std::make_tuple(0, 0.5)
                                                                         });
    lods.emplace_back<std::initializer_list<std::tuple<float, float> > >({std::make_tuple(-1, 1)});

    Vector3f enters_at, leaves_at;
    bool hit = false;
    std::tie(enters_at, leaves_at, hit) = isect_terrain_quadtree_ray(r, 2048, lods);

    CHECK(hit);
    CHECK(enters_at[eX] == 1024);
    CHECK(leaves_at[eY] == 2048);
}

#endif
