#ifndef SCC_ENGINE_GL_FBO_H
#define SCC_ENGINE_GL_FBO_H

#include <GL/glew.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "object.h"
#include "2darray.h"
#include "texture.h"


class Renderbuffer: public GLObject<GL_RENDERBUFFER_BINDING>,
                    public GL2DArray
{
public:
    Renderbuffer(const GLenum internal_format,
                 const GLsizei width,
                 const GLsizei height);

public:
    void bind() override;
    void bound() override;
    void unbind() override;

public:
    void attach_to_fbo(const GLenum target, const GLenum attachment) override;

};


class FBO
{
public:
    enum class Usage {
        READ = GL_READ_FRAMEBUFFER,
        DRAW = GL_DRAW_FRAMEBUFFER,
        BOTH = GL_FRAMEBUFFER
    };

public:
    FBO(const GLsizei width, const GLsizei height);
    FBO(const FBO&) = delete;
    FBO &operator=(const FBO&) = delete;
    FBO(const FBO&&) = delete;
    FBO &operator=(const FBO&&) = delete;
    ~FBO();

private:
    GLuint m_glid;
    bool m_bound;
    GLenum m_current_primary_target;

    const GLsizei m_height;
    const GLsizei m_width;

    std::vector<std::unique_ptr<Renderbuffer> > m_owned_renderbuffers;
    std::unordered_map<GLenum, GL2DArray*> m_attachments;

    bool m_dirty;

    static FBO *m_draw_bound;
    static FBO *m_read_bound;

protected:
    void delete_globject();
    Renderbuffer *make_renderbuffer(
            const GLenum to_attachment,
            const GLenum internal_format);
    void mark_dirty_or_attach(const GLenum attachment, GL2DArray *obj);
    void reconfigure();
    void require_unused_attachment(const GLenum which);
    void unbound(Usage usage);

public:
    void attach(const GLenum to_attachment, GL2DArray *rb);

    Renderbuffer *make_color_buffer(
            const unsigned int color_attachment,
            const GLenum internal_format);
    Renderbuffer *make_depth_buffer(
            const GLenum internal_format = GL_DEPTH_COMPONENT32);

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

    inline GLsizei height() const
    {
        return m_height;
    }

    inline GLsizei width() const
    {
        return m_width;
    }

public:
    void bind(Usage usage = Usage::BOTH);
    void bound(Usage usage);
    void unbind(Usage usage = Usage::BOTH);

};

#endif
