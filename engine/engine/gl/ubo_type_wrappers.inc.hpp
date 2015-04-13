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

    static constexpr GLenum gl_type = GL_FLOAT;

    static inline type unpack(const wrapped_type &from)
    {
        return from.value;
    }

    template <typename value_t>
    static inline wrapped_type pack(value_t &&ref)
    {
        return wrapped_type{ref};
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

    static constexpr GLenum gl_type = GL_FLOAT_VEC2;

    static inline type unpack(const wrapped_type &from)
    {
        return from.value;
    }

    template <typename value_t>
    static inline wrapped_type pack(value_t &&ref)
    {
        return wrapped_type{ref};
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

    static constexpr GLenum gl_type = GL_FLOAT_VEC3;

    static inline type unpack(const wrapped_type &from)
    {
        return from.value;
    }

    template <typename value_t>
    static inline wrapped_type pack(value_t &&ref)
    {
        return wrapped_type{ref};
    }

};

template<>
struct ubo_wrap_type<Vector4f>
{
    typedef Vector4f type;
    typedef type wrapped_type;

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

#endif // SCC_ENGINE_GL_UBO_TYPE_WRAPPERS_H

