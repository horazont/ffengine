/**********************************************************************
File name: intersect.cpp
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
#include "ffengine/math/intersect.hpp"

#include <iostream>
#include <limits>
#include <tuple>


const float ISECT_EPSILON = 0.00001;

std::tuple<float, bool> isect_ray_triangle(
        const Ray &ray,
        const Vector3f &p0,
        const Vector3f &p1,
        const Vector3f &p2)
{
    const Vector3f edge1 = p1 - p0;
    const Vector3f edge2 = p2 - p0;

    const Vector3f pvec = ray.direction % edge2;

    const float det = edge1 * pvec;

    if (det > -ISECT_EPSILON && det < ISECT_EPSILON) {
        return std::make_tuple(NAN, false);
    }

    const float inv_det = 1/det;

    const Vector3f tvec = ray.origin - p0;

    const float u = (tvec * pvec) * inv_det;
    if (u < 0 || u > 1) {
        return std::make_tuple(NAN, false);
    }

    const Vector3f qvec = tvec % edge1;

    const float v = (ray.direction * qvec) * inv_det;
    if (v < 0 || v > 1) {
        return std::make_tuple(NAN, false);
    }

    const float t = (edge2 * qvec) * inv_det;

    return std::make_tuple(t, t >= 0);
}

std::tuple<float, PlaneSide> isect_plane_ray(
        const Plane &plane,
        const Ray &ray)
{
    const float normal_dir = ray.direction * Vector3f(
                plane.homogeneous.as_array[0],
                plane.homogeneous.as_array[1],
                plane.homogeneous.as_array[2]);

    if (normal_dir > -ISECT_EPSILON && normal_dir < ISECT_EPSILON) {
        // parallel
        return std::make_tuple(0, plane.side_of(ray.origin));
    }

    const float t = -(plane.homogeneous*Vector4f(ray.origin, -1)) / normal_dir;

    return std::make_tuple(t, PlaneSide::BOTH);
}

bool isect_aabb_sphere(const AABB &aabb, const Sphere &sphere)
{
    /* Jim Arvo, A Simple Method for Box-Sphere Intersection Testing, Graphics Gems, pp. 247-250 */

    float dmin = 0;
    for (unsigned int i = 0; i < 3; i++) {
        const float Ci = sphere.center.as_array[i];
        if (Ci < aabb.min.as_array[i]) {
            dmin += sqr(Ci - aabb.min.as_array[i]);
        } else if (Ci > aabb.max.as_array[i]) {
            dmin += sqr(Ci - aabb.max.as_array[i]);
        }
    }
    return dmin <= sqr(sphere.radius);
}

bool isect_aabb_ray(const AABB &aabb, const Ray &ray, float &t0, float &t1)
{
    std::array<Plane, 6> sides({{
                                   Plane(aabb.min, Vector3f(-1, 0, 0)),
                                   Plane(aabb.max, Vector3f(1, 0, 0)),
                                   Plane(aabb.min, Vector3f(0, -1, 0)),
                                   Plane(aabb.max, Vector3f(0, 1, 0)),
                                   Plane(aabb.min, Vector3f(0, 0, -1)),
                                   Plane(aabb.max, Vector3f(0, 0, 1))
                               }});

    float test_min, test_max;
    float tmin = std::numeric_limits<float>::min();
    float tmax = std::numeric_limits<float>::max();

    PlaneSide hit;
    std::tie(test_min, hit) = isect_plane_ray(sides[0], ray);
    if (hit == PlaneSide::POSITIVE_NORMAL) {
        return false;
    }
    std::tie(test_max, hit) = isect_plane_ray(sides[1], ray);
    if (hit == PlaneSide::POSITIVE_NORMAL) {
        return false;
    }
    if (hit == PlaneSide::BOTH && test_min != test_max) {
        tmin = std::max(std::min(test_min, test_max), tmin);
        tmax = std::min(std::max(test_max, test_min), tmax);
    }

    std::tie(test_min, hit) = isect_plane_ray(sides[2], ray);
    if (hit == PlaneSide::POSITIVE_NORMAL) {
        return false;
    }
    std::tie(test_max, hit) = isect_plane_ray(sides[3], ray);
    if (hit == PlaneSide::POSITIVE_NORMAL) {
        return false;
    }
    if (hit == PlaneSide::BOTH && test_min != test_max) {
        tmin = std::max(std::min(test_min, test_max), tmin);
        tmax = std::min(std::max(test_max, test_min), tmax);
    }

    std::tie(test_min, hit) = isect_plane_ray(sides[4], ray);
    if (hit == PlaneSide::POSITIVE_NORMAL) {
        return false;
    }
    std::tie(test_max, hit) = isect_plane_ray(sides[5], ray);
    if (hit == PlaneSide::POSITIVE_NORMAL) {
        return false;
    }
    if (hit == PlaneSide::BOTH && test_min != test_max) {
        tmin = std::max(std::min(test_min, test_max), tmin);
        tmax = std::min(std::max(test_max, test_min), tmax);
    }

    if (tmin > tmax) {
        return false;
    }

    t0 = tmin;
    t1 = tmax;

    return true;
}

PlaneSide isect_aabb_frustum(const AABB &aabb,
                             const std::array<Plane, 6> &frustum)
{
    PlaneSide result = PlaneSide::POSITIVE_NORMAL;
    for (unsigned int i = 0; i < 6; i++) {
        PlaneSide tmp = frustum[i].side_of_fast(aabb);
        /* std::cout << tmp << std::endl; */
        if (tmp == PlaneSide::NEGATIVE_NORMAL) {
            return tmp;
        }
        if (tmp == PlaneSide::BOTH) {
            result = tmp;
        }
    }
    return result;
}

bool isect_ray_sphere(const Ray &r, const Sphere &sphere,
                      float &t0, float &t1)
{
    const Vector3f local_sphere_center = sphere.center - r.origin;

    const float projected_center_distance = local_sphere_center * r.direction;

    if (projected_center_distance < -sphere.radius) {
        // sphere behind starting point
        return false;
    }

    const Vector3f projected_sphere_center = projected_center_distance * r.direction;

    const float dist = (projected_sphere_center - local_sphere_center).length();
    if (dist > sphere.radius) {
        return false;
    }

    const float dist_to_entry_point = std::sqrt(sqr(sphere.radius) - sqr(dist));

    if (local_sphere_center.length() < sphere.radius) {
        t0 = 0.f;
        t1 = projected_center_distance + dist_to_entry_point;
    } else {
        t0 = projected_center_distance - dist_to_entry_point;
        t1 = projected_center_distance + dist_to_entry_point;
    }

    return true;
}


bool isect_cylinder_ray(const Vector3f &start,
                        const Vector3f &direction,
                        const float radius,
                        const Ray &r, float &t1, float &t2)
{
    const Vector3f &AB = direction;
    const Vector3f AO = r.origin - start;
    const Vector3f AOxAB = AO % AB;
    const Vector3f VxAB = r.direction % AB;
    const Vector3f n_direction = direction.normalized();
    const float l_direction = direction.length();

    const float ab2 = AB * AB;
    const float a = VxAB * VxAB;
    const float b = 2 * (VxAB * AOxAB);
    const float c = (AOxAB * AOxAB) - (radius*radius * ab2);

    bool hit;
    float cylinder_t1, cylinder_t2;
    Vector3f entrypoint, exitpoint;
    if (std::fabs(a) <= ISECT_EPSILON &&
            std::fabs(b) <= ISECT_EPSILON &&
            std::fabs(c) > ISECT_EPSILON)
    {
        // we have to treat this case specially, it means we are not hitting
        // the outer hull of the cylinder.

        // if c < 0, the ray is inside the cylinder, otherwise, it is outside

        if (c < 0) {
            // order cylinder_t1 and cylinder_t2 correctly
            if (n_direction * r.direction < 0) {
                // we want clamping
                cylinder_t1 = INFINITY;
                cylinder_t2 = -INFINITY;
            } else {
                // we want clamping
                cylinder_t1 = -INFINITY;
                cylinder_t2 = INFINITY;
            }
            hit = true;
        } else {
            return false;
        }
    } else {
        hit = solve_quadratic(a, b, c, t1, t2);
        if (!hit) {
            return false;
        }

        if (t2 < 0) {
            return false;
        }

        // project entry and exit points on the cylinder axis
        entrypoint = r.origin + r.direction*t1;
        exitpoint = r.origin + r.direction*t2;

        cylinder_t1 = (entrypoint - start) * n_direction;
        cylinder_t2 = (exitpoint - start) * n_direction;

        if (cylinder_t1 > l_direction && cylinder_t2 > l_direction) {
            return false;
        }

        if (cylinder_t1 < 0 && cylinder_t2 < 0) {
            return false;
        }
    }

    if (cylinder_t1 > l_direction || cylinder_t1 < 0) {
        // cylinder_t1 is from inifinite cylinder, we have to clamp
        cylinder_t1 = clamp(cylinder_t1, 0.f, l_direction);
        entrypoint = start + cylinder_t1 * n_direction;
    }

    if (cylinder_t2 > l_direction || cylinder_t2 < 0) {
        // cylinder_t2 is from inifinite cylinder, we have to clamp
        cylinder_t2 = clamp(cylinder_t2, 0.f, l_direction);
        exitpoint = start + cylinder_t2 * n_direction;
    }

    t1 = (entrypoint - r.origin) * r.direction;
    t2 = (exitpoint - r.origin) * r.direction;

    if (t1 < 0) {
        t1 = 0;
    }
    return true;
}
