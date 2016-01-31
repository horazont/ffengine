/**********************************************************************
File name: mixedcurve.cpp
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

#include "ffengine/math/mixedcurve.hpp"


TEST_CASE("math/mixedcurve/MixedCurve/MixedCurve()")
{
    const MixedCurvef curve;

    CHECK(curve.curve() == CubeBezier3f());
}

TEST_CASE("math/mixedcurve/MixedCurve/MixedCurve(const QuadBezier3&)")
{
    const QuadBezier3f quad_curve(
                Vector3f(1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 1, 0));
    const MixedCurvef curve(quad_curve);

    for (float t = 0.f; t <= 1.f; t += 0.02f) {
        CHECK((curve.curve()[t] - quad_curve[t]).abssum() < 1e-5f);
    }
}

TEST_CASE("math/mixedcurve/MixedCurve/set_control/with_equal_zs")
{
    const QuadBezier3f quad_curve1(
                Vector3f(1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 1, 0));
    const QuadBezier3f quad_curve2(
                Vector3f(1, 0, 0),
                Vector3f(1, 2, 0),
                Vector3f(0, 1, 0));
    MixedCurvef curve1(quad_curve1);
    MixedCurvef curve2(quad_curve2);

    curve1.set_control(Vector2f(1, 2), 0, 0);

    CHECK(curve1.curve() == curve2.curve());
}

TEST_CASE("math/mixedcurve/MixedCurve/set_control/with_different_zs")
{
    const QuadBezier3f quad_curve1(
                Vector3f(1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 1, 1));
    const QuadBezier3f quad_curve2(
                Vector3f(1, 0, 0),
                Vector3f(1, 2, 0),
                Vector3f(0, 1, 1));
    MixedCurvef curve1(quad_curve1);
    MixedCurvef curve2(quad_curve2);

    curve1.set_control(Vector2f(1, 2), 0, 1);

    CHECK(curve1.curve().p_control1[eX] == curve2.curve().p_control1[eX]);
    CHECK(curve1.curve().p_control1[eY] == curve2.curve().p_control1[eY]);
    CHECK(curve1.curve().p_control2[eX] == curve2.curve().p_control2[eX]);
    CHECK(curve1.curve().p_control2[eY] == curve2.curve().p_control2[eY]);

    CHECK(curve1.curve().p_control1[eZ] == 0);
    CHECK(curve1.curve().p_control2[eZ] == 1);
}

TEST_CASE("math/mixedcurve/MixedCurve/set_start")
{
    const QuadBezier3f quad_curve1(
                Vector3f(1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 1, 0));
    const QuadBezier3f quad_curve2(
                Vector3f(-1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 1, 0));
    MixedCurvef curve1(quad_curve1);
    MixedCurvef curve2(quad_curve2);

    curve1.set_start(Vector3f(-1, 0, 0));

    CHECK(curve1.curve() == curve2.curve());
}

TEST_CASE("math/mixedcurve/MixedCurve/set_end")
{
    const QuadBezier3f quad_curve1(
                Vector3f(1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 1, 0));
    const QuadBezier3f quad_curve2(
                Vector3f(1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 2, 0));
    MixedCurvef curve1(quad_curve1);
    MixedCurvef curve2(quad_curve2);

    curve1.set_end(Vector3f(0, 2, 0));

    CHECK(curve1.curve() == curve2.curve());
}

TEST_CASE("math/mixedcurve/MixedCurve/mutation_order_invariance")
{
    const QuadBezier3f quad_curve1(
                Vector3f(1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 1, 0));
    const QuadBezier3f quad_curve2(
                Vector3f(2, 0, 0),
                Vector3f(2, 2, 0),
                Vector3f(0, 2, 0));

    SECTION("end_control_start")
    {
        MixedCurvef curve1(quad_curve1);
        MixedCurvef curve2(quad_curve2);

        curve1.set_end(Vector3f(0, 2, 0));
        curve1.set_control(Vector2f(2, 2), 0, 0);
        curve1.set_start(Vector3f(2, 0, 0));

        CHECK(curve1.curve() == curve2.curve());
    }

    SECTION("end_start_control")
    {
        MixedCurvef curve1(quad_curve1);
        MixedCurvef curve2(quad_curve2);

        curve1.set_end(Vector3f(0, 2, 0));
        curve1.set_start(Vector3f(2, 0, 0));
        curve1.set_control(Vector2f(2, 2), 0, 0);

        CHECK(curve1.curve() == curve2.curve());
    }

    SECTION("start_end_control")
    {
        MixedCurvef curve1(quad_curve1);
        MixedCurvef curve2(quad_curve2);

        curve1.set_start(Vector3f(2, 0, 0));
        curve1.set_end(Vector3f(0, 2, 0));
        curve1.set_control(Vector2f(2, 2), 0, 0);

        CHECK(curve1.curve() == curve2.curve());
    }

    SECTION("start_control_end")
    {
        MixedCurvef curve1(quad_curve1);
        MixedCurvef curve2(quad_curve2);

        curve1.set_start(Vector3f(2, 0, 0));
        curve1.set_control(Vector2f(2, 2), 0, 0);
        curve1.set_end(Vector3f(0, 2, 0));

        CHECK(curve1.curve() == curve2.curve());
    }

    SECTION("control_start_end")
    {
        MixedCurvef curve1(quad_curve1);
        MixedCurvef curve2(quad_curve2);

        curve1.set_control(Vector2f(2, 2), 0, 0);
        curve1.set_start(Vector3f(2, 0, 0));
        curve1.set_end(Vector3f(0, 2, 0));

        CHECK(curve1.curve() == curve2.curve());
    }

    SECTION("control_end_start")
    {
        MixedCurvef curve1(quad_curve1);
        MixedCurvef curve2(quad_curve2);

        curve1.set_control(Vector2f(2, 2), 0, 0);
        curve1.set_end(Vector3f(0, 2, 0));
        curve1.set_start(Vector3f(2, 0, 0));

        CHECK(curve1.curve() == curve2.curve());
    }
}

TEST_CASE("math/mixedcurve/MixedCurve/set_control(const Vector3&)")
{
    const QuadBezier3f quad_curve1(
                Vector3f(1, 0, 1),
                Vector3f(1, 1, 1),
                Vector3f(0, 1, 3));
    const QuadBezier3f quad_curve2(
                Vector3f(1, 0, 1),
                Vector3f(1, 1, 2),
                Vector3f(0, 1, 3));
    MixedCurvef curve1(quad_curve1);
    MixedCurvef curve2(quad_curve2);

    curve1.set_control(Vector3f(1, 1, 2));

    CHECK(curve1.curve() == curve2.curve());
}

TEST_CASE("math/mixedcurve/MixedCurve/set_qcurve")
{
    const QuadBezier3f quad_curve1(
                Vector3f(1, 0, 1),
                Vector3f(1, 1, 1),
                Vector3f(0, 1, 1));
    const QuadBezier3f quad_curve2(
                Vector3f(1, 0, 2),
                Vector3f(1, 1, 2),
                Vector3f(0, 2, 2));
    MixedCurvef curve1(quad_curve1);
    MixedCurvef curve2(quad_curve2);

    curve1.set_qcurve(quad_curve2);

    CHECK(curve1.curve() == curve2.curve());
}

TEST_CASE("math/mixedcurve/MixedCurve/curve_2d")
{
    const QuadBezier3f quad_curve(
                Vector3f(1, 0, 0),
                Vector3f(1, 1, 0),
                Vector3f(0, 1, 0));
    const QuadBezier<Vector2f> quad_curve_2d(
                Vector2f(1, 0),
                Vector2f(1, 1),
                Vector2f(0, 1));
    const MixedCurvef curve(quad_curve);

    for (float t = 0.f; t <= 1.f; t += 0.02f) {
        CHECK((curve.curve_2d()[t] - quad_curve_2d[t]).abssum() < 1e-5f);
    }
}
