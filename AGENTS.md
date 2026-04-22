# Agent Notes: mdviewer

## Build System
- **qmake only** — `mdviewer.pro` is the source of truth. Do not add CMake files.
- On Windows with MSVC: you must run `vcvarsall.bat x64` before `qmake` and `nmake`, or the compiler (`cl`) will not be found.
- Typical flow:
  ```cmd
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
  qmake mdviewer.pro
  nmake
  ```
- Output lands in `release\mdviewer.exe` (or `debug\` if `CONFIG += debug`).

## Qt6 Quirks
- `QTextEdit::setZoomFactor()` **does not exist in Qt6**. Zoom reset is done by re-setting the base font point size (see `onZoomReset()`).
- Markdown rendering is native via `QTextEdit::setMarkdown()` (no external parser).

## Architecture
- Single-window desktop app. Entry: `src/main.cpp` → `MainWindow`.
- `MainWindow` owns a `QSplitter` with a `QTreeView` (file tree) and a read-only `QTextEdit` (renderer).
- `QFileSystemModel` filters `*.md`, `*.markdown`, `*.txt`.
- Settings (`QSettings`, IniFormat) store last folder, recent files (max 10), window geometry, and splitter state.

## Editing Guidelines
- Keep all source files under `src/`. Update `mdviewer.pro` `SOURCES`/`HEADERS` when adding files.
- C++17 (`CONFIG += c++17`).
- No tests, no lint config, no CI — verify by building and running the executable.

## Runtime Notes
- Accepts an optional file path as a command-line argument.
- Drag-and-drop accepts files (opens) and folders (loads tree).
- `sample.md` at the repo root is useful for quick manual verification.
