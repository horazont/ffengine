/**********************************************************************
File name: intersect.hpp
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
#ifndef SCC_ENGINE_MATH_INTERSECT_H
#define SCC_ENGINE_MATH_INTERSECT_H

#include <array>

#include "ffengine/math/vector.hpp"
#include "ffengine/math/ray.hpp"
#include "ffengine/math/aabb.hpp"
#include "ffengine/math/plane.hpp"
#include "ffengine/math/sphere.hpp"
#include "ffengine/math/algo.hpp"

/**
 * Epsilon to determine floating point equality in intersection algorithms.
 */
extern const float ISECT_EPSILON;

template <typename float_t>
static inline bool solve_linear(const float_t m, const float_t n,
                                float_t &t1)
{
    if (std::fabs(m) <= ISECT_EPSILON) {
        t1 = 0;
        return std::fabs(n) <= ISECT_EPSILON;
    }

    t1 = -n/m;
    return true;
}

template <typename float_t>
static inline bool solve_quadratic(const float_t a, const float_t b, const float_t c,
                                   float_t &t1, float_t &t2)
{
    if (std::fabs(a) <= ISECT_EPSILON) {
        // degrade to linear equation
        bool hit = solve_linear(b, c, t1);
        t2 = t1;
        return hit;
    }

    const float_t beta = b/float_t(2);
    const float_t beta_sq = sqr(beta);

    const float_t radicand = beta_sq - a*c;
    if (radicand < 0) {
        return false;
    }

    const float_t rooted = std::sqrt(radicand) / a;
    if (std::fabs(rooted) <= ISECT_EPSILON) {
        t1 = -beta / a;
        t2 = t1;
        return true;
    }

    const float_t minus_beta_over_a = -beta / a;

    t1 = minus_beta_over_a - rooted;
    t2 = minus_beta_over_a + rooted;
    return true;
}

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

/**
 * Calculate the intersection point between a Plane and a Ray, if any.
 *
 * @return a tuple consisting of the time point at which the ray intersects
 * the plane and a PlaneSide value indicating the state of the intersection.
 *
 * If the intersection fails, PlaneSide is not equal to PlaneSide::BOTH and
 * indicates on which side of the plane the origin of the ray resides.
 */
std::tuple<float, PlaneSide> isect_plane_ray(
        const Plane &plane,
        const Ray &ray);

/**
 * Check whether a Sphere and an AABB intersects.
 *
 * @return true on intersection, false otherwise.
 */
bool isect_aabb_sphere(
        const AABB &aabb,
        const Sphere &sphere);

/**
 * Calculate the intersection points between a Ray and an AABB.
 *
 * @param t0 Reference to store the first intersection time.
 * @param t1 Reference to store the second intersection time.
 * @return true if the Ray intersects the AABB, false otherwise.
 */
bool isect_aabb_ray(
        const AABB &aabb,
        const Ray &ray,
        float &t0,
        float &t1);

/**
 * Calculate on which side of the frustum a AABB resides.
 *
 * @return If the AABB is wholly inside the frustum, PlaneSide::POSITIVE_NORMAL
 * is returned. If AABB is wholly outside the frustum,
 * PlaneSide::NEGATIVE_NORMAL is returned. If the AABB intersects at least one
 * plane, PlaneSide::BOTH is returned.
 */
PlaneSide isect_aabb_frustum(
        const AABB &aabb,
        const std::array<Plane, 6> &frustum);

bool isect_ray_sphere(
        const Ray &r,
        const Sphere &sphere,
        float &t0,
        float &t1);

bool isect_cylinder_ray(
        const Vector3f &start,
        const Vector3f &direction,
        const float radius,
        const Ray &r,
        float &t1,
        float &t2);

#endif
