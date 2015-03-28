#ifndef SCC_GAME_QUICKGLITEM_H
#define SCC_GAME_QUICKGLITEM_H

#include <memory>

#include <QObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QOpenGLFunctions>

class MyFooRenderer: public QObject, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit MyFooRenderer(QObject *parent = 0);
    ~MyFooRenderer();

private:
    bool m_initialized;

public slots:
    void paint();

};

class MyFooItem: public QQuickItem
{
    Q_OBJECT

public:
    MyFooItem(QQuickItem *parent = 0);

private:
    std::unique_ptr<MyFooRenderer> m_renderer;

protected:
    virtual QSGNode *updatePaintNode(QSGNode *oldNode,
                                     UpdatePaintNodeData *updatePaintNodeData);

public slots:
    void cleanup();
    void sync();

private slots:
    void handle_window_changed(QQuickWindow *win);

};

#endif // SCC_GAME_QUICKGLITEM_H
