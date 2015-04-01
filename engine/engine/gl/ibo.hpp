#ifndef SCC_ENGINE_GL_IBO_H
#define SCC_ENGINE_GL_IBO_H

#include "engine/gl/array.hpp"

namespace engine {

class IBO: public GLArray<uint16_t,
                          GL_ELEMENT_ARRAY_BUFFER,
                          GL_ELEMENT_ARRAY_BUFFER_BINDING,
                          IBO>
{
public:
    static constexpr GLenum gl_type = GL_UNSIGNED_SHORT;

public:
    IBO();

};

typedef IBO::allocation_t IBOAllocation;

static inline void draw_elements(const IBOAllocation &alloc, GLenum mode)
{
    glDrawElements(mode, alloc.length(), IBOAllocation::buffer_t::gl_type, (const GLvoid*)alloc.offset());
}

static inline void draw_elements_base_vertex(
        const IBOAllocation &alloc,
        GLenum mode,
        GLint base_vertex)
{
    glDrawElementsBaseVertex(mode, alloc.length(), IBOAllocation::buffer_t::gl_type, (const GLvoid*)alloc.offset(), base_vertex);
}

}

#endif
