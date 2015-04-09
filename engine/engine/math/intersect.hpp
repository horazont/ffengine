#ifndef SCC_ENGINE_MATH_INTERSECT_H
#define SCC_ENGINE_MATH_INTERSECT_H

#include "engine/math/vector.hpp"
#include "engine/math/shapes.hpp"


enum class PlaneSide {
    POSITIVE_NORMAL,
    BOTH,
    NEGATIVE_NORMAL
};


std::tuple<float, bool> isect_ray_triangle(
        const Ray &ray,
        const Vector3f &p0,
        const Vector3f &p1,
        const Vector3f &p2);

std::tuple<float, bool> isect_plane_ray(
        const Vector3f &plane_origin,
        const Vector3f &plane_normal,
        const Ray &ray);

/**
 * Check on which side of a plane an AABB is. This is a fast test which
 * approximates the AABB as a sphere.
 *
 * @param min Minimum point of the AABB
 * @param max Maximum point of the AABB
 * @param plane_origin Origin of the plane
 * @param plane_normal Normal of the plane
 * @return Side of the plane on which the AABB is.
 */
PlaneSide planeside_aabb_fast(
        const Vector3f &plane_origin,
        const Vector3f &plane_normal,
        const Vector3f &min,
        const Vector3f &max);

PlaneSide planeside_sphere(
        const Vector3f &plane_origin,
        const Vector3f &plane_normal,
        const Vector3f &center,
        const float radius);

#endif
