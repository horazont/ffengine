#include <catch.hpp>

#include <engine/math/matrix.hpp>

#define EPSILON 10e-16

#define CHECK_APPROX_ZERO(expr) CHECK((expr).abssum() < EPSILON)

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
