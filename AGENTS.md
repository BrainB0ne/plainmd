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
- **Auto-reload**: `QFileSystemWatcher` monitors current file. Watching stops on welcome page.
- **Recent history**: Separate tracking for files (`recentFiles`) and folders (`recentFolders`), each with independent privacy toggles (`privacy/keepRecentFiles`, `privacy/keepRecentFolders`). Max 10 entries each (LIFO), missing entries cleaned up.
- **Last folder**: `privacy/rememberLastFolder` restores `lastFolder` on startup.
- **QSettings**: IniFormat, UserScope, org=org name, app=app name. All keys: `recentFiles`, `recentFolders`, `lastFolder`, `privacy/*`, `editor/*`, `view/*` (showFileTree), `geometry`, `windowState`, `splitterState`.

## Critical Implementation Details
- **Code block styling**: **NOT POSSIBLE** — QTextCursor corrupts large documents. Do not attempt.
- **Welcome page**: Hardcoded HTML/CSS with Consolas (Windows) / DejaVu Sans Mono (Linux) for shortcuts. Not configurable.
- **Tooltips**: Image tooltips show original + cached absolute paths. Link tooltips use `cursorForPosition()` + `charFormat().anchorHref()` for resolved absolute URL. Both use `QDir::cleanPath()` for relative resolution.
- **External images**: Downloaded **synchronously** (10s timeout via `QEventLoop`) to `%TEMP%\plainmd_images\`. Privacy toggle `privacy/previewExternalImages` disables network entirely.
- **Relative images for printing**: `setBaseUrl()` alone fails for print. `resolveRelativeImages()` converts relative paths to `file:///` URLs before `setMarkdown()`.
- **Fenced code protection**: Image resolution functions skip `` ``` `` blocks (regex-based).
- **Frontmatter**: Converted to fenced `yaml` code block before rendering.
- **MDX files**: Rendered as plain text (Qt markdown parser can't handle JSX syntax). Line breaks preserved.

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
