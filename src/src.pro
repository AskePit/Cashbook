#-------------------------------------------------
#
# Project created by QtCreator 2016-12-20T23:11:57
#
#-------------------------------------------------

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = cashbook
TEMPLATE = app

include( ../askelib/public.pri )

INCLUDEPATH += $${ASKE_INCLUDE_PATH}

LIBS += -L$${ASKE_LIB_PATH} -laskelib_std$${ASKE_LIB_SUFFIX}

FORMS += \
    mainwindow.ui \
    innodedialog.ui

HEADERS += \
    mainwindow.h \
    bookkeeping.h \
    types.h \
    innodedialog.h \
    serialization.h

SOURCES += \
    mainwindow.cpp \
    main.cpp \
    bookkeeping.cpp \
    innodedialog.cpp \
    serialization.cpp

#QMAKE_CXXFLAGS += -std:c++latest

RESOURCES += \
    resources/resources.qrc
