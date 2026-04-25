QT += core gui widgets network printsupport

CONFIG += c++17 debug_and_release

TARGET = vibe-md
TEMPLATE = app

# Separate output directories for debug and release builds
CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

# Keep generated files inside the build type folder
OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.rcc

# Ensure build subdirectories exist before compiling
mkbuilddirs.target = .dummy
mkbuilddirs.commands = $$QMAKE_MKDIR $$OBJECTS_DIR $$MOC_DIR $$RCC_DIR
QMAKE_EXTRA_TARGETS += mkbuilddirs
PRE_TARGETDEPS += .dummy

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
