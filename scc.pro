TEMPLATE = app

QT += qml quick widgets

QMAKE_CXXFLAGS += -std=c++11
unix:LIBS += -lGLEW -lGLU

SOURCES += \
    src/engine/gl/vbo.cpp \
    src/game/quickglitem.cpp \
    src/game/main.cpp \
    src/engine/gl/object.cpp \
    src/engine/gl/array.cpp \
    src/engine/gl/shader.cpp \
    src/engine/gl/vao.cpp \
    src/engine/gl/ibo.cpp \
    src/math/matrix.cpp \
    src/math/vector.cpp \
    src/engine/gl/fbo.cpp \
    src/engine/gl/texture.cpp \
    src/engine/gl/2darray.cpp \
    src/engine/gl/util.cpp \
    src/engine/gl/ubo.cpp \
    src/engine/resource.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    src/engine/gl/vbo.h \
    src/game/quickglitem.h \
    src/engine/gl/object.h \
    src/engine/gl/array.h \
    src/engine/gl/shader.h \
    src/engine/gl/vao.h \
    src/engine/gl/ibo.h \
    src/math/matrix.h \
    src/math/vector.h \
    src/engine/gl/fbo.h \
    src/engine/gl/texture.h \
    src/engine/gl/2darray.h \
    src/engine/gl/util.h \
    src/engine/gl/ubo.h \
    src/engine/gl/ubo_tuple_utils.h \
    src/engine/gl/ubo_type_wrappers.h \
    src/engine/resource.h

DISTFILES +=
CONFIG += object_parallel_to_source
