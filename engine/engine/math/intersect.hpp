#ifndef SCC_ENGINE_MATH_INTERSECT_H
#define SCC_ENGINE_MATH_INTERSECT_H

#include "engine/math/vector.hpp"
#include "engine/math/shapes.hpp"


std::tuple<float, bool> isect_ray_triangle(
        const Ray &ray,
        const Vector3f &p0,
        const Vector3f &p1,
        const Vector3f &p2);

std::tuple<float, bool> isect_plane_ray(
        const Vector3f &plane_origin,
        const Vector3f &plane_normal,
        const Ray &ray);

#endif
