#include <catch.hpp>

#include "engine/math/ray.hpp"

TEST_CASE("math/ray/intersect_triangle/intersecting",
          "Test ray-triangle intersection")
{
    Ray r1{Vector3f(0, 0, 1), Vector3f(0, 0, -1)};

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(0.5, 0, 0);
    Vector3f p2(0, 0.5, 0);

    float t;
    bool success;
    std::tie(t, success) = intersect_triangle(r1, p0, p1, p2);

    CHECK(success);
    CHECK(t == 1);
}

TEST_CASE("math/ray/intersect_triangle/non_intersecting",
          "Test ray-triangle non-intersection")
{
    Ray r1{Vector3f(0, 0, 1), Vector3f(0, 0.5, 1)};

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(0.5, 0, 0);
    Vector3f p2(0, 0.5, 0);

    float t;
    bool success;
    std::tie(t, success) = intersect_triangle(r1, p0, p1, p2);

    CHECK(!success);
    CHECK(isnan(t));
}

