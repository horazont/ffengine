#include "ubo.h"

#include <iostream>


namespace engine {

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
