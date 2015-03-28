TEMPLATE = app

QT += qml quick widgets

QMAKE_CXXFLAGS += -std=c++11

SOURCES += \
    src/engine/gl/vbo.cpp \
    src/game/quickglitem.cpp \
    src/game/main.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    src/engine/gl/vbo.h \
    src/game/quickglitem.h

DISTFILES +=
