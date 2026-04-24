QT += core gui widgets network printsupport

CONFIG += c++17

TARGET = vibe-md
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/filterproxymodel.cpp \
    src/preferencesdialog.cpp \
    src/finddialog.cpp

HEADERS += \
    src/mainwindow.h \
    src/filterproxymodel.h \
    src/preferencesdialog.h \
    src/finddialog.h

RC_FILE = appicon.rc
RESOURCES += resources.qrc images.qrc
