# Agent Notes: plainmd

## Build System
- **qmake only** — `plainmd.pro` is the source of truth. Do not add CMake.
- **Windows (MSVC)**: Must run `vcvarsall.bat x64` before `qmake`/`nmake`.
  - `build.bat` — Full build (vcvarsall + qmake + nmake)
  - `clean.bat` — Clean all build artifacts
  - Manual: `setenv.bat && qmake plainmd.pro && nmake`
- **Linux**: `qmake plainmd.pro && make` → `release/plainmd`.
  - `build.sh` / `clean.sh` convenience wrappers.
- **Installer**: `build-installer.bat` (Windows → `dist/plainmd-<version>-x64-setup.exe`), `build-deb.sh` / `build-appimage.sh` (Linux → `dist/`).
- **Checksums**: `make-checksums.bat` / `make-checksums.sh` after building packages.
- **Editor Integration**: `.zed/tasks.json` (Zed) and `.vscode/tasks.json` (VS Code) have build/deploy/run tasks.
- **Output**: `release/` (MSVC/Linux release), `debug/` (MSVC debug). DESTDIR, MOC_DIR, RCC_DIR, UI_DIR all dump into the same folder.

## Qt6 Quirks
- `QTextEdit::setZoomFactor()` **does not exist in Qt6**. Use `zoomIn(2)` / `zoomOut(2)`. Zoom reset re-applies the editor font at base point size.
- **QTextDocument CSS limitation**: ignores `background-color`, `border-radius`, `padding` on block elements.
- **Document corruption bug**: QTextCursor operations (setBlockFormat/setCharFormat) on documents >5000 bytes corrupt the document. Do not use QTextCursor formatting post-render.
- **Emoji printing**: Color emoji fonts (Segoe UI Emoji) render incorrectly through Windows print spooler. Use Nerd Fonts (CaskaydiaCove, JetBrainsMono) via `editor/printEmojiFont` setting.
- **Export to PDF vs Print**: `QPrinter::PdfFormat` (export) bypasses Windows print drivers — emoji works. `QPrintDialog` (print) goes through spooler — emoji broken. Export to PDF is preferred.

## Architecture
- Single-window app. Entry: `src/main.cpp` → `MainWindow`.
- `QSplitter` with `QTreeView` (file tree) + read-only `QTextEdit` (renderer).
- `QFileSystemModel` wrapped by `FilterProxyModel` (hides empty folders). Root folder exempted via `setExemptPath()`.
- File filter: `*.md`, `*.markdown`, `*.mdx`, `*.txt`.
- **File tree lifecycle**: `setupFileTree()` creates widgets but doesn't attach model. `loadFolder()` attaches `m_proxyModel` on first folder open.
- **Minimap**: Custom `Minimap` widget renders scaled-down overview of document. Detects content types via `QTextCharFormat` properties (font size, bold, anchors, image format, monospace). Theme-aware colors using Catppuccin palette (adapted for light/dark). Toggle via F10 or status bar button, state persisted to `view/showMinimap`. Plain text mode (`setPlainTextMode()`) disables markdown-specific detection for .txt and .mdx files.
- **Status bar**: `setupStatusBar()` creates widgets in two areas: temporary (left side for file message) and permanent (right-aligned for info/buttons). Shows: word count (calculated once on file load via `countWords()`), zoom level (tracks Ctrl+wheel and button zoom), file type, encoding (auto-detected UTF-8 or ANSI using `QStringDecoder`), and icon-only toggle buttons for file tree (`layout-sidebar.png`) and minimap (`layout-sidebar-right.png`). Widgets hidden on welcome page (shows "Ready" in file type to avoid empty separator). Asymmetric padding (8px left, 10px right) on labels for visual centering compensating for Qt's 2px separator. Toggle buttons synced with menu actions (F9/F10) and settings.
- **Search in Files**: `SearchInDialog` (defined in `src/searchindialog.ui`) provides folder-wide search. Uses `QDirIterator` with filters (`*.md`, `*.markdown`, `*.mdx`, `*.txt`) for recursive file discovery. Case-insensitive search via `QString::toLower()`. Match counting with multiple passes (fileContainsText + countMatchesInFile). Results show filename with match count and snippet preview. Dialog persists (non-modal hide/show) to allow picking multiple results. Keyboard navigation: arrows + Enter to open, single-click selects, double-click opens. Signal `fileSelected` passes path and search text to `MainWindow`.
- **Find/F3 workflow**: `FindDialog` (Ctrl+F) and `SearchInDialog` (Ctrl+Shift+F) both emit search text to `MainWindow::m_lastSearchText`. `onFindNext()` (F3) wraps around using `QTextEdit::find()` and cursor manipulation. Find actions disabled on welcome page. Escape key clears selection via `keyPressEvent()`.
- **Auto-reload**: `QFileSystemWatcher` monitors current file. Watching stops on welcome page.
- **Recent history**: Separate tracking for files (`recentFiles`) and folders (`recentFolders`), each with independent privacy toggles (`privacy/keepRecentFiles`, `privacy/keepRecentFolders`). Max 10 entries each (LIFO), missing entries cleaned up.
- **Last folder**: `privacy/rememberLastFolder` restores `lastFolder` on startup.
- **QSettings**: IniFormat, UserScope, org=org name, app=app name. All keys: `recentFiles`, `recentFolders`, `lastFolder`, `privacy/*`, `editor/*`, `view/*` (showFileTree, showMinimap, windowTitleFormat), `geometry`, `windowState`, `splitterState`.

## Critical Implementation Details
- **Minimap color coding**: Content types detected from `QTextCharFormat` during paint:
  - Images: `isImageFormat()` with valid name → Coral rose
  - Headings: Font size ≥14pt or bold → Blue/Green/Peach (H1/H2/H3)
  - Lists: `block.textList() != nullptr` → Yellow
  - Links: `isAnchor()` or underline + blue → Teal  
  - Code: Monospace font or light gray background → Gray
  - Normal text: Default → Gray
  - Background: System button color; colors adapt for light/dark themes (Catppuccin-like palette)
  - **Plain text mode**: `.txt` and `.mdx` files use `setPlainTextMode(true)` which skips heading/link/list detection to avoid false positives from Qt's default formatting
- **Code block styling**: **NOT POSSIBLE** — QTextCursor corrupts large documents. Do not attempt.
- **Welcome page**: Hardcoded HTML/CSS with Consolas (Windows) / DejaVu Sans Mono (Linux) for shortcuts. Not configurable.
- **Tooltips**: Image tooltips show original + cached absolute paths. Link tooltips use `cursorForPosition()` + `charFormat().anchorHref()` for resolved absolute URL. Both use `QDir::cleanPath()` for relative resolution.
- **External images**: Downloaded **synchronously** (10s timeout via `QEventLoop`) to `%TEMP%\plainmd_images\`. Privacy toggle `privacy/previewExternalImages` disables network entirely.
- **Relative images for printing**: `setBaseUrl()` alone fails for print. `resolveRelativeImages()` converts relative paths to `file:///` URLs before `setMarkdown()`.
- **Fenced code protection**: Image resolution functions skip `` ``` `` blocks (regex-based).
- **Frontmatter**: Converted to fenced `yaml` code block before rendering.
- **MDX files**: Rendered as plain text (Qt markdown parser can't handle JSX syntax). Line breaks preserved.
- **Find/F3 search persistence**: `m_lastSearchText` stores the last search term from either FindDialog or SearchInDialog. Cleared when switching files via tree click (to reset context), but preserved for F3 within the same file. When opening from Search in Files, the search text is set AFTER `loadFile()` to ensure it's available for both initial highlighting and subsequent F3 presses.
- **Search highlight clearing**: Escape key handled in `MainWindow::keyPressEvent()` to clear `QTextCursor` selection. Works globally regardless of focus.
- **Encoding detection**: Uses `QStringDecoder` (Qt 6.0+) to auto-detect file encoding. Tries UTF-8 first via `QStringDecoder::Utf8`, falls back to system encoding (`QStringDecoder::System` which is Windows-1252 on Windows, ISO-8859-1/Latin1 on Linux) if UTF-8 decoding has errors. Detection result stored in `m_detectedEncoding` for status bar display.
- **Auto-reload debouncing**: `QFileSystemWatcher::fileChanged` can fire multiple times for a single external save operation. A 500ms debounce timer (`m_fileChangeDebounceTimer`) prevents showing multiple "File Modified" dialogs. The timer is restarted on each file change event, and only shows the dialog when the timer expires (after 500ms of no new events). Additionally, `m_fileChangeDialogOpen` flag prevents multiple dialogs from appearing if the user hasn't responded to the first one yet.
- **Folder protection**: `loadFolder()` validates folders before loading:
  - Empty folder check: `folderHasValidFiles()` scans recursively up to 3 levels deep, stopping at 100 files. Shows status bar warning if no markdown files found.
  - Progress indicator: `m_statusProgress` (QProgressBar in indeterminate mode) shows while scanning. Located on left side of status bar (temporary area), hidden when scan completes. Force repaint with `repaint()` and `QCoreApplication::processEvents()` to ensure visibility.
  - Root drive blocking: `QDir::isRoot()` blocks opening `C:\`, `/`, etc. with warning dialog.
  - System folder blocking: Exact path matching blocks Windows system folders (Windows, Program Files, ProgramData) and Linux system folders (/bin, /usr, /etc, /lib, etc.) with warning dialog.
  - Subfolders allowed: System folder subfolders (e.g., `C:\ProgramData\YourApp`) can be opened.
- **FilterProxyModel**: `hasMatchingFiles()` limited to 1000 files to prevent scanning massive folders. Checks only immediate children + one subdirectory level (not recursive) to prevent blocking the UI.

## Platform Differences
- **Fonts**: Linux defaults to DejaVu Sans (editor) / Noto Sans (emoji). Windows: Segoe UI both. Code font settings removed due to Qt6 bugs.
- **Path separators**: Display with `QDir::toNativeSeparators()`, store normalized (forward slashes).
- **Reveal in folder**: `explorer` (Windows), `xdg-open` (Linux).
- **Menu bar**: File / View / Help only. Preferences under View (Ctrl+,).
- **AppImage**: `build-appimage.sh` bundles `qgtk3` platform theme plugin for native GTK styling on X11/Wayland.

## Resources
- `plainmd.rc` + `icon.ico` → Windows exe icon.
- `plainmd.qrc` + `images/*.png` → runtime icons (Tabler Icons, MIT licensed).
- `tabler-icons/` in `.gitignore`. Copy new icons from `tabler-icons/png/outline/` to `images/` before embedding.

## Verification
- No tests, no CI, no lint config. **Verify by building and running.**
- Samples: `samples/sample.md`, `samples/sample-frontmatter.md`, `samples/image_test.md`.
- C++17 (`CONFIG += c++17` in `.pro`).
- New source/header files must be added to `SOURCES`/`HEADERS` in `plainmd.pro`.
- Build with `build.bat` (Windows) or `build.sh` (Linux).
