/**********************************************************************
File name: line.cpp
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
#include "ffengine/math/line.hpp"

#include "ffengine/math/intersect.hpp"


Line2f::Line2f(const Vector2f p0, const Vector2f v):
    homogeneous(-v[eY], v[eX], v[eY]*p0[eX]-v[eX]*p0[eY])
{

}

Vector2f Line2f::sample() const
{
    if (std::fabs(homogeneous[eY]) > std::fabs(homogeneous[eX])) {
        return Vector2f(0, -homogeneous[eZ]/homogeneous[eY]);
    } else {
        return Vector2f(-homogeneous[eZ]/homogeneous[eX], 0);
    }
}

std::pair<Vector2f, Vector2f> Line2f::point_and_direction() const
{
    return std::make_pair(sample(), Vector2f(homogeneous[eY],
                                             -homogeneous[eX]));
}


Vector2f isect_line_line(const Line2f l1, const Line2f l2)
{
    Vector3f homogeneous(l1.homogeneous % l2.homogeneous);
    if (std::fabs(homogeneous[eZ]) < ISECT_EPSILON) {
        return Vector2f(NAN, NAN);
    }

    return Vector2f(homogeneous[eX], homogeneous[eY]) / homogeneous[eZ];
}


std::ostream &operator<<(std::ostream &dest, const Line2f &l)
{
    auto line = l.point_and_direction();
    return dest << "Line2f(" << line.first << ", " << line.second << ")";
}
