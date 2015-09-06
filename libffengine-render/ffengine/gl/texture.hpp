/**********************************************************************
File name: texture.hpp
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
#ifndef SCC_ENGINE_GL_TEXTURE_H
#define SCC_ENGINE_GL_TEXTURE_H

#include <GL/glew.h>

#include "ffengine/gl/object.hpp"
#include "ffengine/gl/2darray.hpp"


namespace ffe {

class Texture
{
public:
    virtual ~Texture();

public:
    virtual GLenum shader_uniform_type() = 0;
    virtual GLenum target() = 0;

};

class Texture2D: public GLObject<GL_TEXTURE_BINDING_2D>,
                 public GL2DArray,
                 public Texture
{
public:
    Texture2D(const GLenum internal_format,
              const GLsizei width,
              const GLsizei height,
              const GLenum init_format = 0,
              const GLenum init_type = GL_UNSIGNED_BYTE);
    Texture2D &operator=(Texture2D &&src);
    ~Texture2D() override;

protected:
    void delete_globject() override;

public:
    void bind() override;
    void bound() override;
    void sync() override;
    void unbind() override;

public:
    GLenum shader_uniform_type() override;
    GLenum target() override;

public:
    void attach_to_fbo(const GLenum target, const GLenum attachment) override;
    void reinit(const GLenum internal_format,
                const GLsizei width, const GLsizei height,
                const GLenum init_format = 0,
                const GLenum init_type = GL_UNSIGNED_BYTE);

};

}

#endif
