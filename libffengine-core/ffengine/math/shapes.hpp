/**********************************************************************
File name: shapes.hpp
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
#ifndef SCC_ENGINE_MATH_RAY_H
#define SCC_ENGINE_MATH_RAY_H

#include "ffengine/math/vector.hpp"


/**
 * An enum describing on which side of a plane an object is.
 */
enum class PlaneSide {
    POSITIVE_NORMAL, ///< The other object is on wholly the side to which the normal points
    BOTH,            ///< The object intersects the plane, is partially on one and the other side
    NEGATIVE_NORMAL  ///< The other object is on wholly the side from which the normal points away
};


/**
 * A ray (half-line originating at a specific point).
 */
struct Ray
{
    Ray();
    Ray(const Vector3f &origin, const Vector3f &direction);

    /**
     * The origin of the ray.
     */
    Vector3f origin;

    /**
     * The direction into which the ray points.
     */
    Vector3f direction;
};

/**
 * An ideal, mathematical sphere.
 */
struct Sphere
{
    /**
     * The center of the sphere.
     */
    Vector3f center;

    /**
     * The radius of the sphere.
     */
    float radius;
};


/**
 * Axis-aligned bounding box structure.
 */
template <typename float_t>
struct GenericAABB
{
    enum empty_t {
        Empty
    };

    GenericAABB():
        GenericAABB(Empty)
    {

    }

    GenericAABB(empty_t):
        min(Vector3f(1, 1, 1)),
        max(Vector3f(0, 0, 0))
    {

    }

    template <typename float_1_t, typename float_2_t>
    GenericAABB(const Vector<float_1_t, 3> &v1,
         const Vector<float_2_t, 3> &v2):
        min(v1),
        max(v2)
    {

    }

    template <typename other_float_t>
    GenericAABB(const GenericAABB<other_float_t> &ref):
        GenericAABB(ref.min, ref.max)
    {

    }

    template <typename other_float_t>
    GenericAABB &operator=(const GenericAABB<other_float_t> &ref)
    {
        min = ref.min;
        max = ref.max;
        return *this;
    }

    Vector<float_t, 3> min;
    Vector<float_t, 3> max;

    inline bool empty() const
    {
        return (max[eX] < min[eX] || max[eY] < min[eY] || max[eZ] < min[eZ]);
    }

    template <typename other_float_t>
    inline bool operator==(const GenericAABB<other_float_t> &other) const
    {
        if (empty() && other.empty()) {
            return true;
        }
        return (min == other.min) && (max == other.max);
    }

    template <typename other_float_t>
    inline bool operator!=(const GenericAABB<other_float_t> &other) const
    {
        return !((*this) == other);
    }

    inline void extend_to_cover(const GenericAABB &other)
    {
        if (empty()) {
            *this = other;
            return;
        }

        for (unsigned int i = 0; i < 3; ++i) {
            min.as_array[i] = std::min(min.as_array[i], other.min.as_array[i]);
        }
        for (unsigned int i = 0; i < 3; ++i) {
            max.as_array[i] = std::max(max.as_array[i], other.max.as_array[i]);
        }
    }

};

template <typename float_t>
static inline GenericAABB<float_t> bounds(const GenericAABB<float_t> &a,
                                          const GenericAABB<float_t> &b)
{
    GenericAABB<float_t> result(a);
    result.extend_to_cover(b);
    return result;
}

using AABB = GenericAABB<float>;


/**
 * A plane.
 */
struct Plane
{
    Plane() = default;

    /**
     * Construct a plane using an origin and a normal. The origin is converted
     * to a distance internally by calculating the dot-product with the normal.
     *
     * @param origin Origin point for the plane.
     * @param normal Normal of the plane surface.
     */
    Plane(const Vector3f &origin, const Vector3f &normal);
    Plane(const float dist, const Vector3f &normal);
    explicit Plane(const Vector4f &homogenous_vector);

    Vector4f homogenous;

    /**
     * Check on which side of the plane a Sphere is.
     *
     * @param other The Sphere to test
     * @return The side of the plane on which the Sphere is.
     */
    PlaneSide side_of(const Sphere &other) const;

    /**
     * Check on which side of the plane a Point is.
     *
     * @param other The Point to test
     * @return The side of the plane on which the Point is.
     */
    PlaneSide side_of(const Vector3f &other) const;

    /**
     * An alias of side_of() for the sphere.
     *
     * @see \link side_of(const Sphere&) const \endlink
     */
    inline PlaneSide side_of_fast(const Sphere &other) const
    {
        return side_of(other);
    }

    /**
     * Check on which side of a plane an AABB is. This is a fast test which
     * approximates the AABB as a sphere and uses
     * \link side_of(const Sphere&) const \endlink internally.
     *
     * As a result of the approximation, the test will return values of
     * PlaneSide::BOTH when no intersection happens and the value should in
     * fact be PlaneSide::POSITIVE_NORMAL or PlaneSide::NEGATIVE_NORMAL.
     *
     * @param other AABB to test
     * @return Side of the plane on which the AABB is.
     */
    inline PlaneSide side_of_fast(const AABB &other) const
    {
        const Vector3f center = (other.max+other.min) / 2;
        const float radius = (other.max - center).length();
        return side_of(Sphere{center, radius});
    }

    static Plane from_frustum_matrix(const Vector4f frustum_homogenous)
    {
        // we have to negate the W part, because given a test vector
        // vX, vY, vZ and the homogenous plane vector X, Y, Z, W, our
        // intersection is defined as:
        //
        //    X * vX + Y * vY + Z * vZ - W = 0
        //
        // whereas the vectors obtained from frustum matrices are for
        //
        //    X * vX + Y * vY + Z * vZ + W = 0
        return Plane(Vector4f(Vector3f(frustum_homogenous),
                              -frustum_homogenous[eW]));
    }

};

inline bool operator==(const Plane &a, const Plane b)
{
    return a.homogenous == b.homogenous;
}

inline bool operator!=(const Plane &a, const Plane b)
{
    return !(a == b);
}

std::ostream &operator<<(std::ostream &stream, const Plane &plane);
std::ostream &operator<<(std::ostream &stream, const PlaneSide side);

template <typename float_t>
static inline std::ostream &operator<<(std::ostream &stream, const GenericAABB<float_t> &aabb)
{
    return stream << "aabb(" << aabb.min << ", " << aabb.max << ")";
}

#endif
