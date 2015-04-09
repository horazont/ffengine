#include "engine/math/shapes.hpp"


Plane::Plane(const float dist, const Vector3f &normal):
    dist(dist),
    normal(normal)
{

}

Plane::Plane(const Vector3f &origin, const Vector3f &normal):
    dist(origin*normal),
    normal(normal)
{

}

PlaneSide Plane::side_of(const Sphere &other) const
{
    const float normal_projected_center = other.center*normal;
    if (abs(normal_projected_center - dist) <= other.radius) {
        return PlaneSide::BOTH;
    } else {
        if (normal_projected_center > 0) {
            return PlaneSide::POSITIVE_NORMAL;
        } else {
            return PlaneSide::NEGATIVE_NORMAL;
        }
    }
}
