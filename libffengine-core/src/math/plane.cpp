/**********************************************************************
File name: plane.cpp
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
#include "ffengine/math/plane.hpp"

#include "ffengine/math/algo.hpp"
#include "ffengine/math/intersect.hpp"


Plane::Plane(const float dist, const Vector3f &normal):
    Plane(Vector4f(normal.normalized(), dist * normal.length()))
{

}

Plane::Plane(const Vector3f &origin, const Vector3f &normal):
    Plane(Vector4f(normal.normalized(), normal.normalized()*origin))
{

}

Plane::Plane(const Vector4f &homogenous_vector):
    homogeneous(homogenous_vector)
{
    const float magnitude = std::sqrt(
                sqr(homogeneous.as_array[0])
            + sqr(homogeneous.as_array[1])
            + sqr(homogeneous.as_array[2]));
    homogeneous /= magnitude;
}

PlaneSide Plane::side_of(const Sphere &other) const
{
    const float normal_projected_center =
            Vector4f(other.center, -1) * homogeneous;
    if (std::fabs(normal_projected_center) <= other.radius) {
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
    const float ndist = Vector4f(other, -1.f) * homogeneous;
    if (std::fabs(ndist) < ISECT_EPSILON) {
        return PlaneSide::BOTH;
    } else if (ndist < 0) {
        return PlaneSide::NEGATIVE_NORMAL;
    } else {
        return PlaneSide::POSITIVE_NORMAL;
    }
}


std::ostream &operator<<(std::ostream &stream, const Plane &plane)
{
    return stream << "Plane(" << plane.homogeneous << ")";
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
