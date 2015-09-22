#include <catch.hpp>

#include "ffengine/math/line.hpp"


TEST_CASE("math/Line2f/Line2f(p0, v)")
{
    SECTION("through origin, parallel to x axis")
    {
        Line2f l(Vector2f(0, 0), Vector2f(0, 1));
        CHECK(l.homogeneous[eX] == -1);
        CHECK(l.homogeneous[eY] == 0);
        CHECK(l.homogeneous[eZ] == 0);
    }
    SECTION("through origin, parallel to y axis")
    {
        Line2f l(Vector2f(0, 0), Vector2f(1, 0));
        CHECK(l.homogeneous[eX] == 0);
        CHECK(l.homogeneous[eY] == 1);
        CHECK(l.homogeneous[eZ] == 0);
    }
    SECTION("not through origin, parallel to x axis")
    {
        Line2f l(Vector2f(4, 5), Vector2f(1, 0));
        CHECK(l.homogeneous[eX] == 0);
        CHECK(l.homogeneous[eY] == 1);
        CHECK(l.homogeneous[eZ] == -5.f);
    }
    SECTION("not through origin, parallel to y axis")
    {
        Line2f l(Vector2f(4, 5), Vector2f(0, 1));
        CHECK(l.homogeneous[eX] == -1);
        CHECK(l.homogeneous[eY] == 0);
        CHECK(l.homogeneous[eZ] == 4.f);
    }
    SECTION("bisectrix")
    {
        Line2f l(Vector2f(1, 1), Vector2f(1, 1));
        CHECK(l.homogeneous[eX] == -1);
        CHECK(l.homogeneous[eY] == 1);
        CHECK(l.homogeneous[eZ] == 0);
    }
}


TEST_CASE("math/isect_line_line/parallel")
{
    Line2f l1(Vector2f(0, 0), Vector2f(1, 0));
    Line2f l2(Vector2f(2, 2), Vector2f(1, 0));

    Vector2f intersection_point = isect_line_line(l1, l2);
    CHECK(std::isnan(intersection_point[eX]));
    CHECK(std::isnan(intersection_point[eY]));
}


TEST_CASE("math/isect_line_line/intersecting")
{
    Line2f l1(Vector2f(0, 0), Vector2f(1, 0));
    Line2f l2(Vector2f(2, 3), Vector2f(1, 1));

    Vector2f intersection_point = isect_line_line(l1, l2);
    CHECK(intersection_point == Vector2f(-1, 0));
}
