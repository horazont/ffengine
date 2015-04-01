#ifndef SCC_ENGINE_MATH_RAY_H
#define SCC_ENGINE_MATH_RAY_H

#include <tuple>

#include "engine/math/vector.hpp"

struct Ray
{
    Vector3f origin;
    Vector3f direction;
};


std::tuple<float, bool> intersect_triangle(
        const Ray &ray,
        const Vector3f &p0,
        const Vector3f &p1,
        const Vector3f &p2);

#endif
