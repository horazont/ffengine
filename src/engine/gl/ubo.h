#ifndef SCC_ENGINE_GL_UBO_H
#define SCC_ENGINE_GL_UBO_H

#include <GL/glew.h>

#include <tuple>
#include <utility>

#include "object.h"

#include "../../math/vector.h"
#include "../../math/matrix.h"

namespace engine {

#include "ubo_type_wrappers.h"

namespace ubo_storage_utils {

template <typename... element_ts>
struct wrapped_tuple;

template <typename element_t, typename... element_ts>
struct wrapped_tuple<element_t, element_ts...>
{
public:
    typedef wrapped_tuple<element_ts...> next_tuple;
    static constexpr std::size_t nelements = next_tuple::nelements + 1;

public:
    wrapped_tuple():
        m_data(),
        m_next()
    {

    }

    wrapped_tuple(const element_t &v, const element_ts&... vs):
        m_data(ubo_wrap_type<element_t>::pack(v)),
        m_next(vs...)
    {

    }

public:
    typename ubo_wrap_type<element_t>::wrapped_type m_data;
    next_tuple m_next;

};

template<>
struct wrapped_tuple<>
{
public:
    static constexpr std::size_t nelements = 0;

public:
    wrapped_tuple()
    {

    }

};


#include "ubo_tuple_utils.h"
}


class UBOBase: public GLObject<GL_UNIFORM_BUFFER_BINDING>
{
public:
    UBOBase(const GLsizei size, void *storage,
            const GLenum usage = GL_STATIC_READ);

private:
    const GLsizei m_size;
    void *const m_storage;

    bool m_dirty;

protected:
    void delete_globject() override;

public:
    void dump_local_as_floats();
    void mark_dirty(const GLsizei offset, const GLsizei size);
    void update_bound();

public:
    void bind() override;
    void bound() override;
    void sync() override;
    void unbind() override;

    void bind_at(GLuint index);
    void unbind_from(GLuint index);

public:
    inline GLsizei size() const
    {
        return m_size;
    }

};

template <typename... element_ts>
class UBO: public UBOBase
{
public:
    typedef ubo_storage_utils::wrapped_tuple<element_ts...> storage_t;

public:
    UBO():
        UBOBase(sizeof(storage_t), &m_storage),
        m_storage()
    {

    }

private:
    storage_t m_storage;

public:
    template <std::size_t I>
    inline typename std::tuple_element<I, std::tuple<element_ts...>>::type &element()
    {
        return ubo_storage_utils::get<I>(m_storage);
    }

    template <std::size_t I>
    inline const typename std::tuple_element<I, std::tuple<element_ts...>>::type &get() const
    {
        return ubo_storage_utils::get<I>(m_storage);
    }

    template <std::size_t I, typename value_t>
    inline void set(value_t &&ref)
    {
        ubo_storage_utils::set<I>(m_storage, ref);
        mark_dirty<I>();
    }

    template <std::size_t I>
    inline std::size_t offset() const
    {
        return ubo_storage_utils::offset<I>(m_storage);
    }

    template <std::size_t I>
    inline void mark_dirty()
    {
        const GLsizei offset = ubo_storage_utils::offset<I>(m_storage);
        const GLsizei size = ubo_storage_utils::size<I>(m_storage);
        UBOBase::mark_dirty(offset, size);
    }
};

}

#endif
