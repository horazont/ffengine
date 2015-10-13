/**********************************************************************
File name: 2darray.hpp
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
#ifndef SCC_ENGINE_GL_2DARRAY_H
#define SCC_ENGINE_GL_2DARRAY_H

#include <epoxy/gl.h>

namespace ffe {

class GL2DArray
{
public:
    GL2DArray(const GLenum internal_format,
              const GLsizei width,
              const GLsizei height);
    GL2DArray(GL2DArray &&src) = default;

protected:
    GLenum m_internal_format;
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
