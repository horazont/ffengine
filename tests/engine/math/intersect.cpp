#include <catch.hpp>

#include "engine/math/intersect.hpp"

TEST_CASE("math/intersect/isect_ray_triangle/intersecting",
          "Test ray-triangle intersection")
{
    Ray r1{Vector3f(0, 0, 1), Vector3f(0, 0, -1)};

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(0.5, 0, 0);
    Vector3f p2(0, 0.5, 0);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);

    CHECK(success);
    CHECK(t == 1);
}

TEST_CASE("math/intersect/isect_ray_triangle/non_intersecting",
          "Test ray-triangle non-intersection")
{
    Ray r1{Vector3f(0, 0, 1), Vector3f(0, 0.5, 1)};

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(0.5, 0, 0);
    Vector3f p2(0, 0.5, 0);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);

    CHECK(!success);
    CHECK(isnan(t));
}

TEST_CASE("math/intersect/isect_ray_triangle/below",
          "Test ray-triangle ray below triangle")
{
    Ray r1{Vector3f(0, 0, -1), Vector3f(0, 0, -1)};

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(0.5, 0, 0);
    Vector3f p2(0, 0.5, 0);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);

    CHECK(!success);
    CHECK(t < 0);
}

TEST_CASE("math/intersect/isect_plane_ray/intersection",
          "Test ray-plane intersection")
{
    Ray r1{Vector3f(0, 0, 4), Vector3f(0, 0, -1)};

    Vector3f plane_pos(2, 2, 3);
    Vector3f plane_normal(0.1, 0.1, 1.0);
    plane_normal.normalize();

    float t;
    bool success;
    std::tie(t, success) = isect_plane_ray(plane_pos, plane_normal, r1);

    CHECK(success);
    CHECK(t >= 0);
}

TEST_CASE("math/intersect/isect_plane_ray/below",
          "Test ray-plane intersection, ray below plane")
{
    Ray r1{Vector3f(0, 0, 2), Vector3f(0, 0, -1)};

    Vector3f plane_pos(2, 2, 3);
    Vector3f plane_normal(0.1, 0.1, 1.0);
    plane_normal.normalize();

    float t;
    bool success;
    std::tie(t, success) = isect_plane_ray(plane_pos, plane_normal, r1);

    CHECK(!success);
    CHECK(t < 0);
}

TEST_CASE("math/intersect/isect_plane_ray/parallel",
          "Test ray-plane intersection, ray parallel to plane")
{
    Ray r1{Vector3f(0, 0, 2), Vector3f(0, 0, -1)};

    Vector3f plane_pos(-1, 0, 0);
    Vector3f plane_normal(1, 0, 0);
    plane_normal.normalize();

    float t;
    bool success;
    std::tie(t, success) = isect_plane_ray(plane_pos, plane_normal, r1);

    CHECK(!success);
    CHECK(isnan(t));
}

TEST_CASE("math/intersect/isect_aabb_sphere/intersection")
{
    Sphere sphere{Vector3f(1, 2, 3), 2};

    CHECK(isect_aabb_sphere(AABB{Vector3f(0, 0, 0), Vector3f(3, 3, 3)}, sphere));
    CHECK(isect_aabb_sphere(AABB{Vector3f(1, 2, 3), Vector3f(4, 5, 6)}, sphere));
}

TEST_CASE("math/intersect/isect_aabb_sphere/non_intersection")
{
    Sphere sphere{Vector3f(1, 2, 3), 2};

    CHECK_FALSE(isect_aabb_sphere(AABB{Vector3f(-4, -4, -4), Vector3f(-1, -1, -1)}, sphere));
    CHECK_FALSE(isect_aabb_sphere(AABB{Vector3f(10, 10, 10), Vector3f(11, 11, 11)}, sphere));
}
