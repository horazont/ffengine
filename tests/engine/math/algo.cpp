/**********************************************************************
File name: algo.cpp
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

#include "ffengine/math/algo.hpp"


typedef RasterIterator<float> TestIterator;


TEST_CASE("math/raster_line_inclusive/case1")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(0, 0, 1.5, 3.0), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(0, 0),
                                                         std::make_tuple(0, 1),
                                                         std::make_tuple(0, 2),
                                                         std::make_tuple(1, 2),
                                                     });
    CHECK(data == reference);
}

TEST_CASE("math/raster_line_inclusive/case2")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(0, 0, 1.6, 3.0), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(0, 0),
                                                         std::make_tuple(0, 1),
                                                         std::make_tuple(1, 1),
                                                         std::make_tuple(1, 2),
                                                     });
    CHECK(data == reference);
}

TEST_CASE("math/raster_line_inclusive/case3")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(0.1, 0.1, 1.6, 3.1), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(0, 0),
                                                         std::make_tuple(0, 1),
                                                         std::make_tuple(1, 1),
                                                         std::make_tuple(1, 2),
                                                         std::make_tuple(1, 3),
                                                     });
    CHECK(data == reference);
}

TEST_CASE("math/raster_line_inclusive/case4")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(0.1, 0.1, 1.7, 2.9), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(0, 0),
                                                         std::make_tuple(0, 1),
                                                         std::make_tuple(1, 1),
                                                         std::make_tuple(1, 2),
                                                     });
    CHECK(data == reference);
}

TEST_CASE("math/raster_line_inclusive/case5")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(1.7, 2.9, 0.1, 0.1), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(1, 2),
                                                         std::make_tuple(1, 1),
                                                         std::make_tuple(0, 1),
                                                         std::make_tuple(0, 0),
                                                     });
    CHECK(data == reference);
}

TEST_CASE("math/raster_line_inclusive/case6")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(1.7, 0.1, 0.1, 2.9), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(1, 0),
                                                         std::make_tuple(1, 1),
                                                         std::make_tuple(0, 1),
                                                         std::make_tuple(0, 2),
                                                     });
    CHECK(data == reference);
}

TEST_CASE("math/raster_line_inclusive/case7")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(0, 8.9, 0, 10.5), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(0, 8),
                                                         std::make_tuple(0, 9),
                                                         std::make_tuple(0, 10),
                                                     });
    CHECK(data == reference);
}

TEST_CASE("math/raster_line_inclusive/case8")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(22.2, 23.0, 5.46, 22.35), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(22, 23),
                                                         std::make_tuple(22, 22),
                                                         std::make_tuple(21, 22),
                                                         std::make_tuple(20, 22),
                                                         std::make_tuple(19, 22),
                                                         std::make_tuple(18, 22),
                                                         std::make_tuple(17, 22),
                                                         std::make_tuple(16, 22),
                                                         std::make_tuple(15, 22),
                                                         std::make_tuple(14, 22),
                                                         std::make_tuple(13, 22),
                                                         std::make_tuple(12, 22),
                                                         std::make_tuple(11, 22),
                                                         std::make_tuple(10, 22),
                                                         std::make_tuple(9, 22),
                                                         std::make_tuple(8, 22),
                                                         std::make_tuple(7, 22),
                                                         std::make_tuple(6, 22),
                                                         std::make_tuple(5, 22)
                                                     });
    CHECK(data == reference);
}

TEST_CASE("math/raster_line_inclusive/case9")
{
    std::vector<std::tuple<float, float> > data;
    std::copy(TestIterator(0, 0, 11.62, 0), TestIterator(),
              std::back_inserter(data));

    std::vector<std::tuple<float, float> > reference({
                                                         std::make_tuple(0, 0),
                                                         std::make_tuple(1, 0),
                                                         std::make_tuple(2, 0),
                                                         std::make_tuple(3, 0),
                                                         std::make_tuple(4, 0),
                                                         std::make_tuple(5, 0),
                                                         std::make_tuple(6, 0),
                                                         std::make_tuple(7, 0),
                                                         std::make_tuple(8, 0),
                                                         std::make_tuple(9, 0),
                                                         std::make_tuple(10, 0),
                                                         std::make_tuple(11, 0)
                                                     });
    CHECK(data == reference);
}
