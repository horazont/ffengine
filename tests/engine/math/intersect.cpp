#include <catch.hpp>

#include "engine/math/intersect.hpp"

TEST_CASE("math/intersect/isect_ray_triangle/intersecting",
          "Test ray-triangle intersection")
{
    Ray r1(Vector3f(0, 0, 1), Vector3f(0, 0, -1));

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
    Ray r1(Vector3f(0, 0, 1), Vector3f(0, 0.5, 1));

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
    Ray r1(Vector3f(0, 0, -1), Vector3f(0, 0, -1));

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(0.5, 0, 0);
    Vector3f p2(0, 0.5, 0);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);

    CHECK(!success);
    CHECK(t < 0);
}

TEST_CASE("math/intersect/isect_ray_triangle/hit_the_edge")
{
    Ray r1(Vector3f(0, 0, 1000), Vector3f(0, 0, -0.99999));

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(0.5, -0.5, 0);
    Vector3f p2(-0.5, 0.5, 0);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);

    CHECK(success);
    CHECK(fabs(t - 1000) < ISECT_EPSILON);
}

TEST_CASE("math/intersect/isect_ray_triangle/hit_the_edge_non_perp")
{
    Ray r1(Vector3f(-9.27867, 22.95933, 26.37506), Vector3f(9.27867, -22.95933, -26.37506).normalized());

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(0.5, -0.5, 0);
    Vector3f p2(-0.5, 0.5, 0);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);

    CHECK(success);
    CHECK(fabs(t - (1000-963.8217163086)) < ISECT_EPSILON);
}

TEST_CASE("math/intersect/isect_ray_triangle/hit_close_to_the_edge")
{
    Ray r1(Vector3f(-9.27867, 22.95933, 26.37506), Vector3f(9.27867, -22.95933, -26.37506).normalized());

    Vector3f p0(-0.5, -0.5, 0);
    Vector3f p1(3.5, -0.5, 0);
    Vector3f p2(-0.5, 0.1, 0);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);

    CHECK(success);
    CHECK(fabs(t - (1000-963.8217163086)) < ISECT_EPSILON);
}

TEST_CASE("math/intersect/isect_ray_triangle/hit_close_to_the_larger_edge")
{
    Ray r1(Vector3f(-9.27867, 22.95933, 26.37506), Vector3f(9.27867, -22.95933, -26.37506).normalized());

    Vector3f p0(-1.79260, 0.31470, 0);
    Vector3f p1(-1.79260, -1.76613, 0);
    Vector3f p2(12.07960, -1.76613, 0);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);

    CHECK(success);
    CHECK(fabs(t - (1000-963.8217163086)) < ISECT_EPSILON);
}

TEST_CASE("math/intersect/isect_ray_triangle/realworld_miss1")
{
    Ray r1(Vector3f(31.128395, 11.252053, 13.137098), Vector3f(-0.836384, -0.054968, -0.545381));

    Vector3f p0(17.000000, 10.000000, -0.191436);
    Vector3f p1(16.000000, 11.000000, 9.771060);
    Vector3f p2(17.000000, 11.000000, 0.224137);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);
    CHECK_FALSE(success);
}

TEST_CASE("math/intersect/isect_ray_triangle/realworld_miss2")
{
    Ray r1(Vector3f(33.072754, 17.278791, 12.833068), Vector3f(-0.735938, 0.150116, -0.660197));

    Vector3f p0(23.000000, 19.000000, 9.568980);
    Vector3f p1(23.000000, 20.000000, 9.092868);
    Vector3f p2(24.000000, 20.000000, 2.616207);

    float t;
    bool success;
    std::tie(t, success) = isect_ray_triangle(r1, p0, p1, p2);
    CHECK_FALSE(success);
}

TEST_CASE("math/intersect/isect_plane_ray/intersection",
          "Test ray-plane intersection")
{
    Ray r1(Vector3f(0, 0, 4), Vector3f(0, 0, -1));

    Vector3f plane_pos(2, 2, 3);
    Vector3f plane_normal(0.1, 0.1, 1.0);
    plane_normal.normalize();

    float t;
    PlaneSide side;
    std::tie(t, side) = isect_plane_ray(Plane(plane_pos, plane_normal), r1);

    CHECK(side == PlaneSide::BOTH);
    CHECK(t >= 0);
}

TEST_CASE("math/intersect/isect_plane_ray/below",
          "Test ray-plane intersection, ray below plane")
{
    Ray r1(Vector3f(0, 0, 2), Vector3f(0, 0, -1));

    Vector3f plane_pos(2, 2, 3);
    Vector3f plane_normal(0.1, 0.1, 1.0);
    plane_normal.normalize();

    float t;
    PlaneSide side;
    std::tie(t, side) = isect_plane_ray(Plane(plane_pos, plane_normal), r1);

    CHECK(side == PlaneSide::BOTH);
    CHECK(t < 0);
}

TEST_CASE("math/intersect/isect_plane_ray/parallel",
          "Test ray-plane intersection, ray parallel to plane")
{
    Ray r1(Vector3f(0, 0, 2), Vector3f(0, 0, -1));

    Vector3f plane_pos(-1, 0, 0);
    Vector3f plane_normal(1, 0, 0);
    plane_normal.normalize();

    float t;
    PlaneSide side;
    std::tie(t, side) = isect_plane_ray(Plane(plane_pos, plane_normal), r1);

    CHECK(side == PlaneSide::POSITIVE_NORMAL);
}

TEST_CASE("math/intersect/isect_plane_ray/parallel_on_plane",
          "Test ray-plane intersection, ray parallel to plane and on plane")
{
    Ray r1(Vector3f(-1, 0, 2), Vector3f(0, 0, -1));

    Vector3f plane_pos(-1, 0, 0);
    Vector3f plane_normal(1, 0, 0);
    plane_normal.normalize();

    float t;
    PlaneSide side;
    std::tie(t, side) = isect_plane_ray(Plane(plane_pos, plane_normal), r1);

    CHECK(side == PlaneSide::BOTH);
    CHECK(t == 0);
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

TEST_CASE("math/intersect/isect_aabb_ray/through_x_planes_only")
{
    Ray r(Vector3f(-2, 0, 0), Vector3f(1, 0, 0));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK(hit);
    CHECK(t0 == 1);
    CHECK(t1 == 3);
}

TEST_CASE("math/intersect/isect_aabb_ray/through_y_planes_only")
{
    Ray r(Vector3f(0, -2, 0), Vector3f(0, 1, 0));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK(hit);
    CHECK(t0 == 1);
    CHECK(t1 == 3);
}

TEST_CASE("math/intersect/isect_aabb_ray/through_z_planes_only")
{
    Ray r(Vector3f(0, 0, -2), Vector3f(0, 0, 1));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK(hit);
    CHECK(t0 == 1);
    CHECK(t1 == 3);
}

TEST_CASE("math/intersect/isect_aabb_ray/through_corner")
{
    Ray r(Vector3f(-2, -2, -2), Vector3f(1, 1, 1));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK(hit);
    CHECK(fabs(t0 - 1.73205f) < ISECT_EPSILON);
    CHECK(fabs(t1 - 5.19615f) < ISECT_EPSILON);
}

TEST_CASE("math/intersect/isect_aabb_ray/through_xy_edge")
{
    Ray r(Vector3f(-2, -2, 0), Vector3f(1, 1, 0));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK(hit);
    CHECK(fabs(t0 - 1.41421f) < ISECT_EPSILON);
    CHECK(fabs(t1 - 4.24264f) < ISECT_EPSILON);
}

TEST_CASE("math/intersect/isect_aabb_ray/general_case")
{
    Ray r(Vector3f(-1.5, -2, -2.5), Vector3f(0.3, 0.2, 0.6));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK(hit);
    CHECK(fabs(t0 - 3.5f) < ISECT_EPSILON);
    CHECK(fabs(t1 - 4.0833330154f) < ISECT_EPSILON);
}

TEST_CASE("math/intersect/isect_aabb_ray/x_axis_parallel_outside")
{
    Ray r(Vector3f(-2, -10, 0), Vector3f(1, 0, 0));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK_FALSE(hit);
}

TEST_CASE("math/intersect/isect_aabb_ray/x_axis_parallel_on_edge")
{
    Ray r(Vector3f(-2, -1, 0), Vector3f(1, 0, 0));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK(hit);
    CHECK(t0 == 1);
    CHECK(t1 == 3);
}

TEST_CASE("math/intersect/isect_aabb_ray/general_outside")
{
    Ray r(Vector3f(-2, -3, -4), Vector3f(-0.2, 0.3, 0.4));
    AABB aabb{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK_FALSE(hit);
}

TEST_CASE("math/intersect/isect_aabb_ray/general0")
{
    Ray r(Vector3f(1034, -1, -0.5), Vector3f(-0.5, 1, 0));
    AABB aabb{Vector3f(0, 0, -1), Vector3f(2048, 2048, 1)};

    float t0, t1;
    bool hit = isect_aabb_ray(aabb, r, t0, t1);

    CHECK(hit);
}
