#include <catch.hpp>

#include "engine/math/algo.hpp"


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

