#include "texture.h"

#include "util.h"


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

    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
    {
        return GL_LUMINANCE;
    }

    default:
        return GL_RGBA;

    }
}


Texture2D::Texture2D(const GLenum internal_format,
                     const GLsizei width,
                     const GLsizei height):
    GLObject<GL_TEXTURE_BINDING_2D>(),
    GL2DArray(internal_format, width, height)
{
    glGenTextures(1, &m_glid);
    glBindTexture(GL_TEXTURE_2D, m_glid);
    raise_last_gl_error();
    glTexImage2D(GL_TEXTURE_2D, 0, m_internal_format, m_width, m_height, 0,
                 get_suitable_format_for_null(m_internal_format),
                 GL_FLOAT,
                 (const void*)0);
    raise_last_gl_error();
    bound();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::bind()
{
    glBindTexture(GL_TEXTURE_2D, m_glid);
}

void Texture2D::bound()
{

}

void Texture2D::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::attach_to_fbo(const GLenum target, const GLenum attachment)
{
    glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, m_glid, 0);
}

