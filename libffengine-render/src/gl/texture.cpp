/**********************************************************************
File name: texture.cpp
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
#include "ffengine/gl/texture.hpp"

#include "ffengine/gl/util.hpp"


namespace ffe {

GLint get_suitable_format_for_null(const GLenum internal_format)
{
    switch (internal_format)
    {
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
    {
        return GL_DEPTH_COMPONENT;
    }

    default:
        return GL_RGBA;

    }
}


/* ffe::Texture2D */

Texture2D::Texture2D(const GLenum internal_format,
                     const GLsizei width,
                     const GLsizei height,
                     const GLenum init_format,
                     const GLenum init_type):
    GLObject<GL_TEXTURE_BINDING_2D, Texture>(),
    GL2DArray(internal_format, width, height)
{

    glGenTextures(1, &m_glid);
    glBindTexture(GL_TEXTURE_2D, m_glid);
    raise_last_gl_error();
    reinit(internal_format, width, height, init_format, init_type);
    raise_last_gl_error();
    bound();
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::Texture2D(Texture2D &&src):
    GLObject<GL_TEXTURE_BINDING_2D, Texture>(std::move(src)),
    GL2DArray(src.m_internal_format, src.m_width, src.m_height)
{
    src.m_internal_format = 0;
    src.m_width = 0;
    src.m_height = 0;
}

Texture2D &Texture2D::operator =(Texture2D &&src)
{
    if (m_glid) {
        delete_globject();
    }
    m_glid = src.m_glid;
    m_internal_format = src.m_internal_format;
    m_width = src.m_width;
    m_height = src.m_height;
    src.m_glid = 0;
    src.m_internal_format = 0;
    src.m_width = 0;
    src.m_height = 0;
    return *this;
}

Texture2D::~Texture2D()
{
    if (m_glid) {
        delete_globject();
    }
}

void Texture2D::delete_globject()
{
    glDeleteTextures(1, &m_glid);
    m_glid = 0;
}

void Texture2D::bind()
{
    glBindTexture(GL_TEXTURE_2D, m_glid);
}

void Texture2D::bound()
{

}

void Texture2D::sync()
{

}

void Texture2D::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLenum Texture2D::shader_uniform_type()
{
    return GL_SAMPLER_2D;
}

GLenum Texture2D::target()
{
    return GL_TEXTURE_2D;
}

void Texture2D::attach_to_fbo(const GLenum target, const GLenum attachment)
{
    glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, m_glid, 0);
}

void Texture2D::reinit(const GLenum internal_format,
                       const GLsizei width, const GLsizei height,
                       const GLenum init_format,
                       const GLenum init_type)
{
    const GLenum null_format = (init_format ? init_format : get_suitable_format_for_null(m_internal_format));
    const GLenum null_type = init_type;

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
                 null_format,
                 null_type,
                 (const void*)0);
    raise_last_gl_error();
    m_width = width;
    m_height = height;
    m_internal_format = internal_format;

}


/* ffe::TextureArray */

Texture2DArray::Texture2DArray(const GLenum internal_format,
                               const GLsizei width,
                               const GLsizei height,
                               const GLsizei layers):
    GLObject<GL_TEXTURE_BINDING_2D_ARRAY, Texture>(),
    m_internal_format(internal_format),
    m_width(width),
    m_height(height),
    m_layers(layers)
{
    glGenTextures(1, &m_glid);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_glid);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internal_format,
                   width, height, layers);
    raise_last_gl_error();
}

Texture2DArray::Texture2DArray(Texture2DArray &&src):
    GLObject<GL_TEXTURE_BINDING_2D_ARRAY, Texture>(std::move(src)),
    m_internal_format(src.m_internal_format),
    m_width(src.m_width),
    m_height(src.m_height),
    m_layers(src.m_layers)
{
    src.m_internal_format = 0;
    src.m_width = 0;
    src.m_height = 0;
    src.m_layers = 0;
}

Texture2DArray &Texture2DArray::operator=(Texture2DArray &&src)
{
    if (m_glid != 0) {
        delete_globject();
    }
    m_glid = src.m_glid;
    src.m_glid = 0;
    m_internal_format = src.m_internal_format;
    src.m_internal_format = 0;
    m_width = src.m_width;
    src.m_width = 0;
    m_height = src.m_height;
    src.m_height = 0;
    m_layers = src.m_layers;
    src.m_layers = 0;
    return *this;
}

Texture2DArray::~Texture2DArray()
{
    if (m_glid) {
        delete_globject();
    }
}

void Texture2DArray::delete_globject()
{
    glDeleteTextures(1, &m_glid);
    m_glid = 0;
}

void Texture2DArray::bind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_glid);
}

void Texture2DArray::bound()
{

}

void Texture2DArray::sync()
{

}

void Texture2DArray::unbind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

GLenum Texture2DArray::shader_uniform_type()
{
    return GL_SAMPLER_2D_ARRAY;
}

GLenum Texture2DArray::target()
{
    return GL_TEXTURE_2D_ARRAY;
}

}
