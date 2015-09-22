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


Vector2f isect_line_line(const Line2f l1, const Line2f l2)
{
    Vector3f homogeneous(l1.homogeneous % l2.homogeneous);
    if (std::fabs(homogeneous[eZ]) < ISECT_EPSILON) {
        return Vector2f(NAN, NAN);
    }

    return Vector2f(homogeneous[eX], homogeneous[eY]) / homogeneous[eZ];
}
