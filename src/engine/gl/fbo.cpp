#include "fbo.h"

#include <cassert>

#include "util.h"


namespace engine {

Renderbuffer::Renderbuffer(const GLenum internal_format,
                           const GLsizei width,
                           const GLsizei height):
    GLObject<GL_RENDERBUFFER_BINDING>(),
    GL2DArray(internal_format, width, height)
{
    glGenRenderbuffers(1, &m_glid);
    glBindRenderbuffer(GL_RENDERBUFFER, m_glid);
    glRenderbufferStorage(GL_RENDERBUFFER, m_internal_format,
                          m_width, m_height);
    raise_last_gl_error();
    bound();
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Renderbuffer::bind()
{
    glBindRenderbuffer(GL_RENDERBUFFER, m_glid);
    bound();
}

void Renderbuffer::bound()
{
    // some sanity checks for debug mode
    assert(static_cast<GLenum>(gl_get_integer(GL_RENDERBUFFER_INTERNAL_FORMAT)) ==
           m_internal_format);
    assert(gl_get_integer(GL_RENDERBUFFER_WIDTH) ==
           m_width);
    assert(gl_get_integer(GL_RENDERBUFFER_HEIGHT) ==
           m_height);
}

void Renderbuffer::unbind()
{
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Renderbuffer::attach_to_fbo(const GLenum target, const GLenum attachment)
{
    glFramebufferRenderbuffer(target, attachment, GL_RENDERBUFFER, m_glid);
}


FBO::FBO(const GLsizei width, const GLsizei height):
    m_glid(0),
    m_bound(false),
    m_current_primary_target(0),
    m_width(width),
    m_height(height),
    m_dirty(false)
{
    glGenFramebuffers(1, &m_glid);
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
    glDeleteFramebuffers(1, &m_glid);
    m_glid = 0;
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

void FBO::unbound(Usage usage)
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

void FBO::bind(Usage usage)
{
    glBindFramebuffer(int(usage), m_glid);
    raise_last_gl_error();
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

void FBO::bound(Usage usage)
{
    if (m_bound) {
        return;
    }

    if (usage == Usage::READ) {
        m_current_primary_target = GL_READ_FRAMEBUFFER;
    } else {
        m_current_primary_target = GL_DRAW_FRAMEBUFFER;
    }

    if (m_dirty) {
        reconfigure();
    }

    m_bound = true;
}

void FBO::unbind(Usage usage)
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
}


FBO *FBO::m_draw_bound = nullptr;
FBO *FBO::m_read_bound = nullptr;

}
