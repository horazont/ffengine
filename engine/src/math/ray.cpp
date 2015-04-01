#include "engine/math/ray.hpp"

#include <iostream>

const float EPSILON = 0.00001;

std::tuple<float, bool> intersect_triangle(
        const Ray &ray,
        const Vector3f &p0,
        const Vector3f &p1,
        const Vector3f &p2)
{
    const Vector3f edge1 = p1 - p0;
    const Vector3f edge2 = p2 - p0;

    const Vector3f pvec = ray.direction % edge2;

    const float det = edge1 * pvec;

    if (det > -EPSILON && det < EPSILON) {
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

std::tuple<float, bool> intersect_plane(const Ray &ray,
                                        const Vector3f &plane_origin,
                                        const Vector3f &plane_normal)
{
    const float normal_dir = ray.direction * plane_normal;

    if (normal_dir > -EPSILON && normal_dir < EPSILON) {
        // parallel
        return std::make_tuple(NAN, false);
    }

    const float t = (plane_origin*plane_normal - plane_normal*ray.origin) / normal_dir;

    return std::make_tuple(t, t >= 0);
}
