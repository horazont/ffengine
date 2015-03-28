#include "quickglitem.h"

#include <iostream>

#include <QOpenGLContext>
#include <QSGOpacityNode>

MyFooRenderer::MyFooRenderer(QObject *parent):
    QObject(parent),
    m_initialized(false)
{

}

MyFooRenderer::~MyFooRenderer()
{

}

void MyFooRenderer::paint()
{
    if (!m_initialized) {
        initializeOpenGLFunctions();
    }
    glClearColor(0.4, 0.3, 0.2, 1.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}


MyFooItem::MyFooItem(QQuickItem *parent):
    QQuickItem(parent),
    m_renderer(nullptr)
{
    setFlags(QQuickItem::ItemHasContents);
    connect(this, SIGNAL(windowChanged(QQuickWindow*)),
            this, SLOT(handle_window_changed(QQuickWindow*)));
}

QSGNode *MyFooItem::updatePaintNode(
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

void MyFooItem::handle_window_changed(QQuickWindow *win)
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
        context->create();

        context->makeCurrent(win);

        win->setClearBeforeRendering(false);
    }
}

void MyFooItem::sync()
{
    if (!m_renderer)
    {
        m_renderer = std::unique_ptr<MyFooRenderer>(new MyFooRenderer());
        connect(window(), SIGNAL(beforeRendering()),
                m_renderer.get(), SLOT(paint()),
                Qt::DirectConnection);
    }
}

void MyFooItem::cleanup()
{
    if (m_renderer)
    {
        m_renderer = nullptr;
    }
}
