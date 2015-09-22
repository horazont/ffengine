/**********************************************************************
File name: plane.cpp
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

#include "ffengine/math/plane.hpp"


TEST_CASE("math/shapes/Plane/side_of(Sphere)/positive_normal")
{
    Plane plane(0, Vector3f(1, 0, 0));

    CHECK(plane.side_of(Sphere{Vector3f(2, 0, 0), 1.9}) == PlaneSide::POSITIVE_NORMAL);
    CHECK(plane.side_of(Sphere{Vector3f(2, 3, 4), 1.9}) == PlaneSide::POSITIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Sphere)/positive_normal/with_displacement")
{
    Plane plane(2, Vector3f(1, 0, 0));

    CHECK(plane.side_of(Sphere{Vector3f(4, 0, 0), 1.9}) == PlaneSide::POSITIVE_NORMAL);
    CHECK(plane.side_of(Sphere{Vector3f(4, 3, 4), 1.9}) == PlaneSide::POSITIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Sphere)/negative_normal")
{
    Plane plane(0, Vector3f(1, 0, 0));

    CHECK(plane.side_of(Sphere{Vector3f(-2, 0, 0), 1.9}) == PlaneSide::NEGATIVE_NORMAL);
    CHECK(plane.side_of(Sphere{Vector3f(-2, 3, 4), 1.9}) == PlaneSide::NEGATIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Sphere)/negative_normal/with_displacement")
{
    Plane plane(2, Vector3f(-1, 0, 0));

    CHECK(plane.side_of(Sphere{Vector3f(4, 0, 0), 1.9}) == PlaneSide::NEGATIVE_NORMAL);
    CHECK(plane.side_of(Sphere{Vector3f(4, 3, 4), 1.9}) == PlaneSide::NEGATIVE_NORMAL);
    CHECK(plane.side_of(Sphere{Vector3f(0, 3, 4), 1.9}) == PlaneSide::NEGATIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Sphere)/intersection")
{
    Plane plane(0, Vector3f(1, 0, 0));

    CHECK(plane.side_of(Sphere{Vector3f(-2, 0, 0), 2.1}) == PlaneSide::BOTH);
    CHECK(plane.side_of(Sphere{Vector3f(2, 0, 0), 2.1}) == PlaneSide::BOTH);
    CHECK(plane.side_of(Sphere{Vector3f(2, 3, 4), 2.1}) == PlaneSide::BOTH);
    CHECK(plane.side_of(Sphere{Vector3f(-2, -3, 4), 2.1}) == PlaneSide::BOTH);
}

TEST_CASE("math/shapes/Plane/side_of(Sphere)/intersection/with_deplacement")
{
    Plane plane(-2, Vector3f(-1, 0, 0));

    CHECK(plane.side_of(Sphere{Vector3f(2, 0, 0), 2.1}) == PlaneSide::BOTH);
    CHECK(plane.side_of(Sphere{Vector3f(2, 3, 4), 2.1}) == PlaneSide::BOTH);
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

TEST_CASE("math/shapes/Plane/side_of_fast(AABB)/positive_normal/frustum_specific_test")
{
    Plane plane(-1, Vector3f(1, 0, 0));

    CHECK(plane.side_of_fast(AABB{Vector3f(-0.1, -0.1, -0.1), Vector3f(0.1, 0.1, 0.1)}) == PlaneSide::POSITIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Vector3f)/above")
{
    Plane plane(0, Vector3f(1, 0, 0));
    CHECK(plane.side_of(Vector3f(1, 0, 0)) == PlaneSide::POSITIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Vector3f)/below")
{
    Plane plane(0, Vector3f(1, 0, 0));
    CHECK(plane.side_of(Vector3f(-1, 0, 0)) == PlaneSide::NEGATIVE_NORMAL);
}

TEST_CASE("math/shapes/Plane/side_of(Vector3f)/on")
{
    Plane plane(0, Vector3f(1, 0, 0));
    CHECK(plane.side_of(Vector3f(0, 0, 0)) == PlaneSide::BOTH);
}

TEST_CASE("math/shapes/Plane/comparision")
{
    Plane plane1(Vector4f(1, 0, 0, 10));
    Plane plane2(Vector4f(1, 0, 0, 10));
    Plane plane3(Vector4f(1, 0, 0, 4));

    CHECK(plane1 == plane2);
    CHECK_FALSE(plane1 != plane2);
    CHECK(plane1 != plane3);
    CHECK_FALSE(plane1 == plane3);
    CHECK(plane2 != plane3);
    CHECK_FALSE(plane2 == plane3);
}

TEST_CASE("math/shapes/Plane/Plane(origin, normal)")
{
    {
        Plane plane1(Vector3f(10, 3, 3), Vector3f(0, 1, 0));
        CHECK(plane1 == Plane(Vector4f(0, 1, 0, 3)));
    }
    {
        Plane plane1(Vector3f(10, 3, 3), Vector3f(0, 4, 0));
        CHECK(plane1 == Plane(Vector4f(0, 1, 0, 3)));
    }
}

TEST_CASE("math/shapes/Plane/Plane(dist, normal)")
{
    {
        Plane plane1(3, Vector3f(0, 1, 0));
        CHECK(plane1 == Plane(Vector4f(0, 1, 0, 3)));
    }
    {
        Plane plane1(3, Vector3f(0, 4, 0));
        CHECK(plane1 == Plane(Vector4f(0, 1, 0, 12)));
    }
}

TEST_CASE("math/shapes/Plane/Plane(homogenous)")
{
    {
        Plane plane1(Vector4f(1, 0, 0, 2));
        CHECK(plane1 == Plane(Vector4f(1, 0, 0, 2)));
    }
    {
        Plane plane1(Vector4f(0, 4, 0, 3));
        CHECK(plane1 == Plane(Vector4f(0, 1, 0, 0.75)));
    }
}

TEST_CASE("math/shapes/Plane/from_frustum_matrix")
{
    {
        Plane plane1 = Plane::from_frustum_matrix(Vector4f(1, 0, 0, 2));
        CHECK(plane1 == Plane(Vector4f(1, 0, 0, -2)));
    }
}
