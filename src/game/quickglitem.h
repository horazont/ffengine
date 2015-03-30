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
    engine::ResourceManager m_resources;
    engine::ShaderProgram &m_test_shader;
    engine::VBO &m_test_vbo;
    engine::IBO &m_test_ibo;
    engine::VBOAllocation m_test_valloc;
    engine::IBOAllocation m_test_ialloc;
    engine::Texture2D &m_test_texture;
    engine::UBO<Matrix4f, Matrix4f> &m_test_ubo;
    engine::VAO *m_test_vao;
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
