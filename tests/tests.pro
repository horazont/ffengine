TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    engine/math/matrix.cpp \
    engine/math/vector.cpp \
    engine/math/ray.cpp

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/catch/include/

INCLUDEPATH += $$PWD/../engine/

LIBS += -L../engine -lengine

include(deployment.pri)
qtcAddDeployment()

