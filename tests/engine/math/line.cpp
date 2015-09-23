/**********************************************************************
File name: line.cpp
This file is part of: SCC (working title)

LICENSE

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

FEEDBACK & QUESTIONS

For feedback and questions about SCC please e-mail one of the authors named in
the AUTHORS file.
**********************************************************************/
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


TEST_CASE("math/Line2f/sample")
{
    SECTION("homogenous[eY] is larger")
    {
        Line2f l(Vector2f(0, 1), Vector2f(10, 2));
        CHECK(l.homogeneous[eX] == -2.f);
        CHECK(l.homogeneous[eY] == 10.f);
        CHECK(l.homogeneous[eZ] == -10.f);

        CHECK(l.sample() == Vector2f(0, 1));
    }
    SECTION("homogenous[eX] is larger")
    {
        Line2f l(Vector2f(0, 1), Vector2f(2, 10));
        CHECK(l.homogeneous[eX] == -10.f);
        CHECK(l.homogeneous[eY] == 2.f);
        CHECK(l.homogeneous[eZ] == -2.f);

        CHECK(l.sample() == Vector2f(-0.2f, 0));
    }
}


TEST_CASE("math/Line2f/point_and_direction")
{
    SECTION("homogenous[eY] is larger")
    {
        Line2f l(Vector2f(0, 1), Vector2f(10, 2));
        CHECK(l.homogeneous[eX] == -2.f);
        CHECK(l.homogeneous[eY] == 10.f);
        CHECK(l.homogeneous[eZ] == -10.f);

        CHECK(l.point_and_direction().second == Vector2f(10, 2));
        CHECK(l.point_and_direction().first == l.sample());
    }
    SECTION("homogenous[eX] is larger")
    {
        Line2f l(Vector2f(0, 1), Vector2f(2, 10));
        CHECK(l.homogeneous[eX] == -10.f);
        CHECK(l.homogeneous[eY] == 2.f);
        CHECK(l.homogeneous[eZ] == -2.f);

        CHECK(l.point_and_direction().second == Vector2f(2, 10));
        CHECK(l.point_and_direction().first == l.sample());
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
