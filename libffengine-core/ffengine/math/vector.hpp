/**********************************************************************
File name: vector.hpp
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
#ifndef SCC_MATH_VECTORS_H
#define SCC_MATH_VECTORS_H

#include <cmath>
#include <ostream>
#include <vector>


enum vector_component_x_t {
    eX = 0
};

enum vector_component_y_t {
    eY = 1
};

enum vector_component_z_t {
    eZ = 2
};

enum vector_component_w_t {
    eW = 3
};

typedef double VectorFloat;

#include "variadic_initializer.inc.hpp"

template <typename _vector_float_t, unsigned int _dimension>
struct Vector {
    static constexpr unsigned int dimension = _dimension;
    using vector_float_t = _vector_float_t;

    static_assert(dimension > 0, "Vector dimension must be "
                                 "greater than 0");

    Vector():
        as_array()
    {
        for (unsigned int i = 0; i < dimension; i++) {
            as_array[i] = 0;
        }
    }

    template <typename... other_float_ts>
    explicit Vector(other_float_ts... values):
        as_array()
    {
        static_assert(sizeof...(values) == dimension,
                      "Must initialize vector with exactly N = dim "
                      "elements");
        init_array_with_data<vector_float_t, other_float_ts...>::init(
            &as_array[0], values...);
    }

    template <typename other_float_t, unsigned int other_dimension>
    explicit Vector(const Vector<other_float_t, other_dimension> &vec):
        as_array()
    {
        static_assert(other_dimension > dimension,
                      "Dimension of source vector must be larger than "
                      "dimension of destination vector.");
        for (unsigned int i = 0; i < dimension; i++) {
            as_array[i] = vec.as_array[i];
        }
    }

    template <typename other_float_t>
    Vector(const Vector<other_float_t, dimension> &ref):
        as_array()
    {
        for (unsigned int i = 0; i < dimension; i++) {
            as_array[i] = ref.as_array[i];
        }
    }

    template <typename other_vector_float_t,
              unsigned int other_dimension,
              typename... other_float_ts>
    explicit Vector(const Vector<other_vector_float_t, other_dimension> &ref,
                    other_float_ts... values):
        as_array()
    {
        static_assert(sizeof...(values) + other_dimension == dimension,
                      "Must initialize vector with exactly N = dim "
                      "elements");

        for (unsigned int i = 0; i < other_dimension; i++) {
            as_array[i] = ref.as_array[i];
        }

        init_array_with_data<vector_float_t, other_float_ts...>::init(
                    &as_array[other_dimension], values...);
    }

    vector_float_t as_array[dimension];

    template <
        typename other_float_t,
        typename out_float_t = decltype(as_array[0] * other_float_t(0))>
    inline out_float_t operator*(
        const Vector<other_float_t, dimension> &oth) const
    {
        out_float_t dotp = 0;
        for (unsigned int i = 0; i < dimension; i++) {
            dotp += as_array[i] * oth.as_array[i];
        }
        return dotp;
    }

    template <typename other_float_t>
    inline Vector& operator+=(
        const Vector<other_float_t, dimension> &oth)
    {
        for (unsigned int i = 0; i < dimension; i++) {
            as_array[i] += oth.as_array[i];
        }
        return *this;
    }

    template <
        typename other_float_t,
        typename out_float_t = decltype(as_array[0] + other_float_t(0))>
    inline Vector<out_float_t, dimension> operator+(
        const Vector<other_float_t, dimension> &oth) const
    {
        Vector<out_float_t, dimension> result(*this);
        result += oth;
        return result;
    }

    template <typename other_float_t>
    inline Vector& operator-=(
        const Vector<other_float_t, dimension> &oth)
    {
        for (unsigned int i = 0; i < dimension; i++) {
            as_array[i] -= oth.as_array[i];
        }
        return *this;
    }

    template <
        typename other_float_t,
        typename out_float_t = decltype(as_array[0] - other_float_t(0))>
    inline Vector<out_float_t, dimension> operator-(
        const Vector<other_float_t, dimension> &oth) const
    {
        Vector<out_float_t, dimension> result(*this);
        result -= oth;
        return result;
    }

    inline Vector operator-() const
    {
        Vector result(*this);
        for (unsigned int i = 0; i < dimension; i++) {
            result.as_array[i] = -result.as_array[i];
        }
        return result;
    }

    inline Vector operator+() const
    {
        return Vector(*this);
    }

    template <typename other_float_t>
    inline Vector& operator/=(const other_float_t &oth)
    {
        for (unsigned int i = 0; i < dimension; i++) {
            as_array[i] /= oth;
        }
        return *this;
    }

    template <typename other_float_t>
    inline Vector operator/(const other_float_t &oth) const
    {
        Vector result(*this);
        result /= oth;
        return result;
    }

    template <typename other_float_t>
    inline Vector& operator*=(const other_float_t &oth)
    {
        for (unsigned int i = 0; i < dimension; i++) {
            as_array[i] *= oth;
        }
        return *this;
    }

    template <typename other_float_t>
    inline Vector operator*(const other_float_t &oth) const
    {
        Vector result(*this);
        result *= oth;
        return result;
    }

    inline vector_float_t length() const
    {
        return sqrt((*this) * (*this));
    }

    inline Vector& normalize()
    {
        vector_float_t l = length();
        if (l == 0) {
            return *this;
        }
        *this /= l;
        return *this;
    }

    inline Vector normalized() const
    {
        Vector result(*this);
        result.normalize();
        return result;
    }

    template <typename other_float_t>
    inline bool operator==(const Vector<other_float_t, dimension> &oth) const
    {
        for (unsigned int i = 0; i < dimension; i++) {
            if (as_array[i] != oth.as_array[i]) {
                return false;
            }
        }
        return true;
    }

    template <typename other_float_t>
    inline bool operator!=(const Vector<other_float_t, dimension> &oth) const
    {
        return !(*this == oth);
    }

    inline vector_float_t& operator[](unsigned int i)
    {
        return as_array[i];
    }

    inline vector_float_t operator[](unsigned int i) const
    {
        return as_array[i];
    }

    inline vector_float_t& operator[](vector_component_x_t)
    {
        return as_array[0];
    }

    inline vector_float_t& operator[](vector_component_y_t)
    {
        static_assert(dimension > 1, "Attempt to access Y component in "
                                     "Vector with dim <= 1");
        return as_array[1];
    }

    inline vector_float_t& operator[](vector_component_z_t)
    {
        static_assert(dimension > 2, "Attempt to access Z component in "
                                     "Vector with dim <= 2");
        return as_array[2];
    }

    inline vector_float_t& operator[](vector_component_w_t)
    {
        static_assert(dimension > 3, "Attempt to access W component in "
                                     "Vector with dim <= 3");
        return as_array[3];
    }

    inline vector_float_t operator*(vector_component_x_t) const
    {
        return (*this)[eX];
    }

    inline vector_float_t operator*(vector_component_y_t) const
    {
        return (*this)[eY];
    }

    inline vector_float_t operator*(vector_component_z_t) const
    {
        return (*this)[eZ];
    }

    inline vector_float_t operator*(vector_component_w_t) const
    {
        return (*this)[eW];
    }

    vector_float_t abssum() const
    {
        vector_float_t summed = 0;
        for (unsigned int i = 0; i < dimension; i++) {
            summed += std::abs(as_array[i]);
        }
        return summed;
    }

};

template <typename vector_float_t1, typename vector_float_t2,
          typename out_float_t =
            decltype(vector_float_t1(0) * vector_float_t2(0))>
inline Vector<out_float_t, 3> operator%(
    const Vector<vector_float_t1, 3> &a,
    const Vector<vector_float_t2, 3> &b)
{
    return Vector<out_float_t, 3>(
        a[eY] * b[eZ] - a[eZ] * b[eY],
        a[eZ] * b[eX] - a[eX] * b[eZ],
        a[eX] * b[eY] - a[eY] * b[eX]
    );
}

template <
    typename float_t,
    typename vector_float_t,
    unsigned int dimension,
    typename out_float_t = decltype(float_t(0) * vector_float_t(0))>
inline Vector<vector_float_t, dimension> operator*(
    const float_t scale,
    const Vector<vector_float_t, dimension> &vec)
{
    return vec * scale;
}

typedef Vector<float, 2> Vector2f;
typedef Vector<float, 3> Vector3f;
typedef Vector<float, 4> Vector4f;

typedef Vector<double, 2> Vector2d;
typedef Vector<double, 3> Vector3d;
typedef Vector<double, 4> Vector4d;

typedef Vector<VectorFloat, 2> Vector2;
typedef Vector<VectorFloat, 3> Vector3;
typedef Vector<VectorFloat, 4> Vector4;

namespace {

template <typename vector_float_t, unsigned int dimension>
inline void vector_data_to_stream(
    std::ostream &ostream,
    const Vector<vector_float_t, dimension> &vec)
{
    ostream << "(" << vec[0];
    for (unsigned int i = 1; i < dimension; i++) {
        ostream << ", " << vec[i];
    }
    ostream << ")";
}

template <typename vector_float_t, unsigned int dimension>
struct vector_to_stream
{
    static inline std::ostream &put(
            std::ostream &ostream,
            const Vector<vector_float_t, dimension> &vec)
    {
        ostream << "vec_any" << dimension;
        vector_data_to_stream(ostream, vec);
        return ostream;
    }
};

template <unsigned int dimension>
struct vector_to_stream<float, dimension>
{
    static inline std::ostream& put(
        std::ostream &ostream,
        const Vector<float, dimension> &vec)
    {
        ostream << "vec" << dimension << "f";
        vector_data_to_stream(ostream, vec);
        return ostream;
    }
};

template <unsigned int dimension>
struct vector_to_stream<double, dimension>
{
    static inline std::ostream& put(
        std::ostream &ostream,
        const Vector<double, dimension> &vec)
    {
        ostream << "vec" << dimension << "d";
        vector_data_to_stream(ostream, vec);
        return ostream;
    }
};

}

namespace std {

template <typename vector_float_t, unsigned int dimension>
ostream& operator<<(
    ostream &ostream,
    const Vector<vector_float_t, dimension> &vec)
{
    return vector_to_stream<vector_float_t, dimension>::put(
        ostream, vec);
}

template <typename float_t, unsigned int ndim>
struct hash<Vector<float_t, ndim> >
{
    typedef std::size_t result_type;
    typedef Vector<float_t, ndim> argument_type;

    hash():
        m_float_hash()
    {

    }

private:
    std::hash<float_t> m_float_hash;

public:
    result_type operator()(const argument_type &v) const
    {
        result_type result = 0;
        for (unsigned int i = 0; i < ndim; i++) {
            result ^= m_float_hash(v.as_array[i]);
        }
        return result;
    }

};

}


#endif
