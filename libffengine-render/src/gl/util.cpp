/**********************************************************************
File name: util.cpp
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
#include "ffengine/gl/util.hpp"

#include <GL/glu.h>

#include <stdexcept>


namespace ffe {

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
