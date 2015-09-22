/**********************************************************************
File name: aabb.hpp
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
#ifndef SCC_ENGINE_MATH_AABB_H
#define SCC_ENGINE_MATH_AABB_H

#include "ffengine/math/vector.hpp"


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

template <typename float_t>
static inline std::ostream &operator<<(std::ostream &stream, const GenericAABB<float_t> &aabb)
{
    return stream << "aabb(" << aabb.min << ", " << aabb.max << ")";
}

#endif
