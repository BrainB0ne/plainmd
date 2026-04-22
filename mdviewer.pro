QT += core gui widgets network

CONFIG += c++17

TARGET = mdviewer
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/filterproxymodel.cpp

HEADERS += \
    src/mainwindow.h \
    src/filterproxymodel.h

RC_FILE = appicon.rc
RESOURCES += resources.qrc
