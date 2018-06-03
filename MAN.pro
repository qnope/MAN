TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += /std:c++latest /permissive-

SOURCES += main.cpp

HEADERS += \
    man/RangeType.h \
    man/Concept.h \
    man/Model.h \
    man/Trait.h \
    man/Runnable.h \
    man/Chrono.h \
    man/RunnableQueue.h \
    man/copyable_atomic.h \
    man/ThreadPool.h
