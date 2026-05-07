# Agent Notes: plainmd

**Generated:** 2026-05-05
**Commit:** 7d795f2
**Branch:** master

## OVERVIEW
Qt6/C++17 single-window markdown viewer. qmake-only build. No tests, no CI.

## STRUCTURE
```
.
├── src/                               # 22 files, flat — all C++ source
├── images/                            # 25 Tabler Icon PNGs + LICENSE
├── samples/                           # Test markdown files
├── plainmd.pro                        # qmake — sole build system
├── plainmd.qrc                      # Qt resources
├── eu.brainbytez.plainmd.yaml      # Flatpak manifest
├── AGENTS.md                         # This file
└── [build scripts]                   # .bat (Win), .sh (Linux)
```

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Add new source file | `plainmd.pro` | Must add to SOURCES/HEADERS |
| Main app logic | `src/mainwindow.cpp` | 2343 lines, 58% of codebase |
| Entry point | `src/main.cpp` | QApplication + MainWindow |
| File tree filtering | `src/filterproxymodel.cpp/h` | QSortFilterProxyModel subclass |
| Document minimap | `src/minimap.cpp/h` | Custom paint widget |
| Find in document | `src/finddialog.cpp/h/ui` | Ctrl+F modal |
| Search across files | `src/searchindialog.cpp/h/ui` | Ctrl+Shift+F, non-modal |
| Preferences | `src/preferencesdialog.cpp/h/ui` | Ctrl+, |
| Close File | `src/mainwindow.cpp` | File → Close File (Ctrl+F4), clears editor, keeps folder |
| App icons | `images/*.png` | Copy from `tabler-icons/png/outline/` |
| Windows installer | `installer.iss` | Inno Setup |
| Build scripts | `build.bat` / `build.sh` | Full build pipelines |

## CODE MAP

| Symbol | Type | File | Lines | Role |
|--------|------|------|-------|------|
| MainWindow | class | mainwindow.cpp/h | 2343/179 | Core orchestrator: UI, file I/O, search, print, settings |
| Minimap | class | minimap.cpp/h | 351/63 | Scaled document overview with color-coded content types |
| FilterProxyModel | class | filterproxymodel.cpp/h | 93/47 | Hides non-markdown files and empty directories |
| FindDialog | class | finddialog.cpp/h/ui | 123/58 | In-document search (Ctrl+F) |
| SearchInDialog | class | searchindialog.cpp/h/ui | 248/67 | Folder-wide search (Ctrl+Shift+F) |
| PreferencesDialog | class | preferencesdialog.cpp/h/ui | 201/64 | Settings dialog (Ctrl+,) |
| AboutDialog | class | aboutdialog.cpp/h/ui | 47/43 | App info |
| LicenseDialog | class | licensedialog.cpp/h/ui | 47/41 | GPLv3 text viewer |

## CONVENTIONS
- **qmake only** — `plainmd.pro` is the source of truth. Do not add CMake.
- **Flat src/** — No subdirectories. All .cpp/.h/.ui coexist in `src/`.
- **New files** — Must add to SOURCES/HEADERS/FORMS in `plainmd.pro`.
- **C++17** — `CONFIG += c++17` in `.pro`.
- **Tabler Icons** — Copy new icons from `tabler-icons/png/outline/` to `images/` before embedding.
- **Verify by building** — No tests, no CI. Build and run to verify.

## ANTI-PATTERNS (THIS PROJECT)
- `QTextEdit::setZoomFactor()` — does not exist in Qt6. Use `zoomIn(2)` / `zoomOut(2)`.
- **QTextCursor post-render** — `setBlockFormat`/`setCharFormat` on docs >5000 bytes corrupts the document. Do not use.
- **Code block styling** — NOT POSSIBLE via QTextCursor. Do not attempt.
- **QTextDocument CSS** — ignores `background-color`, `border-radius`, `padding` on block elements.
- **CMake** — Do not add. qmake only.

## CRITICAL DETAILS
- **Encoding**: Auto-detected UTF-8 → fallback to system (Windows-1252/ISO-8859-1). `QStringDecoder`.
- **Line endings**: Sampled from first 8KB. CRLF = Windows, LF = Unix.
- **External images**: Downloaded synchronously (10s timeout) to `%TEMP%\plainmd_images\`.
- **Relative images for print**: `resolveRelativeImages()` converts to `file:///` before `setMarkdown()`.
- **Auto-reload debounce**: 500ms timer + `m_fileChangeDialogOpen` flag.
- **Search text lifecycle**: `m_lastSearchText` cleared on file switch, preserved for F3.
- **Folder protection**: Blocks root drives and system folders. `folderHasValidFiles()` limits to 1000 files, 3 levels deep.

## COMMANDS
```bash
# Linux
./build.sh           # Build release/plainmd
./clean.sh           # Remove build artifacts
./build-deb.sh       # Build .deb package
./build-appimage.sh  # Build AppImage
./build-flatpak.sh   # Build Flatpak (outputs to dist/)
./uninstall-flatpak.sh  # Remove Flatpak from system

# Windows (MSVC)
build.bat           # Full build
build-installer.bat # Build installer
```

## NOTES
- `tabler-icons/` is gitignored (5039 PNG stash). Only `images/` is embedded.
- `.qtcreator/` has stale `.pro.user` files from historical renames.
- `scripts/dev.ahk` is gitignored (Windows AutoHotkey dev hotkeys).
- Version hardcoded in: `plainmd.rc`, `build-deb.sh`, `build-appimage.sh`, `build-flatpak.sh`, `installer.iss`.
- `archive-release.sh`/`.bat` extract version dynamically from `src/main.cpp`.
- Welcome page uses `QApplication::applicationVersion()` dynamically (not hardcoded).
- Flatpak uses KDE Platform 6.7 runtime with Qt6, app ID: `eu.brainbytez.plainmd`.
