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
    src/finddialog.cpp \
    src/aboutdialog.cpp \
    src/licensedialog.cpp

HEADERS += \
    src/mainwindow.h \
    src/filterproxymodel.h \
    src/preferencesdialog.h \
    src/finddialog.h \
    src/aboutdialog.h \
    src/licensedialog.h

FORMS += \
    src/aboutdialog.ui \
    src/licensedialog.ui \
    src/finddialog.ui \
    src/preferencesdialog.ui

RC_FILE = plainmd.rc
RESOURCES += plainmd.qrc
