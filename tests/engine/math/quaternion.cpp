/**********************************************************************
File name: quaternion.cpp
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

#include "ffengine/math/quaternion.hpp"

static constexpr double check_epsilon = 1e-14;
static constexpr double check_epsilon_bad = 1e-6;

#define CHECK_APPROX_EQUAL(a, b) CHECK((a-b).abssum() <= check_epsilon)
#define CHECK_APPROX_EQUAL_BAD(a, b) CHECK((a-b).abssum() <= check_epsilon_bad)


TEST_CASE("math/quaternion/Quaternion(raw)")
{
    Quaterniond q(1, 2, 3, 4);
    CHECK(q.as_array[0] == 1);
    CHECK(q.as_array[1] == 2);
    CHECK(q.as_array[2] == 3);
    CHECK(q.as_array[3] == 4);
}

TEST_CASE("math/quaternion/Quaternion()")
{
    Quaterniond q1;
    Quaterniond q(1, 2, 3, 4);

    CHECK(q1.as_array[0] == 1);
    CHECK(q1.as_array[1] == 0);
    CHECK(q1.as_array[2] == 0);
    CHECK(q1.as_array[3] == 0);

    Quaterniond q2 = q1 * q;

    CHECK(q2 == q);
}

TEST_CASE("math/quaternion/Quaternion(Quaternion)")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaternionf q2(q1);
    Quaterniond q3(q1);

    CHECK(q1 == q2);
    CHECK(q1 == q3);
}

TEST_CASE("math/quaternion/Quaternion(Vector3)")
{
    Quaterniond q(Vector3d(1, 2, 3));
    CHECK(q.as_array[0] == 0);
    CHECK(q.as_array[1] == 1);
    CHECK(q.as_array[2] == 2);
    CHECK(q.as_array[3] == 3);
}

TEST_CASE("math/quaternion/Quaternion/rot")
{
    Quaterniond q(Quaterniond::rot(1, Vector3f(2, 3, 4)));
    CHECK(q.as_array[0] == std::cos(0.5));
    CHECK(q.as_array[1] == 2*std::sin(0.5));
    CHECK(q.as_array[2] == 3*std::sin(0.5));
    CHECK(q.as_array[3] == 4*std::sin(0.5));
}

TEST_CASE("math/quaternion/Quaternion/equality")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(1, 2, 3, 4);
    Quaterniond q3(2, 2, 3, 4);
    Quaterniond q4(1, 3, 3, 4);
    Quaterniond q5(1, 2, 4, 4);
    Quaterniond q6(1, 2, 3, 5);
    CHECK(q1 == q2);
    CHECK_FALSE(q1 != q2);

    CHECK(q1 != q3);
    CHECK_FALSE(q1 == q3);
    CHECK(q1 != q4);
    CHECK_FALSE(q1 == q4);
    CHECK(q1 != q5);
    CHECK_FALSE(q1 == q5);
    CHECK(q1 != q6);
    CHECK_FALSE(q1 == q6);

    CHECK(q2 != q3);
    CHECK_FALSE(q2 == q3);
    CHECK(q2 != q4);
    CHECK_FALSE(q2 == q4);
    CHECK(q2 != q5);
    CHECK_FALSE(q2 == q5);
    CHECK(q2 != q6);
    CHECK_FALSE(q2 == q6);
}

TEST_CASE("math/quaternion/Quaternion/add")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(-1, -2, -3, -4);
    Quaterniond q3(2, 2, 2, 2);

    CHECK((q1+q1) == Quaterniond(2, 4, 6, 8));
    CHECK((q2+q2) == Quaterniond(-2, -4, -6, -8));
    CHECK((q3+q3) == Quaterniond(4, 4, 4, 4));
    CHECK((q1+q2) == Quaterniond(0, 0, 0, 0));
    CHECK((q1+q3) == Quaterniond(3, 4, 5, 6));
    CHECK((q2+q1) == Quaterniond(0, 0, 0, 0));
    CHECK((q3+q1) == Quaterniond(3, 4, 5, 6));
    CHECK((q3+q2) == Quaterniond(1, 0, -1, -2));
    CHECK((q2+q3) == Quaterniond(1, 0, -1, -2));
}

TEST_CASE("math/quaternion/Quaternion/subtract")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(-1, -2, -3, -4);
    Quaterniond q3(2, 2, 2, 2);

    CHECK((q1-q1) == Quaterniond(0, 0, 0, 0));
    CHECK((q2-q2) == Quaterniond(0, 0, 0, 0));
    CHECK((q3-q3) == Quaterniond(0, 0, 0, 0));
    CHECK((q1-q2) == Quaterniond(2, 4, 6, 8));
    CHECK((q1-q3) == Quaterniond(-1, 0, 1, 2));
    CHECK((q2-q1) == Quaterniond(-2, -4, -6, -8));
    CHECK((q3-q1) == Quaterniond(1, 0, -1, -2));
    CHECK((q3-q2) == Quaterniond(3, 4, 5, 6));
    CHECK((q2-q3) == Quaterniond(-3, -4, -5, -6));
}

TEST_CASE("math/quaternion/Quaternion/negate")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(-1, -2, -3, -4);
    Quaterniond q3(2, 2, 2, 2);

    CHECK((-q1) == Quaterniond(-1, -2, -3, -4));
    CHECK((-q2) == Quaterniond(1, 2, 3, 4));
    CHECK((-q3) == Quaterniond(-2, -2, -2, -2));
}

TEST_CASE("math/quaternion/Quaternion/conjugated")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(-1, -2, -3, -4);
    Quaterniond q3(2, 2, 2, 2);

    CHECK(q1.conjugated() == Quaterniond(1, -2, -3, -4));
    CHECK(q2.conjugated() == Quaterniond(-1, 2, 3, 4));
    CHECK(q3.conjugated() == Quaterniond(2, -2, -2, -2));
}

TEST_CASE("math/quaternion/Quaternion/vector")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(-1, -2, -3, -4);
    Quaterniond q3(2, 2, 2, 2);

    CHECK(q1.vector() == Vector3d(2, 3, 4));
    CHECK(q2.vector() == Vector3d(-2, -3, -4));
    CHECK(q3.vector() == Vector3d(2, 2, 2));
}

TEST_CASE("math/quaternion/Quaternion/real")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(-1, -2, -3, -4);
    Quaterniond q3(2, 2, 2, 2);

    CHECK(q1.real() == 1);
    CHECK(q2.real() == -1);
    CHECK(q3.real() == 2);
}

TEST_CASE("math/quaternion/Quaternion/abssum")
{
    CHECK(Quaterniond(0, 0, 0, 0).abssum() == 0);
    CHECK(Quaterniond(1, 0, 0, 0).abssum() == 1);
    CHECK(Quaterniond(-1, 0, 0, 0).abssum() == 1);
    CHECK(Quaterniond(0, 1, 0, 0).abssum() == 1);
    CHECK(Quaterniond(0, -1, 0, 0).abssum() == 1);
    CHECK(Quaterniond(0, 0, 1, 0).abssum() == 1);
    CHECK(Quaterniond(0, 0, -1, 0).abssum() == 1);
    CHECK(Quaterniond(0, 0, 0, 1).abssum() == 1);
    CHECK(Quaterniond(0, 0, 0, -1).abssum() == 1);
    CHECK(Quaterniond(1, -2, 3, -4).abssum() == 10);
    CHECK(Quaterniond(-4, 3, -2, 1).abssum() == 10);
}

TEST_CASE("math/quaternion/Quaternion/norm")
{
    CHECK(Quaterniond(0, 0, 0, 0).norm() == 0);
    CHECK(Quaterniond(1, 0, 0, 0).norm() == 1);
    CHECK(Quaterniond(-1, 0, 0, 0).norm() == 1);
    CHECK(Quaterniond(0, 1, 0, 0).norm() == 1);
    CHECK(Quaterniond(0, -1, 0, 0).norm() == 1);
    CHECK(Quaterniond(0, 0, 1, 0).norm() == 1);
    CHECK(Quaterniond(0, 0, -1, 0).norm() == 1);
    CHECK(Quaterniond(0, 0, 0, 1).norm() == 1);
    CHECK(Quaterniond(0, 0, 0, -1).norm() == 1);
    CHECK(Quaterniond(1, -2, 3, -4).norm() == std::sqrt(30));
    CHECK(Quaterniond(-4, 3, -2, 1).norm() == std::sqrt(30));
}

TEST_CASE("math/quaternion/Quaternion/hamiltonian_product")
{
    SECTION("x-axis-aligned euler quat")
    {
        Quaterniond q1(Quaterniond::rot(M_PI_2, Vector3d(1, 0, 0)));
        Quaterniond q2(Quaterniond::rot(M_PI_2, Vector3d(1, 0, 0)));

        Quaterniond qx = q1 * q2;

        CHECK_APPROX_EQUAL(qx, Quaterniond::rot(M_PI, Vector3d(1, 0, 0)));
    }
    SECTION("y-axis-aligned euler quat")
    {
        Quaterniond q1(Quaterniond::rot(M_PI_2, Vector3d(0, 1, 0)));
        Quaterniond q2(Quaterniond::rot(M_PI_2, Vector3d(0, 1, 0)));

        Quaterniond qx = q1 * q2;

        CHECK_APPROX_EQUAL(qx, Quaterniond::rot(M_PI, Vector3d(0, 1, 0)));
    }
    SECTION("z-axis-aligned euler quat")
    {
        Quaterniond q1(Quaterniond::rot(M_PI_2, Vector3d(0, 0, 1)));
        Quaterniond q2(Quaterniond::rot(M_PI_2, Vector3d(0, 0, 1)));

        Quaterniond qx = q1 * q2;

        CHECK_APPROX_EQUAL(qx, Quaterniond::rot(M_PI, Vector3d(0, 0, 1)));
    }
    SECTION("random axis euler quat")
    {
        Quaterniond q1(Quaterniond::rot(M_PI_2, Vector3f(1, 2, 3).normalized()));
        Quaterniond q2(Quaterniond::rot(M_PI_2, Vector3f(1, 2, 3).normalized()));

        Quaterniond qx = q1 * q2;

        CHECK_APPROX_EQUAL_BAD(qx, Quaterniond::rot(M_PI, Vector3d(1, 2, 3).normalized()));
    }
}

TEST_CASE("math/quaternion/Quaternion/scale")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(2, 2, 2, 2);

    CHECK((2*q1) == Quaterniond(2, 4, 6, 8));
    CHECK((q1*2) == Quaterniond(2, 4, 6, 8));
    CHECK((2*q2) == Quaterniond(4, 4, 4, 4));
    CHECK((q2*2) == Quaterniond(4, 4, 4, 4));

    CHECK((q1/2) == Quaterniond(0.5, 1, 1.5, 2));
    CHECK((q2/2) == Quaterniond(1, 1, 1, 1));
}

TEST_CASE("math/quaternion/Quaternion/normalize")
{
    Quaterniond q1(1, 2, 3, 4);
    Quaterniond q2(2, 2, 2, 2);

    const double q1_factor = q1.norm();
    const double q2_factor = q2.norm();

    CHECK(q1.normalized() == Quaterniond(1/q1_factor, 2/q1_factor, 3/q1_factor, 4/q1_factor));
    CHECK(q2.normalized() == Quaterniond(2/q2_factor, 2/q2_factor, 2/q2_factor, 2/q2_factor));
}

TEST_CASE("math/quaternion/Quaternion/rotate")
{
    SECTION("test 1")
    {
        Quaterniond qrot(Quaterniond::rot(M_PI_2, Vector3f(0, 0, 1)));
        Vector3f vec(1, 0, 0);
        Vector3f rotated = qrot.rotate(vec);

        CHECK_APPROX_EQUAL(rotated, Vector3f(0, 1, 0));
    }
    SECTION("test 2")
    {
        Quaterniond qrot(Quaterniond::rot(-M_PI_2, Vector3f(0, 1, 0)));
        Vector3f vec(1, 0, 0);
        Vector3f rotated = qrot.rotate(vec);

        CHECK_APPROX_EQUAL(rotated, Vector3f(0, 0, 1));
    }
    SECTION("test 3")
    {
        Quaterniond qrot(Quaterniond::rot(-M_PI_2, Vector3f(1, 0, 0)));
        Vector3f vec(1, 0, 0);
        Vector3f rotated = qrot.rotate(vec);

        CHECK_APPROX_EQUAL(rotated, Vector3f(1, 0, 0));
    }
}
