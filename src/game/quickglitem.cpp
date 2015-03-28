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
                      VBOAttribute(2)
                  })
        ),
    m_test_valloc(m_test_vbo.allocate(4)),
    m_test_ialloc(m_test_ibo.allocate(4)),
    m_t(hrclock::now()),
    m_nframes(0)
{
    std::array<float, 8> data({
                              0, 0,
                              0, 100,
                              100, 100,
                              100, 0
                });

    {
        auto slice = VBOSlice<Vector2f>(m_test_valloc, 0);
        slice[0] = Vector2f(0, 0);
        slice[1] = Vector2f(0, 100);
        slice[2] = Vector2f(100, 100);
        slice[3] = Vector2f(100, 0);
        m_test_valloc.mark_dirty();
    }

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
                "uniform mat4 proj;"
                "in vec2 vertex;"
                "out vec2 coords;"
                "void main() {"
                "    gl_Position = proj * vec4(vertex, 0f, 1.0f);"
                "    coords = vertex.xy;"
                "}") << std::endl;

    std::cout << m_test_shader.attach(
                GL_FRAGMENT_SHADER,
                "#version 330\n"
                "in vec2 coords;"
                "out vec4 color;"
                "void main() {"
                "    color = vec4(coords, 0.5f, 1.0f);"
                "}") << std::endl;

    std::cout << m_test_shader.link() << std::endl;

    ArrayDeclaration decl;
    decl.declare_attribute("vertex", m_test_vbo, 0);
    decl.set_ibo(&m_test_ibo);

    m_test_vao = decl.make_vao(m_test_shader);

}

QuickGLScene::~QuickGLScene()
{

}

void QuickGLScene::paint()
{
    if (!m_initialized) {
        m_initialized = true;
    }
    glClearColor(0.4, 0.3, 0.2, 1.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    m_test_vao->bind();
    m_test_shader.bind();
    Matrix4f proj = proj_ortho(0, 0,
                               m_viewport_size.width(), m_viewport_size.height(),
                               -2, 2);
    glUniformMatrix4fvARB(
                    m_test_shader.uniform_location("proj"),
                    1,
                    GL_TRUE,
                    &proj.coeff[0]
                    );

    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, nullptr);

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

void QuickGLScene::set_viewport_size(const QSize &size)
{
    m_viewport_size = size;
}


QuickGLItem::QuickGLItem(QQuickItem *parent):
    QQuickItem(parent),
    m_renderer(nullptr)
{
    setFlags(QQuickItem::ItemHasContents);
    connect(this, SIGNAL(windowChanged(QQuickWindow*)),
            this, SLOT(handle_window_changed(QQuickWindow*)));
}

QSGNode *QuickGLItem::updatePaintNode(
        QSGNode *oldNode,
        UpdatePaintNodeData *updatePaintNodeData)
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
}

void QuickGLItem::cleanup()
{
    if (m_renderer)
    {
        m_renderer = nullptr;
    }
}
