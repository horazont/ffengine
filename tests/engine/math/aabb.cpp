/**********************************************************************
File name: aabb.cpp
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

#include "ffengine/math/aabb.hpp"


TEST_CASE("math/shapes/AABB/AABB(empty_t)")
{
    AABB empty(AABB::Empty);
    CHECK(empty.empty());
}

TEST_CASE("math/shapes/AABB/AABB{Vector3f, Vector3f}")
{
    AABB aabb1{Vector3f(-1, -1, -1), Vector3f(1, 1, 1)};
    AABB aabb2(Vector3f(-1, -1, -1), Vector3f(1, 1, 1));
    CHECK(aabb1 == aabb2);
    CHECK(!aabb1.empty());
}

TEST_CASE("math/shapes/AABB/AABB(Vector3f, Vector3f)")
{
    AABB aabb(Vector3f(-1, -1, -1), Vector3f(1, 1, 1));
    CHECK(aabb.min == Vector3f(-1, -1, -1));
    CHECK(aabb.max == Vector3f(1, 1, 1));
    CHECK(!aabb.empty());
}

TEST_CASE("math/shapes/AABB/AABB(AABB<double>)")
{
    GenericAABB<double> other(Vector3d(0, 0, 0), Vector3d(1, 1, 1));
    AABB aabb(other);
    CHECK(aabb.min == Vector3f(0, 0, 0));
    CHECK(aabb.max == Vector3f(1, 1, 1));
}

TEST_CASE("math/shapes/AABB/AABB = AABB<double>")
{
    GenericAABB<double> other(Vector3d(0, 0, 0), Vector3d(1, 1, 1));
    AABB aabb;
    CHECK(aabb.empty());
    aabb = other;
    CHECK(aabb.min == Vector3f(0, 0, 0));
    CHECK(aabb.max == Vector3f(1, 1, 1));
}

TEST_CASE("math/shapes/AABB/equality")
{
    SECTION("empties")
    {
        AABB empty1(AABB::Empty);
        AABB empty2(Vector3f(10, 10, 10), Vector3f(-1, -1, -1));
        CHECK(empty1.empty());
        CHECK(empty2.empty());
        CHECK(empty1.min != empty2.min);
        CHECK(empty1 == empty2);
    }
    SECTION("non-empties")
    {
        AABB aabb1(Vector3f(0, 0, 0), Vector3f(1, 1, 1));
        AABB aabb2(Vector3f(1, 1, 1), Vector3f(1, 1, 1));
        AABB aabb3(Vector3f(1, 1, 1), Vector3f(1, 1, 1));
        CHECK(aabb1 != aabb2);
        CHECK_FALSE(aabb1 == aabb2);
        CHECK(aabb1 != aabb3);
        CHECK_FALSE(aabb1 == aabb3);

        CHECK(aabb2 == aabb3);
        CHECK_FALSE(aabb2 != aabb3);
    }
    SECTION("empties with non-empties")
    {
        AABB aabb1(Vector3f(0, 0, 0), Vector3f(1, 1, 1));
        AABB aabb2(AABB::Empty);
        CHECK(aabb1 != aabb2);
        CHECK_FALSE(aabb1 == aabb2);
    }
}

TEST_CASE("math/shapes/bounds(AABB, AABB)")
{
    SECTION("non-empties")
    {
        AABB aabb1(Vector3f(-2, -2, -2), Vector3f(-1, -1, -1));
        AABB aabb2(Vector3f(1, 1, 1), Vector3f(2, 2, 2));
        AABB aabb_inside_aabb1(Vector3f(-1.75, -1.75, -1.75), Vector3f(-1.25, -1.25, -1.25));
        AABB aabb_overlapping_aabb1(Vector3f(-1.5, -1.5, -1.5), Vector3f(-0.5, -0.5, -0.5));

        CHECK(bounds(aabb1, aabb2) == AABB(Vector3f(-2, -2, -2), Vector3f(2, 2, 2)));
        CHECK(bounds(aabb1, aabb_inside_aabb1) == aabb1);
        CHECK(bounds(aabb1, aabb_overlapping_aabb1) == AABB(Vector3f(-2, -2, -2), Vector3f(-0.5, -0.5, -0.5)));
    }
    SECTION("empties with non-empties")
    {
        AABB aabb_empty(Vector3f(10, 10, 10), Vector3f(-10, -10, -10));
        AABB aabb1(Vector3f(0, 0, 0), Vector3f(1, 1, 1));
        CHECK(!aabb1.empty());

        CHECK(bounds(aabb_empty, aabb1) == aabb1);
    }
    SECTION("empties")
    {
        AABB aabb1(AABB::Empty);
        AABB aabb2(Vector3f(0, 0, 0), Vector3f(-1, -1, -1));

        CHECK(bounds(aabb1, aabb2) == aabb1);
        CHECK(bounds(aabb1, aabb2) == aabb2);
    }
}

