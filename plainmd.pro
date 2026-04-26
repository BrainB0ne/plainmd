QT += core gui widgets network printsupport

CONFIG += c++17

TARGET = plainmd
TEMPLATE = app

# Place the executable in release/ or debug/ on all platforms
CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

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

RC_FILE = plainmd.rc
RESOURCES += plainmd.qrc
