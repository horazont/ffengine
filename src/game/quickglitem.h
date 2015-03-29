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


class QuickGLScene: public QObject
{
    Q_OBJECT
public:
    explicit QuickGLScene(QObject *parent = 0);
    ~QuickGLScene();

private:
    bool m_initialized;
    ShaderProgram m_test_shader;
    VBO m_test_vbo;
    IBO m_test_ibo;
    VBOAllocation m_test_valloc;
    IBOAllocation m_test_ialloc;
    Texture2D m_test_texture;
    UBO<Matrix4f, Matrix4f> m_test_ubo;
    std::unique_ptr<VAO> m_test_vao;
    hrclock::time_point m_t;
    unsigned int m_nframes;
    QSize m_viewport_size;

public slots:
    void paint();

public:
    void set_viewport_size(const QSize &size);

};

class QuickGLItem: public QQuickItem
{
    Q_OBJECT

public:
    QuickGLItem(QQuickItem *parent = 0);

private:
    std::unique_ptr<QuickGLScene> m_renderer;

protected:
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

public slots:
    void cleanup();
    void sync();

private slots:
    void handle_window_changed(QQuickWindow *win);

};

#endif // SCC_GAME_QUICKGLITEM_H
