#include "quickglitem.h"

#include <cassert>
#include <iostream>

#include <QOpenGLContext>
#include <QSGOpacityNode>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_2_Core>

#include "../math/matrix.h"

#include "../io/log.h"
#include "../engine/scenegraph.h"

io::Logger &qml_gl_logger = io::logging().get_logger("qmlgl");


class GridNode: public engine::scenegraph::Node
{
public:
    GridNode(const unsigned int xcells,
             const unsigned int ycells,
             const float size):
        m_vbo(engine::VBOFormat({
                                    engine::VBOAttribute(3)
                                })),
        m_ibo(),
        m_material(),
        m_vbo_alloc(m_vbo.allocate((xcells+1+ycells+1)*2)),
        m_ibo_alloc(m_ibo.allocate((xcells+1+ycells+1)*2))
    {
        auto slice = engine::VBOSlice<Vector3f>(m_vbo_alloc, 0);
        const float x0 = -size*xcells/2.;
        const float y0 = -size*ycells/2.;

        uint16_t *dest = m_ibo_alloc.get();
        unsigned int base = 0;
        for (unsigned int x = 0; x < xcells+1; x++)
        {
            slice[base+2*x] = Vector3f(x0+x*size, y0, 0);
            slice[base+2*x+1] = Vector3f(x0+x*size, -y0, 0);
            *dest++ = base+2*x;
            *dest++ = base+2*x+1;
        }

        base = (xcells+1)*2;
        for (unsigned int y = 0; y < ycells+1; y++)
        {
            slice[base+2*y] = Vector3f(x0, y0+y*size, 0);
            slice[base+2*y+1] = Vector3f(-x0, y0+y*size, 0);
            *dest++ = base+2*y;
            *dest++ = base+2*y+1;
        }

        m_vbo_alloc.mark_dirty();
        m_ibo_alloc.mark_dirty();

        if (!m_material.shader().attach(
                    GL_VERTEX_SHADER,
                    "#version 330\n"
                    "layout(std140) uniform MatrixBlock {"
                    "  layout(row_major) mat4 proj;"
                    "  layout(row_major) mat4 view;"
                    "  layout(row_major) mat4 model;"
                    "  layout(row_major) mat3 normal;"
                    "};"
                    "in vec3 position;"
                    "void main() {"
                    "  gl_Position = proj*view*model*vec4(position, 1.0f);"
                    "}") ||
                !m_material.shader().attach(
                    GL_FRAGMENT_SHADER,
                    "#version 330\n"
                    "out vec4 color;"
                    "void main() {"
                    "  color = vec4(0.5, 0.5, 0.5, 1.0);"
                    "}") ||
                !m_material.shader().link())
        {
            throw std::runtime_error("failed to build shader");
        }

        engine::ArrayDeclaration decl;
        decl.declare_attribute("position", m_vbo, 0);
        decl.set_ibo(&m_ibo);

        m_vao = decl.make_vao(m_material.shader(), true);

        m_material.shader().bind();
        m_material.shader().check_uniform_block<engine::scenegraph::RenderContext::MatrixUBO>(
                    "MatrixBlock");
        m_material.shader().bind_uniform_block(
                    "MatrixBlock",
                    engine::scenegraph::RenderContext::MATRIX_BLOCK_UBO_SLOT);
    }

private:
    engine::VBO m_vbo;
    engine::IBO m_ibo;
    engine::Material m_material;
    std::unique_ptr<engine::VAO> m_vao;

    engine::VBOAllocation m_vbo_alloc;
    engine::IBOAllocation m_ibo_alloc;

public:
    void render(engine::scenegraph::RenderContext &context) override
    {
        context.draw_elements(GL_LINES, *m_vao, m_material, m_ibo_alloc);
    }

    void sync() override
    {
        m_vao->sync();
    }

};


QuickGLScene::QuickGLScene(QObject *parent):
    QObject(parent),
    m_initialized(false),
    m_resources(),
    m_camera(),
    m_scenegraph(),
    m_t(hrclock::now()),
    m_t0(monoclock::now()),
    m_nframes(0)
{
    /*engine::scenegraph::Transformation &transform =
            m_scenegraph.root().emplace<engine::scenegraph::Transformation>();
    transform.emplace_child<GridNode>(10, 10, 10);
    transform.transformation() = translation4(Vector3(100, 100, 0)) * rotation4(eZ, 1.4);*/

    m_scenegraph.root().emplace<GridNode>(64, 64, 1);

    m_camera.controller().set_distance(20.0);
    m_camera.controller().set_rot(Vector2f(45.f/180.f*M_PI, 45.f/180.f*M_PI));
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_scenegraph.render(m_camera);

    hrclock::time_point t1 = hrclock::now();
    const unsigned int msecs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - m_t).count();
    if (msecs >= 1000)
    {
        qml_gl_logger.logf(io::LOG_DEBUG, "fps: %.2lf", (double)m_nframes / (double)msecs * 1000.0d);
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
    m_camera.set_viewport(size.width(), size.height());
    m_camera.set_znear(1.0);
    m_camera.set_zfar(100);
}

void QuickGLScene::sync()
{
    m_camera.sync();
    m_scenegraph.sync();
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
    qml_gl_logger.log(io::LOG_DEBUG, "hover");
    m_hover_pos = event->pos();
}

void QuickGLItem::mouseMoveEvent(QMouseEvent *event)
{
    qml_gl_logger.log(io::LOG_DEBUG, "move");
    m_hover_pos = event->pos();
}

void QuickGLItem::mousePressEvent(QMouseEvent *event)
{
    qml_gl_logger.log(io::LOG_DEBUG, "press");
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
        qml_gl_logger.log(io::LOG_INFO, "initializing window...");

        connect(win, SIGNAL(beforeSynchronizing()),
                this, SLOT(sync()),
                Qt::DirectConnection);
        connect(win, SIGNAL(sceneGraphInvalidated()),
                this, SLOT(cleanup()),
                Qt::DirectConnection);

        win->setSurfaceType(QSurface::OpenGLSurface);

        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setSamples(0);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(8);
        format.setStencilBufferSize(8);
        format.setDepthBufferSize(24);

        win->setFormat(format);
        win->create();

        QOpenGLContext *context = new QOpenGLContext();
        context->setFormat(format);
        if (!context->create()) {
            throw std::runtime_error("failed to create context");
        }

        qml_gl_logger.log(io::LOG_INFO)
                << "created context, version "
                << context->format().majorVersion()
                << "."
                << context->format().minorVersion()
                << io::submit;

        bool context_ok = true;
        io::LogLevel context_info_level = io::LOG_DEBUG;
        if (context->format().profile() != QSurfaceFormat::CoreProfile ||
                context->format().majorVersion() != 3 ||
                context->format().depthBufferSize() == 0)
        {
            context_ok = false;
            context_info_level = io::LOG_WARNING;
            qml_gl_logger.log(io::LOG_EXCEPTION)
                    << "Could not create Core-profile OpenGL 3+ context with depth buffer"
                    << io::submit;
        } else {
            qml_gl_logger.log(io::LOG_DEBUG,
                              "context deemed appropriate, continuing...");
        }

        qml_gl_logger.log(context_info_level)
                << "  renderable  : "
                << (context->format().renderableType() == QSurfaceFormat::OpenGL
                    ? "OpenGL"
                    : context->format().renderableType() == QSurfaceFormat::OpenGLES
                    ? "OpenGL ES"
                    : context->format().renderableType() == QSurfaceFormat::OpenVG
                    ? "OpenVG (software?)"
                    : "unknown")
                << io::submit;
        qml_gl_logger.log(context_info_level)
                << "  rgba        : "
                << context->format().redBufferSize() << " "
                << context->format().greenBufferSize() << " "
                << context->format().blueBufferSize() << " "
                << context->format().alphaBufferSize() << " "
                << io::submit;
        qml_gl_logger.log(context_info_level)
                << "  stencil     : "
                << context->format().stencilBufferSize()
                << io::submit;
        qml_gl_logger.log(context_info_level)
                << "  depth       : " << context->format().depthBufferSize()
                << io::submit;
        qml_gl_logger.log(context_info_level)
                << "  multisamples: " << context->format().samples()
                << io::submit;
        qml_gl_logger.log(context_info_level)
                << "  profile     : "
                << (context->format().profile() == QSurfaceFormat::CoreProfile
                    ? "core"
                    : "compatibility")
                << io::submit;

        if (!context_ok) {
            throw std::runtime_error("Failed to create appropriate OpenGL context");
        }

        context->makeCurrent(win);

        qml_gl_logger.log(io::LOG_INFO)
                << "initializing GLEW in experimental mode"
                << io::submit;
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            const std::string error = std::string((const char*)glewGetErrorString(err));
            qml_gl_logger.log(io::LOG_EXCEPTION)
                    << "GLEW failed to initialize"
                    << error
                    << io::submit;
            throw std::runtime_error("failed to initialize GLEW: " + error);
        }

        qml_gl_logger.log(io::LOG_DEBUG) << "turning off clear" << io::submit;
        win->setClearBeforeRendering(false);

        qml_gl_logger.log(io::LOG_INFO) << "Window and rendering context initialized :)"
                                        << io::submit;
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
    m_renderer->sync();
}

void QuickGLItem::cleanup()
{
    if (m_renderer)
    {
        m_renderer = nullptr;
    }
}
