/**********************************************************************
File name: fbo.hpp
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
#ifndef SCC_ENGINE_GL_FBO_H
#define SCC_ENGINE_GL_FBO_H

#include <GL/glew.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "ffengine/gl/object.hpp"
#include "ffengine/gl/2darray.hpp"
#include "ffengine/gl/texture.hpp"


namespace engine {

class Renderbuffer: public GLObject<GL_RENDERBUFFER_BINDING>,
                    public GL2DArray
{
public:
    Renderbuffer(const GLenum internal_format,
                 const GLsizei width,
                 const GLsizei height);
    ~Renderbuffer() override;

protected:
    void delete_globject() override;

public:
    void bind() override;
    void bound() override;
    void sync() override;
    void unbind() override;

public:
    void attach_to_fbo(const GLenum target, const GLenum attachment) override;
    void resize(const GLsizei width, const GLsizei height);

};

/**
 * A target for rendering, which may include colour and depth buffers.
 */
class RenderTarget
{
public:
    enum class Usage {
        READ = GL_READ_FRAMEBUFFER, ///< Use for reading
        DRAW = GL_DRAW_FRAMEBUFFER, ///< Use for drawing
        BOTH = GL_FRAMEBUFFER       ///< Use for both
    };

public:
    /**
     * Size of the render target
     *
     * @param width Width in pixels
     * @param height Height in pixels
     */
    RenderTarget(GLsizei width, GLsizei height);
    virtual ~RenderTarget();

protected:
    bool m_bound;
    GLenum m_current_primary_target;

    GLsizei m_height;
    GLsizei m_width;

    static RenderTarget *m_draw_bound;
    static RenderTarget *m_read_bound;

public:
    inline GLsizei height() const
    {
        return m_height;
    }

    inline GLsizei width() const
    {
        return m_width;
    }

public:
    /**
     * Bind the render target for a specific Usage.
     *
     * This calls bound().
     *
     * @param usage Usage to bind for
     */
    virtual void bind(Usage usage = Usage::BOTH);

    /**
     * Notify that the target has been bound, possibly by other means, for
     * the given usages.
     *
     * bind() calls this internally.
     *
     * @param usage Usage for which the target was bound.
     */
    virtual void bound(Usage usage);

    /**
     * Notify that the target has been unbound, possibly by another target
     * which was bound.
     *
     * This is called by bind() for the targets which are implicitly unbound.
     *
     * @param usage Usage for which the target was unbound.
     */
    void unbound(Usage usage);

};


/**
 * A fake render target which represents the main render target, identified by
 * OpenGL object id 0.
 */
class WindowRenderTarget: public RenderTarget
{
public:
    WindowRenderTarget();
    WindowRenderTarget(GLsizei width, GLsizei height);

public:
    /**
     * Change the size of the render target.
     */
    void set_size(const GLsizei width, const GLsizei height);

public:
    void bind(Usage usage = Usage::BOTH) override;
    void bound(Usage usage) override;

};


/**
 * A framebuffer object, which is also a RenderTarget.
 */
class FBO: public Resource, public RenderTarget
{
public:
    FBO(const GLsizei width, const GLsizei height);
    FBO(const FBO&) = delete;
    FBO &operator=(const FBO&) = delete;
    FBO &operator=(FBO&&);
    ~FBO();

private:
    GLuint m_glid;

    std::vector<std::unique_ptr<Renderbuffer> > m_owned_renderbuffers;
    std::unordered_map<GLenum, GL2DArray*> m_attachments;

    bool m_dirty;

protected:
    void delete_globject();
    Renderbuffer *make_renderbuffer(
            const GLenum to_attachment,
            const GLenum internal_format);
    void mark_dirty_or_attach(const GLenum attachment, GL2DArray *obj);
    void reconfigure();
    void require_unused_attachment(const GLenum which);

public:
    void attach(const GLenum to_attachment, GL2DArray *rb);

    Renderbuffer *make_color_buffer(
            const unsigned int color_attachment,
            const GLenum internal_format);
    Renderbuffer *make_depth_buffer(
            const GLenum internal_format = GL_DEPTH_COMPONENT32);

    void resize(GLsizei width, GLsizei height);

public:
    inline GL2DArray *attachment(const GLenum attachment) const
    {
        auto iter = m_attachments.find(attachment);
        if (iter == m_attachments.end())
        {
            return nullptr;
        }
        return iter->second;
    }

public:
    void bind(Usage usage = Usage::BOTH) override;
    void bound(Usage usage) override;

};

}

#endif
