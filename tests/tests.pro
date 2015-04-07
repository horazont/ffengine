TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    engine/math/matrix.cpp \
    engine/math/vector.cpp \
    engine/math/ray.cpp \
    engine/sim/quadterrain.cpp \
    engine/math/shapes.cpp \
    engine/render/quadterrain.cpp

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/catch/include/

INCLUDEPATH += $$PWD/../engine/

LIBS += -L../engine -lengine

include(deployment.pri)

CONFIG += object_parallel_to_source

unix {
    make_engine.commands = cd ..; make sub-engine
}

QMAKE_EXTRA_TARGETS += make_engine
PRE_TARGETDEPS += make_engine ../engine/libengine.a
