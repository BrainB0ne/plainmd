# Markdown Viewer

A simple and elegant **Markdown Viewer** built with **Qt6**.

## Features

- Native Qt6 markdown rendering
- File browser sidebar
- Drag and drop support
- Recent files
- Zoom controls

## Code Example

```cpp
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
```

## Table Example

| Feature | Status |
|---------|--------|
| Markdown Rendering | ✅ Supported |
| File Browser | ✅ Supported |
| Drag & Drop | ✅ Supported |

> **Note:** This application uses qmake as the build system.

## Build Instructions

1. Run `qmake mdviewer.pro`
2. Run `make` (or `nmake` on Windows)
