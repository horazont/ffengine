/**********************************************************************
File name: ubo.cpp
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
#include "ffengine/gl/ubo.hpp"

#include <iostream>


namespace ffe {

UBOBase::UBOBase(const GLsizei size, void *storage, const GLenum usage):
    GLObject<GL_UNIFORM_BUFFER_BINDING>(),
    m_size(size),
    m_storage(storage)
{
    std::cout << size << std::endl;
    glGenBuffers(1, &m_glid);
    glBindBuffer(GL_UNIFORM_BUFFER, m_glid);
    glBufferData(GL_UNIFORM_BUFFER, m_size, nullptr, usage);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBOBase::delete_globject()
{
    glDeleteBuffers(1, &m_glid);
}

void UBOBase::dump_local_as_floats()
{
    const float *ptr = (const float*)m_storage;
    const GLsizei N = m_size / sizeof(float);
    const float *const end = ptr + N;

    std::cout << "BEGIN OF local UNIFORM BUFFER DUMP" << std::endl;
    for (; ptr != end; ptr++)
    {
        std::cout << *ptr << std::endl;
    }
    std::cout << "END OF local UNIFORM BUFFER DUMP" << std::endl;
}

void UBOBase::mark_dirty(const GLsizei, const GLsizei)
{
    m_dirty = true;
}

void UBOBase::update_bound()
{
    if (!m_dirty) {
        return;
    }

    glBufferSubData(GL_UNIFORM_BUFFER, 0, m_size, m_storage);
}

void UBOBase::bind()
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_glid);
}

void UBOBase::bound()
{

}

void UBOBase::sync()
{
    bind();
    update_bound();
}

void UBOBase::unbind()
{
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBOBase::bind_at(GLuint index)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, index, m_glid);
}

void UBOBase::unbind_from(GLuint index)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, index, 0);
}

}
