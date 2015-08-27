/**********************************************************************
File name: ibo.hpp
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
#ifndef SCC_ENGINE_GL_IBO_H
#define SCC_ENGINE_GL_IBO_H

#include "ffengine/gl/array.hpp"

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

static inline void draw_elements(
        const IBOAllocation &alloc, GLenum mode,
        unsigned int nmax = std::numeric_limits<unsigned int>::max())
{
    glDrawElements(mode,
                   std::min(alloc.length(), nmax),
                   IBOAllocation::buffer_t::gl_type,
                   (const GLvoid*)alloc.offset());
}

static inline void draw_elements_base_vertex(
        const IBOAllocation &alloc,
        GLenum mode,
        GLint base_vertex,
        unsigned int nmax = std::numeric_limits<unsigned int>::max())
{
    glDrawElementsBaseVertex(mode, std::min(alloc.length(), nmax), IBOAllocation::buffer_t::gl_type, (const GLvoid*)alloc.offset(), base_vertex);
}

}

#endif
