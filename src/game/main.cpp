#include <iostream>

#include "GL/glew.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickWindow>

#include "quickglitem.h"

#include "../io/log.h"


int main(int argc, char *argv[])
{
    io::logging().attach_sink<io::LogAsynchronousSink>(
                std::move(std::unique_ptr<io::LogSink>(new io::LogTTYSink()))
                )->set_synchronous(true);
    io::logging().log(io::LOG_INFO) << "Log initialized" << io::submit;

    io::logging().get_logger("engine.gl.array").set_level(io::LOG_WARNING);
    io::logging().get_logger("engine.gl.vao").set_level(io::LOG_WARNING);
    io::logging().get_logger("engine.scenegraph").set_level(io::LOG_WARNING);

    QApplication app(argc, argv);
    io::logging().log(io::LOG_INFO) << "QApplication initialized" << io::submit;

    qmlRegisterType<QuickGLItem>("SCC", 1, 0, "GLScene");
    io::logging().log(io::LOG_INFO) << "GL Scene registered with QML" << io::submit;

    QQmlEngine engine;
    io::logging().log(io::LOG_INFO) << "QML engine initialized" << io::submit;
    QQmlComponent component(&engine, QUrl("qrc:/qml/main.qml"));
    component.create();
    io::logging().log(io::LOG_INFO) << "QML scene created" << io::submit;

    io::logging().log(io::LOG_INFO) << "Ready to roll out!" << io::submit;
    int exitcode = app.exec();

    io::logging().log(io::LOG_INFO) << "Terminated. Exit code: " << exitcode
                                    << io::submit;
    return exitcode;
}
