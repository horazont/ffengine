/**********************************************************************
File name: fbo.cpp
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
#include "engine/gl/fbo.hpp"

#include <cassert>

#include "engine/io/log.hpp"

#include "engine/gl/util.hpp"


namespace engine {

static io::Logger &logger = io::logging().get_logger("gl.fbo");

Renderbuffer::Renderbuffer(const GLenum internal_format,
                           const GLsizei width,
                           const GLsizei height):
    GLObject<GL_RENDERBUFFER_BINDING>(),
    GL2DArray(internal_format, width, height)
{
    glGenRenderbuffers(1, &m_glid);
    std::cout << "creating rbo " << m_glid << std::endl;
    glBindRenderbuffer(GL_RENDERBUFFER, m_glid);
    glRenderbufferStorage(GL_RENDERBUFFER, m_internal_format,
                          m_width, m_height);
    raise_last_gl_error();
    bound();
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

Renderbuffer::~Renderbuffer()
{
    if (m_glid != 0) {
        delete_globject();
    }
}

void Renderbuffer::delete_globject()
{
    std::cout << "deleting rbo " << m_glid << std::endl;
    glDeleteRenderbuffers(1, &m_glid);
    m_glid = 0;
}

void Renderbuffer::bind()
{
    glBindRenderbuffer(GL_RENDERBUFFER, m_glid);
    bound();
}

void Renderbuffer::bound()
{
    // some sanity checks for debug mode
    /* assert(static_cast<GLenum>(gl_get_integer(GL_RENDERBUFFER_INTERNAL_FORMAT)) ==
           m_internal_format); */
    /* std::cout << gl_get_integer(GL_RENDERBUFFER_WIDTH) << std::endl;
    std::cout << gl_get_integer(GL_RENDERBUFFER_HEIGHT) << std::endl;
    assert(gl_get_integer(GL_RENDERBUFFER_WIDTH) ==
           m_width);
    assert(gl_get_integer(GL_RENDERBUFFER_HEIGHT) ==
           m_height); */
}

void Renderbuffer::sync()
{

}

void Renderbuffer::unbind()
{
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Renderbuffer::attach_to_fbo(const GLenum target, const GLenum attachment)
{
    glFramebufferRenderbuffer(target, attachment, GL_RENDERBUFFER, m_glid);
}

void Renderbuffer::resize(const GLsizei width, const GLsizei height)
{
    bind();
    glRenderbufferStorage(GL_RENDERBUFFER, m_internal_format,
                          m_width, m_height);
    raise_last_gl_error();
    m_width = width;
    m_height = height;
}


RenderTarget::RenderTarget(GLsizei width, GLsizei height):
    m_bound(false),
    m_current_primary_target(0),
    m_height(height),
    m_width(width)
{

}

RenderTarget::~RenderTarget()
{

}

void RenderTarget::bind(Usage usage)
{
    if (usage == Usage::DRAW || usage == Usage::BOTH) {
        if (m_draw_bound) {
            m_draw_bound->unbound(Usage::DRAW);
        }
        m_draw_bound = this;
    }
    if (usage == Usage::READ || usage == Usage::BOTH) {
        if (m_read_bound) {
            m_read_bound->unbound(Usage::READ);
        }
        m_read_bound = this;
    }
    bound(usage);
}

void RenderTarget::bound(Usage usage)
{
    if (m_bound) {
        return;
    }

    if (usage == Usage::READ) {
        m_current_primary_target = GL_READ_FRAMEBUFFER;
    } else {
        m_current_primary_target = GL_DRAW_FRAMEBUFFER;
    }

    m_bound = true;
}

void RenderTarget::unbound(Usage usage)
{
    if (!m_bound) {
        return;
    }

    if (usage == Usage::DRAW && m_read_bound == this) {
        m_current_primary_target = GL_READ_FRAMEBUFFER;
    } else if (usage == Usage::READ && m_draw_bound == this) {
        m_current_primary_target = GL_DRAW_FRAMEBUFFER;
    } else {
        m_current_primary_target = 0;
        m_bound = false;
    }
}


WindowRenderTarget::WindowRenderTarget():
    RenderTarget(0, 0)
{

}

WindowRenderTarget::WindowRenderTarget(GLsizei width, GLsizei height):
    RenderTarget(width, height)
{

}

void WindowRenderTarget::set_size(const GLsizei width, const GLsizei height)
{
    assert(width >= 0 && height >= 0);
    m_width = width;
    m_height = height;
}

void WindowRenderTarget::bind(Usage usage)
{
    glBindFramebuffer(int(usage), 0);
    raise_last_gl_error();
    RenderTarget::bind(usage);
}

void WindowRenderTarget::bound(Usage usage)
{
    RenderTarget::bound(usage);
    if (m_draw_bound == this) {
        glDrawBuffer(GL_BACK);
    }
    if (m_read_bound == this) {
        glReadBuffer(GL_BACK);
    }
}


FBO::FBO(const GLsizei width, const GLsizei height):
    RenderTarget(width, height),
    m_glid(0),
    m_dirty(false)
{
    glGenFramebuffers(1, &m_glid);
}

FBO &FBO::operator =(FBO &&ref)
{
    if (m_glid != 0) {
        delete_globject();
    }
    m_glid = ref.m_glid;
    m_dirty = ref.m_dirty;
    m_attachments = std::move(ref.m_attachments);
    m_owned_renderbuffers = std::move(ref.m_owned_renderbuffers);
    m_width = ref.m_width;
    m_height = ref.m_height;
    ref.m_glid = 0;
    ref.m_dirty = false;
    ref.m_height = 0;
    ref.m_width = 0;
    return *this;
}

FBO::~FBO()
{
    if (m_glid != 0) {
        delete_globject();
    }
    if (m_draw_bound == this) {
        m_draw_bound = nullptr;
    }
    if (m_read_bound == this) {
        m_read_bound = nullptr;
    }
}

void FBO::delete_globject()
{
    if (m_bound) {
        unbound(Usage::BOTH);
    }
    glDeleteFramebuffers(1, &m_glid);
    m_glid = 0;
    m_owned_renderbuffers.clear();
}

Renderbuffer *FBO::make_renderbuffer(
        const GLenum to_attachment,
        const GLenum internal_format)
{
    require_unused_attachment(to_attachment);

    Renderbuffer *rb = new Renderbuffer(internal_format, m_width, m_height);
    m_owned_renderbuffers.emplace_back(rb);
    m_attachments[to_attachment] = rb;
    mark_dirty_or_attach(to_attachment, rb);
    return rb;
}

void FBO::mark_dirty_or_attach(const GLenum attachment, GL2DArray *obj)
{
    if (m_bound) {
        obj->attach_to_fbo(m_current_primary_target, attachment);
    } else {
        m_dirty = true;
    }
}

void FBO::reconfigure()
{
    assert(m_bound && m_current_primary_target != 0);
    for (auto &attachment: m_attachments)
    {
        attachment.second->attach_to_fbo(m_current_primary_target, attachment.first);
        raise_last_gl_error();
    }
    GLenum status = glCheckFramebufferStatus(m_current_primary_target);
    switch (status)
    {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
    {
        logger.log(io::LOG_ERROR, "FBO has incomplete attachment");
        break;
    }
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
    {
        logger.log(io::LOG_ERROR, "FBO has missing attachment");
        break;
    }
    case GL_FRAMEBUFFER_UNSUPPORTED:
    {
        logger.log(io::LOG_ERROR, "FBO configuration is unsupported");
        break;
    }
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
    {
        logger.log(io::LOG_ERROR, "FBO multisampling is incorrectly configured");
        break;
    }
    case GL_FRAMEBUFFER_COMPLETE:
    {
        logger.log(io::LOG_INFO, "FBO is valid");
        break;
    }
    default:
        logger.logf(io::LOG_WARNING, "FBO status is unknown (%d)", status);
    }

    m_dirty = false;
}

void FBO::require_unused_attachment(const GLenum which)
{
    if (attachment(which)) {
        throw std::runtime_error("duplicate attachment: "+std::to_string(
                                     which));
    }
}

void FBO::attach(const GLenum to_attachment, GL2DArray *obj)
{
    require_unused_attachment(to_attachment);
    m_attachments[to_attachment] = obj;
    mark_dirty_or_attach(to_attachment, obj);
}

Renderbuffer *FBO::make_color_buffer(const unsigned int color_attachment,
                                     const GLenum internal_format)
{
    return make_renderbuffer(GL_COLOR_ATTACHMENT0+color_attachment,
                             internal_format);
}

Renderbuffer *FBO::make_depth_buffer(const GLenum internal_format)
{
    return make_renderbuffer(GL_DEPTH_ATTACHMENT, internal_format);
}

void FBO::resize(GLsizei width, GLsizei height)
{
    std::cout << "FBO::resize(" << width << ", " << height << ")" << std::endl;
    m_width = width;
    m_height = height;
    for (auto &rb: m_owned_renderbuffers)
    {
        std::cout << "resizing rbo " << rb.get() << std::endl;
        rb->resize(width, height);
    }
    bind();
    reconfigure();
}

void FBO::bind(Usage usage)
{
    raise_last_gl_error();
    glBindFramebuffer(int(usage), m_glid);
    raise_last_gl_error();
    RenderTarget::bind(usage);
}

void FBO::bound(Usage usage)
{
    RenderTarget::bound(usage);

    if (m_dirty) {
        reconfigure();
    }
}

/* void FBO::unbind(Usage usage)
{
    if (!m_bound) {
        return;
    }

    assert(m_read_bound == this || m_draw_bound == this);

    if (usage == Usage::BOTH) {
        if (m_read_bound != this) {
            usage = Usage::DRAW;
        }
        if (m_draw_bound != this) {
            usage = Usage::READ;
        }
    }

    glBindFramebuffer(int(usage), 0);
    raise_last_gl_error();

    if (usage == Usage::DRAW || usage == Usage::BOTH) {
        m_draw_bound = nullptr;
    }
    if (usage == Usage::READ || usage == Usage::BOTH) {
        m_read_bound = nullptr;
    }
    unbound(usage);
} */


RenderTarget *RenderTarget::m_draw_bound = nullptr;
RenderTarget *RenderTarget::m_read_bound = nullptr;

}
