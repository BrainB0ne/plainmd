QT += core gui widgets network printsupport

CONFIG += c++17 debug_and_release

TARGET = plainmd
TEMPLATE = app

# Separate output directories for debug and release builds
CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

# Dump all generated files directly into the build type folder
OBJECTS_DIR = $$DESTDIR
MOC_DIR = $$DESTDIR
RCC_DIR = $$DESTDIR
UI_DIR = $$DESTDIR

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
