/**********************************************************************
File name: shapes.cpp
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
    Plane(Vector4f(normal.normalized(), dist * normal.length()))
{

}

Plane::Plane(const Vector3f &origin, const Vector3f &normal):
    Plane(Vector4f(normal.normalized(), normal.normalized()*origin))
{

}

Plane::Plane(const Vector4f &homogenous_vector):
    homogenous(homogenous_vector)
{
    const float magnitude = std::sqrt(
                sqr(homogenous.as_array[0])
            + sqr(homogenous.as_array[1])
            + sqr(homogenous.as_array[2]));
    homogenous[eX] /= magnitude;
    homogenous[eY] /= magnitude;
    homogenous[eZ] /= magnitude;
    homogenous[eW] *= magnitude;
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


std::ostream &operator<<(std::ostream &stream, const Plane &plane)
{
    return stream << "Plane(" << plane.homogenous << ")";
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
