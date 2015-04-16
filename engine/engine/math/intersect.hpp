#ifndef SCC_ENGINE_MATH_INTERSECT_H
#define SCC_ENGINE_MATH_INTERSECT_H

#include "engine/math/vector.hpp"
#include "engine/math/shapes.hpp"

/**
 * Calculate the intersection point between a Ray and a triangle, the latter
 * given by three Vector3f.
 *
 * The implementation is a verbatim port of the implementation from
 * ["Fast, Minimum Storage Ray/Triangle Intersection" by Möller and Trumbore](http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf),
 * using the two-sided triangle version.
 *
 * @param ray Ray to intersect the triangle with
 * @param p0 First point of the triangle
 * @param p1 Second point of the triangle
 * @param p2 Third point of the triangle
 * @return Tuple consisting of the "time" \a t at which the ray hits the
 * triangle and a boolean whether the ray hit the triangle.
 */
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

bool isect_aabb_ray(
        const AABB &aabb,
        const Ray &ray,
        float &t0,
        float &t1);

extern const float ISECT_EPSILON;

#endif
