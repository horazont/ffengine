#-------------------------------------------------
#
# Project created by QtCreator 2015-04-01T12:04:27
#
#-------------------------------------------------

QT       -= core gui

QMAKE_CXXFLAGS += -std=c++11
unix:LIBS += -lGLEW -lGLU

TARGET = engine
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    common/resource.cpp \
    gl/2darray.cpp \
    gl/array.cpp \
    gl/fbo.cpp \
    gl/ibo.cpp \
    gl/material.cpp \
    gl/object.cpp \
    gl/shader.cpp \
    gl/texture.cpp \
    gl/ubo.cpp \
    gl/util.cpp \
    gl/vao.cpp \
    gl/vbo.cpp \
    io/log.cpp \
    math/matrix.cpp \
    math/vector.cpp \
    render/scenegraph.cpp

HEADERS += \
    common/resource.h \
    gl/2darray.h \
    gl/array.h \
    gl/fbo.h \
    gl/ibo.h \
    gl/material.h \
    gl/object.h \
    gl/shader.h \
    gl/texture.h \
    gl/ubo.h \
    gl/ubo_tuple_utils.h \
    gl/ubo_type_wrappers.h \
    gl/util.h \
    gl/vao.h \
    gl/vbo.h \
    io/log.h \
    math/matrix.h \
    math/MatrixTemplates.hpp \
    math/variadic_initializer.h \
    math/vector.h \
    render/scenegraph.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
