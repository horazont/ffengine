#include "util.h"

#include <stdexcept>


namespace engine {

GLint gl_get_integer(const GLenum pname)
{
    GLint value = 0;
    glGetIntegerv(pname, &value);
    return value;
}

void raise_gl_error(const GLenum err)
{
    throw std::runtime_error("OpenGL error: " +
                             std::string((const char*)gluErrorString(err)));
}

void raise_last_gl_error()
{
    const GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        raise_gl_error(err);
    }
}

}
