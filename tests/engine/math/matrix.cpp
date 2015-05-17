/**********************************************************************
File name: matrix.cpp
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

#include <ffengine/math/matrix.hpp>

#define EPSILON 10e-16

#define CHECK_APPROX_ZERO(expr) CHECK((expr).abssum() <= EPSILON)

TEST_CASE("math/matrix/Matrix4/init0"
          "Test initialization with zeros")
{
    Matrix4 id(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    );
    CHECK(Matrix4() == id);
}

TEST_CASE("math/matrix/Matrix4/Identity"
          "Test creation of identity matrix")
{
    Matrix4 id(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    CHECK(Matrix4(Identity) == id);
}

TEST_CASE("math/matrix/rotation4(eX)"
          "Test creation of rotation matrix around X axis")
{
    double alpha = 0.1532*3.14159;
    double sin_alpha = sin(alpha);
    double cos_alpha = cos(alpha);
    Matrix4 ref(
        1, 0, 0, 0,
        0, cos_alpha, -sin_alpha, 0,
        0, sin_alpha, cos_alpha, 0,
        0, 0, 0, 1
    );

    CHECK(ref == rotation4(eX, alpha));
    // this isn't exactly equal, thus the check using the difference
    CHECK_APPROX_ZERO(ref - rotation4(Vector3(1, 0, 0), alpha));
}

TEST_CASE("math/matrix/rotation4(eY)"
          "Test creation of rotation matrix around Y axis")
{
    double alpha = 0.6182*3.14159;
    double sin_alpha = sin(alpha);
    double cos_alpha = cos(alpha);
    Matrix4 ref(
        cos_alpha, 0, sin_alpha, 0,
        0, 1, 0, 0,
        -sin_alpha, 0, cos_alpha, 0,
        0, 0, 0, 1
    );

    CHECK(ref == rotation4(eY, alpha));
    // this isn't exactly equal, thus the check using the difference
    CHECK_APPROX_ZERO(ref - rotation4(Vector3(0, 1, 0), alpha));
}

TEST_CASE("math/matrix/rotation4(eZ)"
          "Test creation of rotation matrix around Z axis")
{
    double alpha = 0.6182*3.14159;
    double sin_alpha = sin(alpha);
    double cos_alpha = cos(alpha);
    Matrix4 ref(
        cos_alpha, -sin_alpha, 0, 0,
        sin_alpha, cos_alpha, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    CHECK(ref == rotation4(eZ, alpha));
    // this isn't exactly equal, thus the check using the difference
    CHECK_APPROX_ZERO(ref - rotation4(Vector3(0, 0, 1), alpha));
}

TEST_CASE("math/matrix/translation"
          "Test creation of translation matrix")
{
    Vector3 trans(1, 2, 3);
    Matrix4 ref(
        1, 0, 0, 1,
        0, 1, 0, 2,
        0, 0, 1, 3,
        0, 0, 0, 1
    );

    CHECK(ref == translation4(trans));
}

TEST_CASE("math/matrix/scale"
          "Test creation of scale matrix")
{
    Vector3 factors(1, 2, 3);
    Matrix4 ref(
        1, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 3, 0,
        0, 0, 0, 1
    );

    CHECK(ref == scale4(factors));
}

TEST_CASE("math/matrix/matrix_vector_product"
          "Test matrix-vector product and matrix product")
{
    Matrix3 rot1 = rotation3(eX, M_PI_2l);
    Matrix3 rot2 = rotation3(eX, M_PIl);
    Matrix3 rot3 = rotation3(eY, M_PI_2l);

    Vector3 ex(1, 0, 0);
    Vector3 ey(0, 1, 0);

    CHECK((rot1*ex) == ex);
    CHECK((rot2*ex) == ex);

    CHECK_APPROX_ZERO((rot1*ey) - Vector3(0, 0, 1));
    CHECK_APPROX_ZERO((rot2*ey) - Vector3(0, -1, 0));

    Matrix3 scale(
        -1, 0, 0,
        0, -1, 0,
        0, 0, -1
    );

    CHECK((scale*rot1) == (-rot1));

    CHECK((scale*ex) == (-ex));
    CHECK((scale*ey) == (-ey));

    CHECK(((scale*rot1)*ex) == (-ex));
    CHECK_APPROX_ZERO((rot1*scale)*ey - Vector3(0, 0, -1));

    CHECK_APPROX_ZERO(((rot3*rot1)*ey) - ex);
}

TEST_CASE("math/matrix/invert/mat2")
{
    Matrix2 m(-1, -2,
              -1, 0.0);

    invert(m);
    CHECK(Matrix2(0, -0.5, -1, 0.5) == m);
}

TEST_CASE("math/matrix/invert_proj_matrix")
{
    Matrix4f m(1.34, 0, 0, 0,
               0, 1.79, 0, 0,
               0, 0, -1, -2,
               0, 0, -1, 0);

    invert_proj_matrix(m);

    Matrix4f diff = (Matrix4(0.746268656716418, 0, 0, 0,
                             0, 0.558659217877095, 0, 0,
                             0, 0, 0, -1,
                             0, 0, -0.5, 0.5) - m);
    CHECK_APPROX_ZERO(diff);
}

TEST_CASE("math/matrix/matrix_matrix_product/2×4_4×2")
{
    Matrix<float, 2, 4> m1(
                1, 2, 3, 4,
                5, 6, 7, 8
                );
    Matrix<float, 4, 2> m2(
                1, 0,
                0, 1,
                1, 1,
                0, 0);

    Matrix<float, 2, 2> result = m1 * m2;

    Matrix<float, 2, 2> expected(4, 5, 12, 13);
    CHECK_APPROX_ZERO(result - expected);
}

TEST_CASE("math/matrix/matrix_matrix_product/4×4_4×4")
{
    Matrix<float, 4, 4> t = translation4(-Vector3(30, 20, 30));
    std::cout << t << std::endl;
}
