#ifndef SCC_ENGINE_GL_2DARRAY_H
#define SCC_ENGINE_GL_2DARRAY_H

#include <GL/glew.h>

namespace engine {

class GL2DArray
{
public:
    GL2DArray(const GLenum internal_format,
              const GLsizei width,
              const GLsizei height);

protected:
    const GLenum m_internal_format;
    GLsizei m_width;
    GLsizei m_height;

public:
    inline GLsizei height() const
    {
        return m_height;
    }

    inline GLenum internal_format() const
    {
        return m_internal_format;
    }

    inline GLsizei width() const
    {
        return m_width;
    }

public:
    virtual void attach_to_fbo(const GLenum target, const GLenum attachment) = 0;

};

}

#endif
