TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
        minieloop.cpp

HEADERS += \
   minieloop.h

LIBS += -pthread
