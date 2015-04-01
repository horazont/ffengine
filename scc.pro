TEMPLATE = subdirs

SUBDIRS += \
    engine \
    game \
    tests

game.depends = engine
tests.depends = engine

CONFIG += object_parallel_to_source
