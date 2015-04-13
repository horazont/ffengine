#include "engine/gl/texture.hpp"

#include "engine/gl/util.hpp"


namespace engine {

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


Texture::~Texture()
{

}


Texture2D::Texture2D(const GLenum internal_format,
                     const GLsizei width,
                     const GLsizei height,
                     const GLenum init_format,
                     const GLenum init_type):
    GLObject<GL_TEXTURE_BINDING_2D>(),
    GL2DArray(internal_format, width, height)
{
    const GLenum null_format = (init_format ? init_format : get_suitable_format_for_null(m_internal_format));
    const GLenum null_type = init_type;

    glGenTextures(1, &m_glid);
    glBindTexture(GL_TEXTURE_2D, m_glid);
    raise_last_gl_error();
    glTexImage2D(GL_TEXTURE_2D, 0, m_internal_format, m_width, m_height, 0,
                 null_format,
                 null_type,
                 (const void*)0);
    raise_last_gl_error();
    bound();
    glBindTexture(GL_TEXTURE_2D, 0);
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

}
