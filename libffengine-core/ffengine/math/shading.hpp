#ifndef SCC_ENGINE_MATH_SHADING_H
#define SCC_ENGINE_MATH_SHADING_H

#include "ffengine/math/vector.hpp"

/**
 * From "Real Shading in Unreal Engine 4"
 */
Vector3f importance_sample_ggx(const Vector2f Xi,
                               const float roughness,
                               const Vector3f N);

#endif
