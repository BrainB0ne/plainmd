# Agent Notes: plainmd

**Generated:** 2026-05-09
**Commit:** 6aa93aa
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
| Reload | `src/mainwindow.cpp` | View → Reload (F5), `onReload()`, `loadFile()` |
| Copy Path | `src/mainwindow.cpp` | File tree context menu: Copy File/Folder Path, Show in Explorer/File Manager |
| Navigation History | `src/mainwindow.cpp` | Back/Forward (`Alt+←`/`Alt+→`), session-only, privacy toggle |
| Zen Mode | `src/mainwindow.cpp` | `F11` toggle, hides sidebar/toolbar/statusbar/menubar, Escape to exit |
| Enhanced Find | `src/finddialog.cpp/h/ui` | Case sensitive, whole words, regex toggle. Regex uses `QRegularExpression` on rendered plain text. Whole words disabled when regex active |


| Document Outline | `src/mainwindow.cpp` | Left `QTabWidget` (tabs at bottom): Files/Outline, `setupOutline()`, `updateOutline()`, `headingLevel()` scan |
| App icons | `images/*.png` | Copy from `tabler-icons/png/outline/` |
| Windows installer | `installer.iss` | Inno Setup |
| Windows portable ZIP | `build-zip.bat` | Creates portable distribution with `portable` marker |
| Release archive | `archive-release.sh`/`.bat` | Versioned zip with prefix directory |
| Checksums | `make-checksums.sh`/`.bat` | SHA256 per package + combined |
| All-in-one release | `build-release.sh`/`.bat` | Full pipeline: build → package → checksum → archive |
| Build scripts | `build.bat` / `build.sh` | Full build pipelines |

## CODE MAP

| Symbol | Type | File | Lines | Role |
|--------|------|------|-------|------|
| MainWindow | class | mainwindow.cpp/h | 2925/216 | Core orchestrator: UI, file I/O, search, print, settings |
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
- **Manual reload**: View → Reload (F5) unconditionally re-runs `loadFile(m_currentFile)`. Disabled on welcome page.
- **Document outline**: Left `QTabWidget` (tabs at bottom) with Files/Outline panes. Outline deferred via `QTimer::singleShot` after `setMarkdown()` so Qt's async heading format assignment finishes before scanning `headingLevel()`. Hierarchical QTreeWidget; click scrolls to heading via `QTextCursor::setPosition()`. Cleared on welcome page.
- **File tree context menu**: Right-click any file → Copy File Path, Show in Explorer (Windows) / File Manager (Linux). Right-click any folder → Copy Folder Path, Show in Explorer. Uses `QDir::toNativeSeparators()` for clipboard paths.
- **Navigation history**: Session-only back/forward stack (`Alt+←`/`Alt+→`). Toolbar buttons and View menu actions. Truncates forward history when branching. Privacy toggle in Preferences clears existing history. Never persisted to disk.
- **Zen Mode**: `F11` toggles distraction-free view. Hides sidebar, toolbar, status bar, and menu bar. Stores pre-zen visibility states to restore exactly on exit. Explicit `F11` fallback in `keyPressEvent` because `QAction` shortcut stops working when menu bar is hidden. Escape also exits zen mode.
- **Enhanced Find**: `Ctrl+F` dialog with case sensitive, whole words, and regular expression toggles. Regex searches use `QRegularExpression` on rendered plain text (markdown syntax is not preserved in rendered content). Whole words checkbox auto-disables when regex is active. Invalid regex shows error message. F3 "Find Next" remembers all search options including regex state.
- **Search text lifecycle**: `m_lastSearchText` cleared on file switch, preserved for F3.
- **Folder protection**: Blocks root drives and system folders. `folderHasValidFiles()` limits to 1000 files, 3 levels deep.
- **CLI args**: `main.cpp` passes `argv[1]` to `MainWindow::openPath()` which detects directory vs file via `QFileInfo` and calls `loadFolder()` or `openFile()` accordingly. Relative paths are resolved to absolute before storing in recent folders.
- **Plain text loading**: `QTextEdit::setPlainText()` inherits the cursor's current char format. After clicking a heading in the outline, the cursor holds bold formatting. When switching to a `.txt` file, this leaks into the plain text display. Fix: call `setCurrentCharFormat(QTextCharFormat())` before `setPlainText()` to reset the format.
- **Portable mode** (Windows): A `portable` file next to the executable triggers self-contained mode. `MainWindow::isPortable()` checks for this marker. When active: `QSettings` uses `Data/settings.ini` in the app directory; external image cache goes to `Data/images/`. The portable ZIP builder (`build-zip.bat`) includes the `portable` marker automatically.

## COMMANDS
```bash
# Linux
./build.sh              # Build release/plainmd
./clean.sh              # Remove build artifacts
./build-deb.sh          # Build .deb package
./build-appimage.sh     # Build AppImage
./build-flatpak.sh      # Build Flatpak (outputs to dist/)
./build-release.sh      # All-in-one: .deb + AppImage + Flatpak + checksums + archive
./uninstall-flatpak.sh  # Remove Flatpak from system

# Windows (MSVC)
build.bat              # Full build
build-installer.bat    # Build installer
build-zip.bat          # Build portable ZIP distribution
build-release.bat      # All-in-one: installer + ZIP + checksums + archive
```

## NOTES
- `tabler-icons/` is gitignored (5039 PNG stash). Only `images/` is embedded.
- `.qtcreator/` has stale `.pro.user` files from historical renames.
- `scripts/dev.ahk` is gitignored (Windows AutoHotkey dev hotkeys).
- Portable ZIP (`build-zip.bat`) includes all Qt6 DLLs, plugins, and `README-PORTABLE.txt`.
- Version hardcoded in: `plainmd.rc`, `installer.iss`.
- Dynamically extracted from `src/main.cpp`: `build-deb.sh`, `build-appimage.sh`, `build-flatpak.sh`, `archive-release.sh`, `archive-release.bat`, `build-release.sh`, `build-release.bat`.
- Welcome page uses `QApplication::applicationVersion()` dynamically (not hardcoded).
- Flatpak uses KDE Platform 6.7 runtime with Qt6, app ID: `eu.brainbytez.plainmd`.
