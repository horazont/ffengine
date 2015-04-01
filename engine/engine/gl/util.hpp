#ifndef SCC_ENGINE_GL_UTIL_H
#define SCC_ENGINE_GL_UTIL_H

#include <GL/glew.h>

#include <iostream>

namespace engine {

GLint gl_get_integer(const GLenum pname);

void raise_gl_error(const GLenum err);
void raise_last_gl_error();

}

#endif
