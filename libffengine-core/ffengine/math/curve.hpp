/**********************************************************************
File name: curve.hpp
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
#ifndef SCC_ENGINE_MATH_CURVE_H
#define SCC_ENGINE_MATH_CURVE_H

#include <ostream>

#include "ffengine/math/algo.hpp"
#include "ffengine/math/vector.hpp"

template <typename vector_t>
struct QuadBezier
{
    QuadBezier() = default;

    template <typename p1_t, typename p2_t, typename p3_t>
    QuadBezier(p1_t &&p1, p2_t &&p2, p3_t &&p3):
        p1(p1),
        p2(p2),
        p3(p3)
    {

    }

    vector_t p1, p2, p3;

    // printing

    template <typename other_vector_t>
    inline bool operator==(const QuadBezier<other_vector_t> &other) const
    {
        return (p1 == other.p1) && (p2 == other.p2) && (p3 == other.p3);
    }

    template <typename other_vector_t>
    inline bool operator!=(const QuadBezier<other_vector_t> &other) const
    {
        return !(*this == other);
    }

    // evaluating

    template <typename float_t>
    inline Vector<float_t, vector_t::dimension> operator[](float_t t) const
    {
        return sqr(1-t)*p1 + 2*(1-t)*t*p2 + sqr(t)*p3;
    }


    template <typename float_t>
    inline Vector<float_t, vector_t::dimension> diff(float_t t) const
    {
        return 2*(1-t)*(p2 - p1) + 2*t*(p3 - p2);
    }

};

typedef QuadBezier<Vector3f> QuadBezier3f;
typedef QuadBezier<Vector3d> QuadBezier3d;

namespace std {

template <typename vector_t>
ostream &operator<<(ostream &stream, const QuadBezier<vector_t> &bezier)
{
    return stream << "bezier("
                  << bezier.p1 << ", "
                  << bezier.p2 << ", "
                  << bezier.p3 << ")";
}

}

#endif
