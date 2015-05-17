/**********************************************************************
File name: quaternion.hpp
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
#ifndef SCC_ENGINE_MATH_QUATERNION_H
#define SCC_ENGINE_MATH_QUATERNION_H

#include "ffengine/math/vector.hpp"
#include "ffengine/math/algo.hpp"

#include <type_traits>


template <typename float_t>
struct Quaternion
{
    static_assert(std::is_arithmetic<float_t>::value,
                  "Quaternion base type must be arithmetic");

    Quaternion(const float_t real,
               const float_t imag_i,
               const float_t imag_j,
               const float_t imag_k):
        as_array{real, imag_i, imag_j, imag_k}
    {

    }

    /*explicit Quaternion(const float_t real):
        as_array{real, 0., 0., 0.}
    {

    }*/

    template <typename other_float_t>
    explicit Quaternion(const Vector<other_float_t, 3> &vector):
        Quaternion(0, vector[eX], vector[eY], vector[eZ])
    {

    }

    template <typename other_float_t>
    Quaternion(const Quaternion<other_float_t> &ref):
        Quaternion(ref.as_array[0], ref.as_array[1], ref.as_array[2], ref.as_array[3])
    {

    }

    float_t as_array[4];

    template <typename other_float_t,
              typename out_float_t = decltype(float_t(0)*other_float_t(0))>
    inline Quaternion<out_float_t> operator*(const Quaternion<other_float_t> &other) const
    {
        const out_float_t result_real =
                as_array[0]*other.as_array[0] -
                as_array[1]*other.as_array[1] -
                as_array[2]*other.as_array[2] -
                as_array[3]*other.as_array[3];

        const out_float_t result_imag_i =
                as_array[0]*other.as_array[1] +
                as_array[1]*other.as_array[0] +
                as_array[2]*other.as_array[3] -
                as_array[3]*other.as_array[2];

        const out_float_t result_imag_j =
                as_array[0]*other.as_array[2] -
                as_array[1]*other.as_array[3] +
                as_array[2]*other.as_array[0] +
                as_array[3]*other.as_array[1];

        const out_float_t result_imag_k =
                as_array[0]*other.as_array[3] +
                as_array[1]*other.as_array[2] -
                as_array[2]*other.as_array[1] +
                as_array[3]*other.as_array[0];

        return Quaternion<out_float_t>(result_real, result_imag_i,
                                       result_imag_j, result_imag_k);
    }

    template <typename other_float_t>
    inline Quaternion &operator-=(const Quaternion<other_float_t> &other)
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            as_array[i] -= other.as_array[i];
        }
        return *this;
    }

    template <typename other_float_t>
    inline Quaternion &operator+=(const Quaternion<other_float_t> &other)
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            as_array[i] += other.as_array[i];
        }
        return *this;
    }

    template <typename other_float_t>
    inline Quaternion &operator*=(const other_float_t scale)
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            as_array[i] *= scale;
        }
        return *this;
    }

    template <typename other_float_t>
    inline Quaternion &operator/=(const other_float_t scale)
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            as_array[i] /= scale;
        }
        return *this;
    }

    inline float_t abssum() const
    {
        float_t result = 0;
        for (unsigned int i = 0; i < 4; ++i)
        {
            result += std::abs(as_array[i]);
        }
        return result;
    }

    inline Quaternion &conjugate()
    {
        for (unsigned int i = 1; i < 4; ++i)
        {
            as_array[i] = -as_array[i];
        }
        return *this;
    }

    inline Quaternion conjugated() const
    {
        Quaternion result(*this);
        result.conjugate();
        return result;
    }

    inline Vector<float_t, 3> vector() const
    {
        return Vector<float_t, 3>(as_array[1], as_array[2], as_array[3]);
    }

    inline float_t real() const
    {
        return as_array[0];
    }

    inline float_t norm() const
    {
        float_t norm = 0;
        for (unsigned int i = 0; i < 4; ++i)
        {
            norm += sqr(as_array[i]);
        }
        return std::sqrt(norm);
    }

    inline Quaternion &normalize()
    {
        *this /= norm();
    }

    inline Quaternion normalized() const
    {
        Quaternion result(*this);
        result /= norm();
        return result;
    }

    template <typename vector_float_t,
              typename out_float_t = decltype(vector_float_t(0)*float_t(0))>
    inline Vector<out_float_t, 3> rotate(const Vector<vector_float_t, 3> &v) const
    {
        return ((*this) * Quaternion<vector_float_t>(v) * conjugated()).vector();
    }


    template <typename axis_float_t>
    static inline Quaternion rot(const float_t angle,
                                 const Vector<axis_float_t, 3> &axis)
    {
        return Quaternion(std::cos(angle/2.),
                          axis[eX]*std::sin(angle/2.),
                          axis[eY]*std::sin(angle/2.),
                          axis[eZ]*std::sin(angle/2.));
    }
};

template <typename floata_t, typename floatb_t>
static inline bool operator==(const Quaternion<floata_t> &a,
                              const Quaternion<floatb_t> &b)
{
    for (unsigned int i = 0; i < 4; ++i)
    {
        if (a.as_array[i] != b.as_array[i]) {
            return false;
        }
    }
    return true;
}

template <typename floata_t, typename floatb_t>
static inline bool operator!=(const Quaternion<floata_t> &a,
                              const Quaternion<floatb_t> &b)
{
    return !(a == b);
}

template <typename floata_t, typename floatb_t,
          typename float_out_t = decltype(floata_t(0)*floatb_t(0))>
static inline Quaternion<float_out_t> operator-(const Quaternion<floata_t> &a,
                                                const Quaternion<floatb_t> &b)
{
    Quaternion<float_out_t> result(a);
    result -= b;
    return result;
}

template <typename floata_t, typename floatb_t,
          typename float_out_t = decltype(floata_t(0)*floatb_t(0))>
static inline Quaternion<float_out_t> operator+(const Quaternion<floata_t> &a,
                                                const Quaternion<floatb_t> &b)
{
    Quaternion<float_out_t> result(a);
    result += b;
    return result;
}

template <typename floata_t, typename floatb_t,
          typename float_out_t = decltype(floata_t(0)*floatb_t(0))>
static inline Quaternion<float_out_t> operator*(const Quaternion<floata_t> &q,
                                                const floatb_t scale)
{
    static_assert(std::is_arithmetic<floatb_t>::value,
                  "Quaternion scale must be arithmetic");
    Quaternion<float_out_t> result(q);
    result *= scale;
    return result;
}

template <typename floata_t, typename floatb_t,
          typename float_out_t = decltype(floata_t(0)*floatb_t(0))>
static inline Quaternion<float_out_t> operator*(const floata_t scale,
                                                const Quaternion<floatb_t> &q)
{
    static_assert(std::is_arithmetic<floata_t>::value,
                  "Quaternion scale must be arithmetic");
    Quaternion<float_out_t> result(q);
    result *= scale;
    return result;
}

template <typename floata_t, typename floatb_t,
          typename float_out_t = decltype(floata_t(0)*floatb_t(0))>
static inline Quaternion<float_out_t> operator/(const Quaternion<floata_t> &q,
                                                const floatb_t scale)
{
    static_assert(std::is_arithmetic<floatb_t>::value,
                  "Quaternion scale must be arithmetic");
    Quaternion<float_out_t> result(q);
    result /= scale;
    return result;
}

template <typename float_t>
static inline Quaternion<float_t> operator-(const Quaternion<float_t> &q)
{
    return Quaternion<float_t>(
                -q.as_array[0],
                -q.as_array[1],
                -q.as_array[2],
                -q.as_array[3]);
}

using Quaternionf = Quaternion<float>;
using Quaterniond = Quaternion<double>;

template <typename float_t>
static inline std::ostream &operator<<(std::ostream &dest, const Quaternion<float_t> &q)
{
    return dest << "quat" << "("
                << q.as_array[0]
                << ", "
                << q.as_array[1]
                << ", "
                << q.as_array[2]
                << ", "
                << q.as_array[3]
                << ")";
}

#endif
