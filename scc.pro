TEMPLATE = subdirs

SUBDIRS += \
    engine \
    game \
    tests

engine.subdir = engine

game.depends = engine
tests.depends = engine

CONFIG += object_parallel_to_source
