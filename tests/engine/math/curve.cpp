/**********************************************************************
File name: curve.cpp
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

#include "ffengine/math/curve.hpp"


TEST_CASE("math/curve/QuadBezier/QuadBezier(p1, p2, p3)")
{
    QuadBezier3f curve(Vector3f(0, 0, 0),
                       Vector3f(1, 0, 0),
                       Vector3f(1, 1, 0));

    CHECK(curve.p1 == Vector3f(0, 0, 0));
    CHECK(curve.p2 == Vector3f(1, 0, 0));
    CHECK(curve.p3 == Vector3f(1, 1, 0));
}

TEST_CASE("math/curve/QuadBezier/QuadBezier(const QuadBezier&)")
{
    QuadBezier3f curve1(Vector3f(0, 0, 0),
                        Vector3f(0, 1, 0),
                        Vector3f(1, 1, 0));

    CHECK(curve1.p1 == Vector3f(0, 0, 0));
    CHECK(curve1.p2 == Vector3f(0, 1, 0));
    CHECK(curve1.p3 == Vector3f(1, 1, 0));

    QuadBezier3f curve2(curve1);

    CHECK(curve2.p1 == Vector3f(0, 0, 0));
    CHECK(curve2.p2 == Vector3f(0, 1, 0));
    CHECK(curve2.p3 == Vector3f(1, 1, 0));
}

TEST_CASE("math/curve/QuadBezier/equality")
{
    QuadBezier3f curve1(Vector3f(0, 0, 0),
                        Vector3f(0, 1, 0),
                        Vector3f(0, 0, 1));

    QuadBezier3f curve2(Vector3f(0, 0, 0),
                        Vector3f(0, 1, 0),
                        Vector3f(0, 0, 1));

    QuadBezier3f curve3(Vector3f(0, 0, 0),
                        Vector3f(0, 1, 1),
                        Vector3f(0, 0, 1));

    CHECK(curve1 == curve2);
    CHECK_FALSE(curve1 != curve2);
    CHECK(curve1 != curve3);
    CHECK_FALSE(curve1 == curve3);
    CHECK(curve2 != curve3);
    CHECK_FALSE(curve2 == curve3);
}

TEST_CASE("math/curve/QuadBezier/[]")
{
    QuadBezier3f curve(Vector3f(0, 0, 0),
                       Vector3f(1, 0, 0),
                       Vector3f(1, 1, 0));

    CHECK(curve[0.f] == Vector3f(0, 0, 0));
    CHECK(curve[0.25f] == Vector3f(0.0625, 0.0625, 0));
    CHECK(curve[0.5f] == Vector3f(0.25, 0.25, 0));
    CHECK(curve[0.75f] == Vector3f(0.5625, 0.5625, 0));
    CHECK(curve[1.f] == Vector3f(1, 1, 0));
}