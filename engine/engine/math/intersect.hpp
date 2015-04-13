#ifndef SCC_ENGINE_MATH_INTERSECT_H
#define SCC_ENGINE_MATH_INTERSECT_H

#include "engine/math/vector.hpp"
#include "engine/math/shapes.hpp"


std::tuple<float, bool> isect_ray_triangle(
        const Ray &ray,
        const Vector3f &p0,
        const Vector3f &p1,
        const Vector3f &p2);

std::tuple<float, PlaneSide> isect_plane_ray(
        const Plane &plane,
        const Ray &ray);

bool isect_aabb_sphere(
        const AABB &aabb,
        const Sphere &sphere);

extern const float ISECT_EPSILON;

#endif
