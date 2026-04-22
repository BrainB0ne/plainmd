QT += core gui widgets network

CONFIG += c++17

TARGET = mdviewer
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/mainwindow.h

RC_FILE = appicon.rc
RESOURCES += resources.qrc
