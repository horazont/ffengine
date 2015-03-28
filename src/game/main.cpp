#include <iostream>

#include "GL/glew.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickWindow>

#include "quickglitem.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qmlRegisterType<QuickGLItem>("SCC", 1, 0, "GLScene");

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/qml/main.qml"));
    QQuickWindow *root_window = qobject_cast<QQuickWindow*>(component.create());

    return app.exec();
}
