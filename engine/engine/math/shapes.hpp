#ifndef SCC_ENGINE_MATH_RAY_H
#define SCC_ENGINE_MATH_RAY_H

#include "engine/math/vector.hpp"


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
struct AABB
{
    /**
     * The minimum point of the AABB.
     */
    Vector3f min;

    /**
     * The maximum point of the AABB.
     */
    Vector3f max;
};

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

    /**
     * The distance of the plane from the origin, in \link normal \endlink
     * direction.
     */
    float dist;

    /**
     * The normal of the plane.
     */
    Vector3f normal;

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

};

#endif
