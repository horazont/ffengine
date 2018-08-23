/**********************************************************************
File name: ubo_type_wrappers.inc.hpp
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
#ifndef SCC_ENGINE_GL_UBO_TYPE_WRAPPERS_H
#define SCC_ENGINE_GL_UBO_TYPE_WRAPPERS_H

template <typename unwrapped_type>
struct ubo_wrap_type;

/*{
    typedef unwrapped_type wrapped_type;

    static inline unwrapped_type extract(const wrapped_type &ref)
    {
        return ref;
    }

    static inline unwrapped_type &extract_ref(wrapped_type &ref)
    {
        return ref;
    }

    template <typename value_t>
    static inline wrapped_type pack(value_t &&ref)
    {
        return ref;
    }

};*/

template<>
struct ubo_wrap_type<float>
{
    typedef float type;
    struct wrapped_type
    {
        float value;
        float pad[3];
    };

    static constexpr unsigned nitems = 1;
    static constexpr GLenum gl_type = GL_FLOAT;

    static inline type unpack(const wrapped_type &from)
    {
        return from.value;
    }

    template <typename value_t>
    static inline wrapped_type pack(value_t &&ref)
    {
        return wrapped_type{ref, {0, 0, 0}};
    }

};

template<>
struct ubo_wrap_type<Vector2f>
{
    typedef Vector2f type;
    struct wrapped_type
    {
        Vector2f value;
        float pad[2];
    };
    static constexpr unsigned nitems = 1;

    static constexpr GLenum gl_type = GL_FLOAT_VEC2;

    static inline type unpack(const wrapped_type &from)
    {
        return from.value;
    }

    template <typename value_t>
    static inline wrapped_type pack(value_t &&ref)
    {
        return wrapped_type{ref, {0, 0}};
    }

};

template<>
struct ubo_wrap_type<Vector3f>
{
    typedef Vector3f type;
    struct wrapped_type
    {
        Vector3f value;
        float pad[1];
    };

    static constexpr unsigned nitems = 1;
    static constexpr GLenum gl_type = GL_FLOAT_VEC3;

    static inline type unpack(const wrapped_type &from)
    {
        return from.value;
    }

    template <typename value_t>
    static inline wrapped_type pack(value_t &&ref)
    {
        return wrapped_type{ref, {0}};
    }

};

template<>
struct ubo_wrap_type<Vector4f>
{
    typedef Vector4f type;
    typedef type wrapped_type;

    static constexpr unsigned nitems = 1;
    static constexpr GLenum gl_type = GL_FLOAT_VEC4;

    static inline type unpack(const wrapped_type &from)
    {
        return from;
    }

    template <typename value_t>
    static inline type pack(value_t &&ref)
    {
        return ref;
    }

};

template<>
struct ubo_wrap_type<Matrix4f>
{
    typedef Matrix4f type;
    typedef type wrapped_type;

    static constexpr unsigned nitems = 1;
    static constexpr GLenum gl_type = GL_FLOAT_MAT4;

    static inline type unpack(const wrapped_type &from)
    {
        return from;
    }

    static inline type &ref(wrapped_type &from)
    {
        return from;
    }

    template <typename value_t>
    static inline type pack(value_t &&ref)
    {
        return ref;
    }
};

template<>
struct ubo_wrap_type<Matrix3f>
{
    typedef Matrix3f type;
    typedef Matrix4f wrapped_type;

    static constexpr unsigned nitems = 1;
    static constexpr GLenum gl_type = GL_FLOAT_MAT3;

    static inline type unpack(const wrapped_type &from)
    {
        return type::clip(from);
    }

    template <typename value_t>
    static inline wrapped_type pack(value_t &&ref)
    {
        return wrapped_type::extend(ref);
    }

};

template<std::size_t N>
struct ubo_wrap_type<Vector4f[N]>
{
    typedef std::array<Vector4f, N> type;
    typedef type wrapped_type;

    static constexpr unsigned nitems = N;
    static constexpr GLenum gl_type = GL_FLOAT_VEC4;

    static inline type unpack(const wrapped_type &from)
    {
        return from;
    }

    static inline type pack(const Vector4f src[N])
    {
        type result;
        memcpy(&result[0], &src[0], sizeof(Vector4f) * N);
        return result;
    }

    /* template <typename value_t,
              typename _ = typename std::enable_if<!std::is_array<typename std::remove_cv<value_t>::type>::value>::type>
    static inline type pack(value_t &&ref)
    {
        return ref;
    } */

};

#endif // SCC_ENGINE_GL_UBO_TYPE_WRAPPERS_H

