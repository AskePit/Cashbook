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
    widgets/mainwindow.ui \
    widgets/innodedialog.ui \
    widgets/selectwalletdialog.ui

HEADERS += \
    bookkeeping/basic_types.h \
    bookkeeping/models.h \
    bookkeeping/bookkeeping.h \
    bookkeeping/serialization.h \
    widgets/mainwindow.h \
    widgets/innodedialog.h \
    widgets/selectwalletdialog.h \
    widgets/widgets.h \
    $$files($$PWD/../third-party/yaml-cpp/src/*.h) \
    $$PWD/../third-party/qtyaml.h

SOURCES += \
    bookkeeping/basic_types.cpp \
    bookkeeping/models.cpp \
    bookkeeping/bookkeeping.cpp \
    bookkeeping/serialization.cpp \
    widgets/mainwindow.cpp \
    widgets/innodedialog.cpp \
    widgets/selectwalletdialog.cpp \
    widgets/widgets.cpp \
    main.cpp \
    $$files($$PWD/../third-party/yaml-cpp/src/*.cpp)

#QMAKE_CXXFLAGS += -std:c++latest
QMAKE_CXXFLAGS_RELEASE += -O3

RESOURCES += \
    resources/resources.qrc
