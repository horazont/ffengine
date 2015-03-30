#include "quickglitem.h"

#include <cassert>
#include <iostream>

#include <QOpenGLContext>
#include <QSGOpacityNode>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_2_Core>

#include "../math/matrix.h"


QuickGLScene::QuickGLScene(QObject *parent):
    QObject(parent),
    m_initialized(false),
    m_test_shader(),
    m_test_vbo(
        VBOFormat({
                      VBOAttribute(2),
                      VBOAttribute(2)
                  })
        ),
    m_test_valloc(m_test_vbo.allocate(4)),
    m_test_ialloc(m_test_ibo.allocate(4)),
    m_test_texture(GL_RGBA, 256, 256),
    m_t(hrclock::now()),
    m_t0(monoclock::now()),
    m_nframes(0)
{
    std::array<float, 8> data({
                              0, 0,
                              0, 100,
                              100, 100,
                              100, 0
                });

    {
        std::basic_string<unsigned char> texbuffer(256*256*4, 0);
        unsigned char *ptr = &texbuffer.front();
        for (unsigned int row = 0; row < 256; row++) {
            for (unsigned int col = 0; col < 256; col++) {
                unsigned char *const pixel = &ptr[(row*256+col)*4];
                pixel[0] = col;
                pixel[1] = row;
                pixel[2] = (row+col) / 2;
                pixel[3] = 1.0;
            }
        }

        m_test_texture.bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
        engine::raise_last_gl_error();
        GLEW_GET_FUN(__glewGenerateMipmap)(GL_TEXTURE_2D);
        engine::raise_last_gl_error();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        engine::raise_last_gl_error();
        m_test_texture.unbind();
    }

    {
        auto slice = engine::VBOSlice<Vector2f>(m_test_valloc, 0);
        slice[0] = Vector2f(-100, -100);
        slice[1] = Vector2f(-100, 100);
        slice[2] = Vector2f(100, 100);
        slice[3] = Vector2f(100, -100);
    }

    {
        auto slice = engine::VBOSlice<Vector2f>(m_test_valloc, 1);
        slice[0] = Vector2f(0, 0);
        slice[1] = Vector2f(0, 1);
        slice[2] = Vector2f(1, 1);
        slice[3] = Vector2f(1, 0);
    }
    m_test_valloc.mark_dirty();

    {
        uint32_t *dest = m_test_ialloc.get();
        *dest++ = 1;
        *dest++ = 0;
        *dest++ = 2;
        *dest++ = 3;
        m_test_ialloc.mark_dirty();
    }

    std::cout << m_test_shader.attach(
                GL_VERTEX_SHADER,
                "#version 330\n"
                "layout(std140) uniform MatrixBlock {"
                "  layout(row_major) mat4 modelview;"
                "  layout(row_major) mat4 proj;"
                "} matrices;"
                "in vec2 vertex;"
                "in vec2 texcoord0;"
                "out vec2 tc;"
                "void main() {"
                "    gl_Position = matrices.proj * matrices.modelview * vec4(vertex, 0f, 1.0f);"
                "    tc = texcoord0;"
                "}") << std::endl;

    std::cout << m_test_shader.attach(
                GL_FRAGMENT_SHADER,
                "#version 330\n"
                "uniform sampler2D tex;"
                "in vec2 tc;"
                "out vec4 color;"
                "void main() {"
                "    color = texture2D(tex, tc);"
                "}") << std::endl;

    std::cout << m_test_shader.link() << std::endl;

    engine::ArrayDeclaration decl;
    decl.declare_attribute("vertex", m_test_vbo, 0);
    decl.declare_attribute("texcoord0", m_test_vbo, 1);
    decl.set_ibo(&m_test_ibo);

    m_test_vao = decl.make_vao(m_test_shader);

    m_test_shader.bind();

    glUniform1ui(
                m_test_shader.uniform_location("tex"),
                0
            );
    m_test_shader.bind_uniform_block("MatrixBlock", 0);
}

QuickGLScene::~QuickGLScene()
{

}

void QuickGLScene::paint()
{
    if (!m_initialized) {
        m_initialized = true;
    }

    const float alpha = std::chrono::duration_cast<
            std::chrono::duration<float, std::ratio<1>>
            >(monoclock::now() - m_t0).count() * M_PI / 5.;

    glClearColor(0.4, 0.3, 0.2, 1.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    GLEW_GET_FUN(__glewActiveTexture)(GL_TEXTURE0);
    m_test_vao->bind();
    m_test_shader.bind();
    m_test_texture.bind();
    const Matrix4f proj = proj_ortho(0, 0,
                                     m_viewport_size.width(), m_viewport_size.height(),
                                     -2, 2);
    m_test_ubo.bind();
    m_test_ubo.set<0>(translation4(Vector3(m_pos.as_array[0], m_pos.as_array[1], 0.0)) * rotation4(eZ, alpha));
    m_test_ubo.set<1>(proj);
    m_test_ubo.update_bound();
    m_test_ubo.unbind();

    m_test_ubo.bind_at(0);

    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, nullptr);

    m_test_texture.unbind();
    m_test_shader.unbind();
    m_test_vao->unbind();

    hrclock::time_point t1 = hrclock::now();
    const unsigned int msecs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - m_t).count();
    if (msecs > 1000)
    {
        std::cout << "fps: " << (double)m_nframes / (double)msecs * 1000.0d << std::endl;
        m_nframes = 0;
        m_t = t1;
    }
    m_nframes += 1;
}

void QuickGLScene::set_pos(const QPoint &pos)
{
    m_pos = Vector2f(pos.x(), pos.y());
}

void QuickGLScene::set_viewport_size(const QSize &size)
{
    m_viewport_size = size;
}


QuickGLItem::QuickGLItem(QQuickItem *parent):
    QQuickItem(parent),
    m_renderer(nullptr)
{
    setFlags(QQuickItem::ItemHasContents);
    setAcceptHoverEvents(false);
    setAcceptedMouseButtons(Qt::AllButtons);
    connect(this, SIGNAL(windowChanged(QQuickWindow*)),
            this, SLOT(handle_window_changed(QQuickWindow*)));
}

void QuickGLItem::hoverMoveEvent(QHoverEvent *event)
{
    std::cout << "hover" << std::endl;
    m_hover_pos = event->pos();
}

void QuickGLItem::mouseMoveEvent(QMouseEvent *event)
{
    std::cout << "move" << std::endl;
    m_hover_pos = event->pos();
}

void QuickGLItem::mousePressEvent(QMouseEvent *event)
{
    std::cout << "press" << std::endl;
    m_hover_pos = event->pos();
}

QSGNode *QuickGLItem::updatePaintNode(
        QSGNode *oldNode,
        UpdatePaintNodeData *)
{
    update();
    if (!oldNode) {
        oldNode = new QSGOpacityNode();
    }
    oldNode->markDirty(QSGNode::DirtyForceUpdate);
    return oldNode;
}

void QuickGLItem::handle_window_changed(QQuickWindow *win)
{
    if (win)
    {
        std::cout << "win" << std::endl;

        connect(win, SIGNAL(beforeSynchronizing()),
                this, SLOT(sync()),
                Qt::DirectConnection);
        connect(win, SIGNAL(sceneGraphInvalidated()),
                this, SLOT(cleanup()),
                Qt::DirectConnection);

        win->setSurfaceType(QSurface::OpenGLSurface);

        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setVersion(3, 2);
        format.setProfile(QSurfaceFormat::CoreProfile);

        win->setFormat(format);
        win->create();

        QOpenGLContext *context = new QOpenGLContext();
        context->setFormat(format);
        if (!context->create()) {
            throw std::runtime_error("failed to create context");
        }

        std::cout << context->format().majorVersion() << "." <<
                     context->format().minorVersion() << std::endl;

        context->makeCurrent(win);

        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            throw std::runtime_error("failed to initialize GLEW: " +
                                     std::string((const char*)glewGetErrorString(err)));
        }

        win->setClearBeforeRendering(false);
    }
}

void QuickGLItem::sync()
{
    if (!m_renderer)
    {
        m_renderer = std::unique_ptr<QuickGLScene>(new QuickGLScene());
        connect(window(), SIGNAL(beforeRendering()),
                m_renderer.get(), SLOT(paint()),
                Qt::DirectConnection);
    }
    m_renderer->set_viewport_size(window()->size() * window()->devicePixelRatio());
    m_renderer->set_pos(m_hover_pos);
}

void QuickGLItem::cleanup()
{
    if (m_renderer)
    {
        m_renderer = nullptr;
    }
}
