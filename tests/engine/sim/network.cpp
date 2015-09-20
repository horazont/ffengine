/**********************************************************************
File name: network.cpp
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

#include <iostream>

#include "ffengine/sim/network.hpp"


using namespace sim;

template <typename f>
typename std::enable_if<std::is_arithmetic<f>::value>::type CHECK_APPROX_ZERO(f value)
{
    CHECK(value <= 1e-5);
}

template <typename f, unsigned int n>
void CHECK_APPROX_ZERO(Vector<f, n> value)
{
    CHECK(value.abssum() <= 1e-5);
}

template <typename f>
void CHECK_APPROX_EQUAL(f value1, f value2)
{
    CHECK_APPROX_ZERO(value1 - value2);
}

/*template <typename float_t, unsigned int n>
void CHECK_APPROX_ZERO<Vector<float_t, n>>(Vector<float_t, n> value)
{
    CHECK_APPROX_ZERO(value.abssum() <= 1e-5);
}*/


TEST_CASE("sim/network/offset_segments/concave_corner")
{
    std::vector<PhysicalEdgeSegment> segments;
    segments.emplace_back(0, Vector3f(0, 0, 0), Vector3f(0, 1, 0));
    segments.emplace_back(1, Vector3f(0, 1, 0), Vector3f(1, 0, 0));

    std::vector<PhysicalEdgeSegment> result;
    offset_segments(segments, 0.5, Vector3f(1, 0, 0), result);

    std::vector<PhysicalEdgeSegment> expected;
    expected.emplace_back(0, Vector3f(0.5, 0, 0), Vector3f(0, 0.5, 0));
    expected.emplace_back(0.5, Vector3f(0.5, 0.5, 0), Vector3f(0.5, 0, 0));

    CHECK(expected == result);
}


TEST_CASE("sim/network/offset_segments/convex_corner")
{
    std::vector<PhysicalEdgeSegment> segments;
    segments.emplace_back(0, Vector3f(0, 0, 0), Vector3f(0, 1, 0));
    segments.emplace_back(1, Vector3f(0, 1, 0), Vector3f(1, 0, 0));

    std::vector<PhysicalEdgeSegment> result;
    offset_segments(segments, -0.5, Vector3f(1, 0, 0), result);

    std::vector<PhysicalEdgeSegment> expected;
    expected.emplace_back(0, Vector3f(-0.5, 0, 0), Vector3f(0, 1.5, 0));
    expected.emplace_back(1.5, Vector3f(-0.5, 1.5, 0), Vector3f(1.5, 0, 0));

    CHECK(expected == result);
}


TEST_CASE("sim/network/offset_segments/both_corner_types")
{
    std::vector<PhysicalEdgeSegment> segments;
    segments.emplace_back(0, Vector3f(0, 0, 0), Vector3f(0, 1, 0));
    segments.emplace_back(1, Vector3f(0, 1, 0), Vector3f(1, 0, 0));
    segments.emplace_back(2, Vector3f(1, 1, 0), Vector3f(0, 1, 0));

    std::vector<PhysicalEdgeSegment> result;
    offset_segments(segments, 0.5, Vector3f(0, 1, 0), result);

    std::vector<PhysicalEdgeSegment> expected;
    expected.emplace_back(0, Vector3f(0.5, 0, 0), Vector3f(0, 0.5, 0));
    expected.emplace_back(0.5, Vector3f(0.5, 0.5, 0), Vector3f(1, 0, 0));
    expected.emplace_back(1.5, Vector3f(1.5, 0.5, 0), Vector3f(0, 1.5, 0));

    CHECK(expected == result);
}


TEST_CASE("sim/network/offset_segments/non_right_angle")
{
    std::vector<PhysicalEdgeSegment> segments;
    segments.emplace_back(0, Vector3f(0, 0, 0), Vector3f(1, 1, 0));
    segments.emplace_back(1, Vector3f(1, 1, 0), Vector3f(1, 0, 0));

    std::vector<PhysicalEdgeSegment> result;
    offset_segments(segments, 0.5, Vector3f(1, 0, 0), result);

    CHECK_APPROX_EQUAL(result[0].start, Vector3f(0.353553, -0.353553, 0));
    CHECK_APPROX_EQUAL(result[0].direction, Vector3f(0.853553, 0.853553, 0));
    CHECK_APPROX_EQUAL(result[1].start, Vector3f(1.20711, 0.5, 0));
    CHECK_APPROX_EQUAL(result[1].direction, Vector3f(0.792893, 0, 0));
}


TEST_CASE("sim/network/segmentize_curve/straight")
{
    const QuadBezier3f curve(Vector3f(0, 0, 0),
                             Vector3f(10, 0, 0),
                             Vector3f(20, 0, 0));

    std::vector<QuadBezier3f> result;
    segmentize_curve(curve, result);

    std::vector<QuadBezier3f> expected({
                                           QuadBezier3f(Vector3f(0, 0, 0), Vector3f(5, 0, 0), Vector3f(10, 0, 0)),
                                           QuadBezier3f(Vector3f(10, 0, 0), Vector3f(15, 0, 0), Vector3f(20, 0, 0)),
                                       });
    CHECK(expected == result);
}


TEST_CASE("sim/network/segmentize_curve/cut_short_segments")
{
    const QuadBezier3f curve(Vector3f(0, 0, 0),
                             Vector3f(6, 0, 0),
                             Vector3f(12, 0, 0));

    std::vector<QuadBezier3f> result;
    segmentize_curve(curve, result);

    std::vector<QuadBezier3f> expected({
                                           QuadBezier3f(Vector3f(0, 0, 0), Vector3f(6, 0, 0), Vector3f(12, 0, 0)),
                                       });
    CHECK(expected == result);
}
