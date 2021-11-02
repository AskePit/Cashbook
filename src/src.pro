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
    gui/forms/mainwindow.ui \
    gui/forms/innodedialog.ui \
    gui/forms/selectwalletdialog.ui \
    gui/forms/walletpropertieswindow.ui

HEADERS += \
    bookkeeping/basic_types.h \
    bookkeeping/models.h \
    bookkeeping/bookkeeping.h \
    bookkeeping/serialization.h \
    gui/forms/mainwindow.h \
    gui/forms/innodedialog.h \
    gui/forms/selectwalletdialog.h \
    gui/forms/walletpropertieswindow.h \
    gui/widgets/widgets.h \
    $$files($$PWD/../third-party/yaml-cpp/src/*.h) \
    $$PWD/../third-party/qtyaml.h

SOURCES += \
    bookkeeping/basic_types.cpp \
    bookkeeping/models.cpp \
    bookkeeping/bookkeeping.cpp \
    bookkeeping/serialization.cpp \
    gui/forms/mainwindow.cpp \
    gui/forms/innodedialog.cpp \
    gui/forms/selectwalletdialog.cpp \
    gui/forms/walletpropertieswindow.cpp \
    gui/widgets/widgets.cpp \
    main.cpp \
    $$files($$PWD/../third-party/yaml-cpp/src/*.cpp)


win32-msvc* {
    QMAKE_CXXFLAGS_RELEASE += /O2
    QMAKE_CXXFLAGS += /std:c++17
}
win32-g++ {
    QMAKE_CXXFLAGS_RELEASE += -O3
    QMAKE_CXXFLAGS += -std=c++17
}

RESOURCES += \
    resources/resources.qrc
