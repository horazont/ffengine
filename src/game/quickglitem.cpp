#include "quickglitem.h"

#include <cassert>
#include <iostream>

#include <QOpenGLContext>
#include <QSGOpacityNode>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_2_Core>


QuickGLScene::QuickGLScene(QObject *parent):
    QObject(parent),
    m_initialized(false),
    m_test_shader(),
    m_test_vbo(
        VBOFormat({
                      VBOAttribute(2)
                  })
        ),
    m_test_allocation(m_test_vbo.allocate(4)),
    m_t(hrclock::now()),
    m_nframes(0)
{
    std::array<float, 8> data({
                              -1, -1,
                              -1,  1,
                               1,  1,
                               1, -1
                });

    {
        float *dest = m_test_allocation.get();
        for (auto &value: data) {
            *dest++ = value;
        }
        m_test_allocation.mark_dirty();
    }
    raise_last_gl_error();
    m_test_vbo.bind();
    raise_last_gl_error();
    m_test_vbo.dump_remote_raw();
    m_test_vbo.unbind();
    raise_last_gl_error();

    raise_last_gl_error();
    m_test_ibo.bind();
    raise_last_gl_error();
    m_test_ibo.dump_remote_raw();
    m_test_ibo.unbind();
    raise_last_gl_error();

    std::cout << m_test_shader.attach(
                GL_VERTEX_SHADER,
                "#version 330\n"
                "in vec2 vertex;"
                "out vec2 coords;"
                "void main() {"
                "    gl_Position = vec4(vertex, 0f, 1.0f);"
                "    coords = vertex.xy;"
                "}") << std::endl;
    raise_last_gl_error();

    std::cout << m_test_shader.attach(
                GL_FRAGMENT_SHADER,
                "#version 330\n"
                "in vec2 coords;"
                "out vec4 color;"
                "void main() {"
                "    color = vec4(coords, 0.5f, 1.0f);"
                "}") << std::endl;
    raise_last_gl_error();

    std::cout << m_test_shader.link() << std::endl;

    raise_last_gl_error();
    m_test_shader.bind();
    raise_last_gl_error();
    m_test_shader.unbind();
    raise_last_gl_error();

    ArrayDeclaration decl;
    decl.declare_attribute("vertex", m_test_vbo, 0);
    decl.set_ibo(&m_test_ibo);

    m_test_vao = decl.make_vao(m_test_shader);
    raise_last_gl_error();

    {
        auto ibo_allocation = m_test_ibo.allocate(4);
        uint32_t *dest = ibo_allocation.get();
        *dest++ = 0;
        *dest++ = 1;
        *dest++ = 2;
        *dest++ = 3;
        ibo_allocation.mark_dirty();
    }
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
    raise_last_gl_error();
    m_test_shader.bind();
    raise_last_gl_error();

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
    raise_last_gl_error();

    m_test_shader.unbind();
    raise_last_gl_error();
    m_test_vao->unbind();
    raise_last_gl_error();

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
}

void QuickGLItem::cleanup()
{
    if (m_renderer)
    {
        m_renderer = nullptr;
    }
}
