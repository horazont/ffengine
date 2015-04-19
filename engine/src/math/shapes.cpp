#include "engine/math/shapes.hpp"

#include "engine/math/algo.hpp"
#include "engine/math/intersect.hpp"


Ray::Ray():
    origin(),
    direction()
{

}

Ray::Ray(const Vector3f &origin, const Vector3f &direction):
    origin(origin),
    direction(direction.normalized())
{

}


Plane::Plane(const float dist, const Vector3f &normal):
    homogenous(normal / normal.length(), dist / normal.length())
{

}

Plane::Plane(const Vector3f &origin, const Vector3f &normal):
    homogenous()
{
    Vector3f normalized_normal = normal.normalized();
    homogenous = Vector4f(normalized_normal, normalized_normal*origin);
}

Plane::Plane(const Vector4f &homogenous_vector):
    homogenous(homogenous_vector)
{
    const float magnitude = std::sqrt(
                sqr(homogenous.as_array[0])
            + sqr(homogenous.as_array[1])
            + sqr(homogenous.as_array[2]));
    homogenous /= magnitude;
}

PlaneSide Plane::side_of(const Sphere &other) const
{
    const float normal_projected_center =
            Vector4f(other.center, -1) * homogenous;
    if (fabs(normal_projected_center) <= other.radius) {
        return PlaneSide::BOTH;
    } else {
        if (normal_projected_center > 0) {
            return PlaneSide::POSITIVE_NORMAL;
        } else {
            return PlaneSide::NEGATIVE_NORMAL;
        }
    }
}

PlaneSide Plane::side_of(const Vector3f &other) const
{
    const float ndist = Vector4f(other, -1.f) * homogenous;
    if (fabs(ndist) < ISECT_EPSILON) {
        return PlaneSide::BOTH;
    } else if (ndist < 0) {
        return PlaneSide::NEGATIVE_NORMAL;
    } else {
        return PlaneSide::POSITIVE_NORMAL;
    }
}


std::ostream &operator<<(std::ostream &stream, const PlaneSide side)
{
    switch (side)
    {
    case PlaneSide::BOTH:
    {
        return stream << "PlaneSide::BOTH";
    }
    case PlaneSide::NEGATIVE_NORMAL:
    {
        return stream << "PlaneSide::NEGATIVE_NORMAL";
    }
    case PlaneSide::POSITIVE_NORMAL:
    {
        return stream << "PlaneSide::POSITIVE_NORMAL";
    }
    }
    return stream << "PlaneSide(" << static_cast<long int>(side) << ")";
}
