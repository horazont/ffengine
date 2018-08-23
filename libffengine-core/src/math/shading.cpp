#include "ffengine/math/shading.hpp"

#include "ffengine/math/algo.hpp"

Vector3f importance_sample_ggx(const Vector2f Xi,
                               const float roughness,
                               const Vector3f N)
{
    const float a = sqr(roughness);

    const float phi = 2 * M_PI * Xi[eX];
    const float cos_theta = std::sqrt((1 - Xi[eY]) / (1 + (a*a - 1) * Xi[eY]));
    const float sin_theta = std::sqrt(1 - cos_theta * cos_theta);

    const Vector3f H(sin_theta * std::cos(phi),
                     sin_theta * std::sin(phi),
                     cos_theta);

    const Vector3f up = (N[eZ] < 0.999 ? Vector3f(0, 0, 1) : Vector3f(1, 0, 0));
    const Vector3f tangent_x = (up % N).normalized();
    const Vector3f tangent_y = N % tangent_x;

    return tangent_x * H[eX] + tangent_y * H[eY] + N * H[eZ];
}
