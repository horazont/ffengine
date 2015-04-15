#include <catch.hpp>

#include "engine/math/rect.hpp"

typedef GenericRect<unsigned int> Rect;


TEST_CASE("math/rect/Rect/init_with_vectors")
{
    Rect::point_t p0(0, 1);
    Rect::point_t p1(2, 3);

    SECTION("two vectors")
    {
        Rect r(p0, p1);
        CHECK(r.x0() == 0);
        CHECK(r.x1() == 2);
        CHECK(r.y0() == 1);
        CHECK(r.y1() == 3);

        CHECK(r.is_a_rect());
    }

    SECTION("one vector")
    {
        Rect r(p0);
        CHECK(r.x0() == 0);
        CHECK(r.x1() == 0);
        CHECK(r.y0() == 1);
        CHECK(r.y1() == 1);

        CHECK(r.is_a_rect());
    }
}

TEST_CASE("math/rect/Rect/xy01")
{
    Rect r(0, 1, 2, 3);
    CHECK(r.x0() == 0);
    CHECK(r.x1() == 2);
    CHECK(r.y0() == 1);
    CHECK(r.y1() == 3);

    r.set_x0(10);
    CHECK(r.x0() == 10);
    CHECK(r.x1() == 2);

    CHECK_FALSE(r.is_a_rect());
}

TEST_CASE("math/rect/Rect/is_a_rect")
{
    CHECK(Rect(0, 0, 1, 1).is_a_rect());
    CHECK_FALSE(Rect(2, 2, 1, 1).is_a_rect());
    CHECK_FALSE(Rect(0, 2, 1, 1).is_a_rect());
    CHECK_FALSE(Rect(2, 0, 1, 1).is_a_rect());
}

TEST_CASE("math/rect/Rect/copy_construct")
{
    const Rect r1(0, 1, 2, 3);
    Rect r2(r1);

    CHECK(r2.x0() == 0);
    CHECK(r2.x1() == 2);
    CHECK(r2.y0() == 1);
    CHECK(r2.y1() == 3);
}

TEST_CASE("math/rect/Rect/copy_assign")
{
    const Rect r1(0, 1, 2, 3);
    Rect r2;

    r2 = r1;
    CHECK(r2.x0() == 0);
    CHECK(r2.x1() == 2);
    CHECK(r2.y0() == 1);
    CHECK(r2.y1() == 3);
}

TEST_CASE("math/rect/Rect/equality")
{
    const Rect r1(0, 1, 2, 3);
    SECTION ("not equal")
    {
        const Rect r2(1, 1, 2, 3);
        CHECK(r2 != r1);
        CHECK(r1 != r2);
        CHECK_FALSE(r2 == r1);
        CHECK_FALSE(r1 == r2);
    }
    SECTION("equal")
    {
        const Rect r2(0, 1, 2, 3);
        CHECK(r2 == r1);
        CHECK(r1 == r2);
        CHECK_FALSE(r2 != r1);
        CHECK_FALSE(r1 != r2);
    }


}

TEST_CASE("math/rect/Rect/area")
{
    CHECK(Rect(0, 1, 2, 3).area() == 4);
    CHECK(Rect(0, 0, 10, 10).area() == 100);
}

TEST_CASE("math/rect/Rect/NotARect")
{
    SECTION("copy into rect")
    {
        Rect r(0, 1, 2, 3);
        CHECK(r.is_a_rect());
        r = NotARect;
        CHECK_FALSE(r.is_a_rect());
    }
    SECTION("construct")
    {
        Rect r(NotARect);
        CHECK_FALSE(r.is_a_rect());
    }
    SECTION("compare with a non-rect")
    {
        Rect r(2, 2, 0, 0);
        REQUIRE_FALSE(r.is_a_rect());
        CHECK(r == NotARect);
        CHECK(NotARect == r);
        CHECK_FALSE(r != NotARect);
        CHECK_FALSE(NotARect != r);
    }
    SECTION("compare with a rect")
    {
        Rect r(0, 0, 2, 2);
        REQUIRE(r.is_a_rect());
        CHECK(r != NotARect);
        CHECK(NotARect != r);
        CHECK_FALSE(r == NotARect);
        CHECK_FALSE(NotARect == r);
    }
    SECTION("intersect")
    {
        Rect r1(0, 0, 2, 2);
        Rect r2 = r1 & NotARect;
        CHECK(r2 == NotARect);
    }
}

TEST_CASE("math/rect/Rect/intersection")
{
    const Rect r1(0, 0, 4, 4);
    SECTION("case subquad")
    {
        CHECK((r1 & Rect(1, 1, 2, 2)) == Rect(1, 1, 2, 2));
    }
    SECTION("case subrect")
    {
        CHECK((r1 & Rect(1, 1, 3, 2)) == Rect(1, 1, 3, 2));
    }
    SECTION("case superrect")
    {
        CHECK((r1 & Rect(0, 0, 10, 10)) == Rect(0, 0, 4, 4));
    }
    SECTION("case true intersection")
    {
        CHECK((r1 & Rect(1, 1, 10, 10)) == Rect(1, 1, 4, 4));
    }
}

TEST_CASE("math/rect/Rect/empty")
{
    CHECK(Rect(0, 0, 0, 0).empty());
    CHECK(Rect(1, 1, 1, 1).empty());
    CHECK(Rect(2, 3, 2, 3).empty());
    CHECK_FALSE(Rect(1, 2, 3, 4).empty());

    CHECK(Rect(NotARect).empty());
}

TEST_CASE("math/rect/Rect/bounds")
{
    SECTION("normal rects")
    {
        Rect r1(0, 0, 10, 10);
        Rect r2(5, 0, 13, 5);

        CHECK(bounds(r1, r2) == Rect(0, 0, 13, 10));
    }
    SECTION("one operand is NotARect")
    {
        Rect r1(20, 20, 30, 30);

        CHECK(bounds(r1, Rect(NotARect)) == r1);
        CHECK(bounds(Rect(NotARect), r1) == r1);
    }
}
