/**********************************************************************
File name: line.hpp
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
#ifndef SCC_ENGINE_MATH_LINE_H
#define SCC_ENGINE_MATH_LINE_H

#include <ostream>

#include "ffengine/math/vector.hpp"

struct Line2f
{
    Line2f(const Vector2f p0, const Vector2f v);

    Vector3f homogeneous;

    /**
     * Return a point on the line.
     */
    Vector2f sample() const;

    /**
     * Return a point on the line and a direction from that point which together
     * can be used as arguments to the Line2f constructor to create an identical
     * line.
     */
    std::pair<Vector2f, Vector2f> point_and_direction() const;
};


Vector2f isect_line_line(const Line2f l1, const Line2f l2);

std::ostream &operator<<(std::ostream &dest, const Line2f &l);

#endif
