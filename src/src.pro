#-------------------------------------------------
#
# Project created by QtCreator 2016-12-20T23:11:57
#
#-------------------------------------------------

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = cashbook
TEMPLATE = app

include( ../askelib_qt/public.pri )
include( ../askelib_qt/askelib/public.pri )

INCLUDEPATH += ..
INCLUDEPATH += ../askelib_qt
INCLUDEPATH += ../third-party/yaml-cpp/include
INCLUDEPATH += ../third-party/

LIBS += -L$${ASKELIBQT_LIB_PATH} -laskelib_qt_std$${ASKELIBQT_LIB_SUFFIX}
LIBS += -L$${ASKELIB_LIB_PATH} -laskelib_std$${ASKELIB_LIB_SUFFIX}

FORMS += \
    mainwindow.ui \
    innodedialog.ui \
    selectwalletdialog.ui

HEADERS += \
    bookkeeping/basic_types.h \
    bookkeeping/models.h \
    bookkeeping/statistics.h \
    bookkeeping/bookkeeping.h \
    bookkeeping/widgets.h \
    mainwindow.h \
    common.h \
    innodedialog.h \
    serialization.h \
    selectwalletdialog.h \
    $$files($$PWD/../third-party/yaml-cpp/src/*.h) \
    $$PWD/../third-party/qtyaml.h

SOURCES += \
    bookkeeping/basic_types.cpp \
    bookkeeping/models.cpp \
    bookkeeping/statistics.cpp \
    bookkeeping/bookkeeping.cpp \
    mainwindow.cpp \
    main.cpp \
    innodedialog.cpp \
    serialization.cpp \
    selectwalletdialog.cpp \
    bookkeeping/widgets.cpp \
    $$files($$PWD/../third-party/yaml-cpp/src/*.cpp)

#QMAKE_CXXFLAGS += -std:c++latest

RESOURCES += \
    resources/resources.qrc
