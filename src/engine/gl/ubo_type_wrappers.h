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
    struct wrapped_type
    {
        float value;
        float pad[3];
    };

    static constexpr GLenum gl_type = GL_FLOAT;

    static inline float extract(const wrapped_type &ref)
    {
        return ref.value;
    }

    static inline float &extract_ref(wrapped_type &ref)
    {
        return ref.value;
    }

    static inline const float &extract_ref(const wrapped_type &ref)
    {
        return ref.value;
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
    struct wrapped_type
    {
        Vector2f value;
        float pad[2];
    };

    static constexpr GLenum gl_type = GL_FLOAT_VEC2;

    static inline Vector2f extract(const wrapped_type &ref)
    {
        return ref.value;
    }

    static inline Vector2f &extract_ref(wrapped_type &ref)
    {
        return ref.value;
    }

    static inline const Vector2f &extract_ref(const wrapped_type &ref)
    {
        return ref.value;
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
    struct wrapped_type
    {
        Vector3f value;
        float pad[1];
    };

    static constexpr GLenum gl_type = GL_FLOAT_VEC3;

    static inline Vector3f extract(const wrapped_type &ref)
    {
        return ref.value;
    }

    static inline Vector3f &extract_ref(wrapped_type &ref)
    {
        return ref.value;
    }

    static inline const Vector3f &extract_ref(const wrapped_type &ref)
    {
        return ref.value;
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

    static inline type extract(const type &ref)
    {
        return ref;
    }

    static inline type &extract_ref(type &ref)
    {
        return ref;
    }

    static inline const type &extract_const_ref(const type &ref)
    {
        return ref;
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

    static inline type extract(const type &ref)
    {
        return ref;
    }

    static inline type &extract_ref(type &ref)
    {
        return ref;
    }

    static inline const type &extract_const_ref(const type &ref)
    {
        return ref;
    }

    template <typename value_t>
    static inline type pack(value_t &&ref)
    {
        return ref;
    }
};

#endif // SCC_ENGINE_GL_UBO_TYPE_WRAPPERS_H

