/**********************************************************************
File name: vector.cpp
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

#include <engine/math/vector.hpp>

TEST_CASE("math/vector/Vector2/operations"
          "Test vector2 operations")
{
    Vector2 ex(1, 0);
    Vector2 ey(0, 1);
    Vector2 a(1, 1);

    const double anormcomp = 1./sqrt(2);

    CHECK(ex.length() == 1);
    CHECK(ey.length() == 1);
    CHECK(a.length() == sqrt(2));

    CHECK(a.normalized() == Vector2(anormcomp, anormcomp));

    CHECK((ex + ex) == Vector2(2, 0));
    CHECK((ex*ex) == 1);
    CHECK((ex*a) == 1);
    CHECK((ey*ex) == 0);
    CHECK((2*ex) == Vector2(2, 0));
    CHECK((2*a) == Vector2(2, 2));
    CHECK((ex-a) == Vector2(0, -1));

    CHECK(&a.as_array[0] == &a[eX]);
    CHECK(&a.as_array[1] == &a[eY]);
}

TEST_CASE("math/vector/Vector3/operations"
          "Test vector3 operations")
{
    Vector3 ex(1, 0, 0);
    Vector3 ey(0, 1, 0);
    Vector3 ez(0, 0, 1);
    Vector3 a(1, 1, 1);

    const double anormcomp = 1./sqrt(3);

    CHECK(ex.length() == 1);
    CHECK(ey.length() == 1);
    CHECK(ez.length() == 1);
    CHECK(a.length() == sqrt(3));

    CHECK(a.normalized() == Vector3(anormcomp, anormcomp, anormcomp));

    CHECK(Vector2(a) == Vector2(1, 1));

    CHECK((ex + ey + ez) == a);
    CHECK((ex*ex) == 1);
    CHECK((ex*ey) == 0);
    CHECK((ex*ez) == 0);
    CHECK((ex*a) == 1);
    CHECK((ex-a) == Vector3(0, -1, -1));

    CHECK(&a.as_array[0] == &a[eX]);
    CHECK(&a.as_array[1] == &a[eY]);
    CHECK(&a.as_array[2] == &a[eZ]);

    CHECK((ex%ey) == ez);
}

TEST_CASE("math/vector/Vector4/operations"
          "Test vector4 operations")
{
    Vector4 ex(1, 0, 0, 0);
    Vector4 ey(0, 1, 0, 0);
    Vector4 ez(0, 0, 1, 0);
    Vector4 ew(0, 0, 0, 1);
    Vector4 a(1, 1, 1, 1);


    const double anormcomp = 1./2.;


    CHECK(ex.length() == 1);
    CHECK(ey.length() == 1);
    CHECK(ez.length() == 1);
    CHECK(ew.length() == 1);
    CHECK(a.length() == 2);

    CHECK(a.normalized() == Vector4(anormcomp, anormcomp, anormcomp, anormcomp));


    CHECK((ex + ex) == Vector4(2, 0, 0, 0));
    CHECK((ex*ex) == 1);
    CHECK((ex*a) == 1);
    CHECK((ey*ex) == 0);
    CHECK((2*ex) == Vector4(2, 0, 0, 0));
    CHECK((2*a) == Vector4(2, 2, 2, 2));
    CHECK((ex-a) == Vector4(0, -1, -1, -1));

    CHECK(&a.as_array[0] == &a[eX]);
    CHECK(&a.as_array[1] == &a[eY]);
    CHECK(&a.as_array[2] == &a[eZ]);
    CHECK(&a.as_array[3] == &a[eW]);
}
