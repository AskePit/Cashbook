#-------------------------------------------------
#
# Project created by QtCreator 2016-12-20T23:11:57
#
#-------------------------------------------------

QT += core widgets gui charts qml quick quickwidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webenginewidgets

TARGET = cashbook
TEMPLATE = app

INCLUDEPATH += third-party/yaml-cpp/include
INCLUDEPATH += third-party/

FORMS += \
    gui/forms/analytics/categoriesstaticchart.ui \
    gui/forms/mainwindow.ui \
    gui/forms/innodedialog.ui \
    gui/forms/selectwalletdialog.ui \
    gui/forms/walletpropertieswindow.ui

HEADERS += \
    bookkeeping/analytics.h \
    bookkeeping/basic_types.h \
    bookkeeping/models.h \
    bookkeeping/bookkeeping.h \
    bookkeeping/money.h \
    bookkeeping/serialization.h \
    gui/forms/analytics/categoriesstaticchart.h \
    gui/forms/mainwindow.h \
    gui/forms/innodedialog.h \
    gui/forms/selectwalletdialog.h \
    gui/forms/walletpropertieswindow.h \
    gui/widgets/widgets.h \
    $$files($$PWD/third-party/yaml-cpp/src/*.h) \
    $$PWD/third-party/qtyaml.h \
    utils/filesystem.h \
    utils/tree.h

SOURCES += \
    bookkeeping/analytics.cpp \
    bookkeeping/basic_types.cpp \
    bookkeeping/models.cpp \
    bookkeeping/bookkeeping.cpp \
    bookkeeping/money.cpp \
    bookkeeping/serialization.cpp \
    gui/forms/analytics/categoriesstaticchart.cpp \
    gui/forms/mainwindow.cpp \
    gui/forms/innodedialog.cpp \
    gui/forms/selectwalletdialog.cpp \
    gui/forms/walletpropertieswindow.cpp \
    gui/widgets/widgets.cpp \
    main.cpp \
    $$files($$PWD/third-party/yaml-cpp/src/*.cpp) \
    utils/filesystem.cpp

win32-msvc* {
    QMAKE_CXXFLAGS_RELEASE += /O2
}
win32-g++ {
    QMAKE_CXXFLAGS_RELEASE += -Ofast
}

RESOURCES += \
    resources/resources.qrc

CONFIG += c++20

RC_ICONS = resources/logo_white.ico
