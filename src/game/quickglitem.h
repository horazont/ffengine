#ifndef SCC_GAME_QUICKGLITEM_H
#define SCC_GAME_QUICKGLITEM_H

#include "GL/glew.h"

#include <chrono>
#include <memory>

#include <QObject>
#include <QQuickItem>
#include <QQuickWindow>

#include "../engine/gl/vbo.h"
#include "../engine/gl/shader.h"
#include "../engine/gl/ibo.h"
#include "../engine/gl/vao.h"
#include "../engine/gl/texture.h"
#include "../engine/gl/ubo.h"

#include "../engine/scenegraph.h"

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::steady_clock monoclock;


class QuickGLScene: public QObject
{
    Q_OBJECT
public:
    explicit QuickGLScene(QObject *parent = 0);
    ~QuickGLScene();

private:
    bool m_initialized;
    engine::scenegraph::RenderContext::MatrixUBO m_matrix_ubo;
    engine::ResourceManager m_resources;
    engine::scenegraph::Group m_scenegraph_root;
    hrclock::time_point m_t;
    monoclock::time_point m_t0;
    unsigned int m_nframes;
    QSize m_viewport_size;
    Vector2f m_pos;

public slots:
    void paint();

public:
    void set_pos(const QPoint &pos);
    void set_viewport_size(const QSize &size);
    void sync();

};

class QuickGLItem: public QQuickItem
{
    Q_OBJECT

public:
    QuickGLItem(QQuickItem *parent = 0);

private:
    std::unique_ptr<QuickGLScene> m_renderer;
    QPoint m_hover_pos;

protected:
    void hoverMoveEvent(QHoverEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

public slots:
    void cleanup();
    void sync();

private slots:
    void handle_window_changed(QQuickWindow *win);

};

#endif // SCC_GAME_QUICKGLITEM_H
