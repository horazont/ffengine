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
    src/common/resource.cpp \
    src/gl/2darray.cpp \
    src/gl/array.cpp \
    src/gl/fbo.cpp \
    src/gl/ibo.cpp \
    src/gl/material.cpp \
    src/gl/object.cpp \
    src/gl/shader.cpp \
    src/gl/texture.cpp \
    src/gl/ubo.cpp \
    src/gl/util.cpp \
    src/gl/vao.cpp \
    src/gl/vbo.cpp \
    src/io/log.cpp \
    src/math/matrix.cpp \
    src/math/vector.cpp \
    src/render/scenegraph.cpp \
    src/render/camera.cpp \
    src/render/rendergraph.cpp \
    src/render/terrain.cpp \
    src/sim/terrain.cpp

HEADERS += \
    engine/common/resource.hpp \
    engine/gl/2darray.hpp \
    engine/gl/array.hpp \
    engine/gl/fbo.hpp \
    engine/gl/ibo.hpp \
    engine/gl/material.hpp \
    engine/gl/object.hpp \
    engine/gl/shader.hpp \
    engine/gl/texture.hpp \
    engine/gl/ubo.hpp \
    engine/gl/ubo_tuple_utils.hpp \
    engine/gl/ubo_type_wrappers.hpp \
    engine/gl/util.hpp \
    engine/gl/vao.hpp \
    engine/gl/vbo.hpp \
    engine/io/log.hpp \
    engine/math/matrix.hpp \
    engine/math/MatrixTemplates.hpppp \
    engine/math/variadic_initializer.hpp \
    engine/math/vector.hpp \
    engine/render/scenegraph.hpp \
    engine/render/camera.hpp \
    engine/common/types.hpp \
    engine/render/rendergraph.hpp \
    engine/render/terrain.hpp \
    engine/sim/terrain.hpp

unix {
    target.path = /usr/lib
    INSTALLS += target
}

CONFIG += object_parallel_to_source
