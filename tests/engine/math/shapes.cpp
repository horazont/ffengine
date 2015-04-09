#include <catch.hpp>

#include "engine/math/shapes.hpp"


TEST_CASE("math/shapes/Plane/side_of(Sphere)/positive_normal")
{
    Plane plane(0, Vector3f(1, 0, 0));
    Sphere sphere{Vector3f(2, 0, 0), 1.9};

    CHECK(plane.side_of(Sphere{Vector3f(2, 0, 0), 1.9}) == PlaneSide::POSITIVE_NORMAL);
    CHECK(plane.side_of(Sphere{Vector3f(2, 3, 4), 1.9}) == PlaneSide::POSITIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Sphere)/negative_normal")
{
    Plane plane(0, Vector3f(1, 0, 0));

    CHECK(plane.side_of(Sphere{Vector3f(-2, 0, 0), 1.9}) == PlaneSide::NEGATIVE_NORMAL);
    CHECK(plane.side_of(Sphere{Vector3f(-2, 3, 4), 1.9}) == PlaneSide::NEGATIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Sphere)/intersection")
{
    Plane plane(0, Vector3f(1, 0, 0));

    CHECK(plane.side_of(Sphere{Vector3f(-2, 0, 0), 2.1}) == PlaneSide::BOTH);
    CHECK(plane.side_of(Sphere{Vector3f(2, 0, 0), 2.1}) == PlaneSide::BOTH);
    CHECK(plane.side_of(Sphere{Vector3f(2, 3, 4), 2.1}) == PlaneSide::BOTH);
    CHECK(plane.side_of(Sphere{Vector3f(-2, -3, 4), 2.1}) == PlaneSide::BOTH);
}

TEST_CASE("math/shapes/Plane/side_of_fast(AABB)/positive_normal")
{
    Plane plane(0, Vector3f(1, 0, 0));

    CHECK(plane.side_of_fast(AABB{Vector3f(2, 2, 2), Vector3f(3, 3, 3)}) == PlaneSide::POSITIVE_NORMAL);
    CHECK(plane.side_of_fast(AABB{Vector3f(1, 1, 1), Vector3f(3, 3, 3)}) == PlaneSide::POSITIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of_fast(AABB)/negative_normal")
{
    Plane plane(0, Vector3f(-1, 0, 0));

    CHECK(plane.side_of_fast(AABB{Vector3f(2, 2, 2), Vector3f(3, 3, 3)}) == PlaneSide::NEGATIVE_NORMAL);
    CHECK(plane.side_of_fast(AABB{Vector3f(1, 1, 1), Vector3f(3, 3, 3)}) == PlaneSide::NEGATIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of_fast(AABB)/intersection/false_positive")
{
    Plane plane(0, Vector3f(-1, 0, 0));

    CHECK(plane.side_of_fast(AABB{Vector3f(0.1, 0.1, 0.1), Vector3f(3, 1, 1)}) == PlaneSide::BOTH);
}

TEST_CASE("math/shapes/Plane/side_of_fast(AABB)/intersection/true_positive")
{
    Plane plane(0, Vector3f(-1, 0, 0));

    CHECK(plane.side_of_fast(AABB{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)}) == PlaneSide::BOTH);
}

