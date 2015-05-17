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
#include "ffengine/math/shapes.hpp"

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
        const std::array<Plane, 4> &frustum);

/**
 * Epsilon to determine floating point equality in intersection algorithms.
 */
extern const float ISECT_EPSILON;

#endif
