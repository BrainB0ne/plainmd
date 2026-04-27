# Agent Notes: plainmd

Copyright © 2026 BrainByteZ

## Build System
- **qmake only** — `plainmd.pro` is the source of truth. Do not add CMake.
- **Windows (MSVC)**: Must run `vcvarsall.bat x64` before `qmake`/`nmake`. Use provided scripts:
  - `build.bat` — Full build (vcvarsall + qmake + nmake)
  - `clean.bat` — Clean all build artifacts
  - Manual: `setenv.bat && qmake plainmd.pro && nmake` (or run `vcvarsall.bat x64` directly)
- **Linux**: `qmake plainmd.pro && make`. Output: `release/plainmd`.
  - `build.sh` — convenience wrapper that finds system qmake and builds release
  - `clean.sh` — removes `release/`, `debug/`, Makefiles, and generated files
- **Installer**: Run `build-installer.bat` (Windows, outputs `dist/plainmd-<version>-x64-setup.exe`) or `build-deb.sh`/`build-appimage.sh` (Linux, outputs to `dist/`).
- **Editor Integration**: Build tasks configured in `.zed/tasks.json` (Zed) and `.vscode/tasks.json` (VS Code) for integrated development.

## Qt6 Quirks
- `QTextEdit::setZoomFactor()` **does not exist in Qt6**. Zoom reset by re-setting base font point size.
- **CSS limitation**: `QTextDocument` ignores `background-color`, `border-radius`, `padding` on block elements.
- **Document corruption bug**: QTextCursor operations (setBlockFormat/setCharFormat) on documents >5000 bytes corrupt the document structure. Both `styleCodeBlocks()` and custom CSS styling are **disabled** as a result.
- **Emoji printing bug**: Color emoji fonts (Segoe UI Emoji) render incorrectly when printing to PDF on Windows. Use **Nerd Fonts** (CaskaydiaCove, JetBrainsMono) via `editor/printEmojiFont` setting.
- **Export to PDF vs Print**: Export to PDF uses `QPrinter::PdfFormat` which handles emoji fonts correctly (bypasses Windows print drivers). Print goes through Windows print spooler and has emoji rendering issues. Export to PDF is preferred for digital distribution.

## Architecture
- Single-window app. Entry: `src/main.cpp` → `MainWindow`.
- `QSplitter` with `QTreeView` (file tree) + read-only `QTextEdit` (renderer).
- `QFileSystemModel` wrapped by `FilterProxyModel` (hides empty folders). Root folder exempted via `setExemptPath()`.
- File filter: `*.md`, `*.markdown`, `*.mdx`, `*.txt`.
- **File tree lifecycle**: `setupFileTree()` doesn't attach model immediately. `loadFolder()` attaches `m_proxyModel` on first open.
- **Auto-reload**: `QFileSystemWatcher` monitors current file. Watching stops on welcome page.
- **Recent history**: Separate tracking for recent files (`recentFiles`) and recent folders (`recentFolders`) with independent privacy toggles (`privacy/keepRecentFiles`, `privacy/keepRecentFolders`). Both stored as `QStringList` in QSettings.

## Critical Implementation Details
- **Code block styling**: **DISABLED** — `styleCodeBlocks()` caused document corruption on large files (>5000 bytes). QTextCursor operations corrupt document structure. Code blocks and inline code render with default Qt styling now.
- **Welcome page styling**: Welcome page uses hardcoded CSS with Consolas (Windows) / DejaVu Sans Mono (Linux) for shortcuts. Not configurable.
- **Tooltips**: Image tooltips show original + cached paths (resolved to absolute). Link tooltips show resolved absolute URL on hover via `cursorForPosition()` + `charFormat().anchorHref()`. Both use `QDir::cleanPath()` to resolve `..` and `.` in paths.
- **External images**: Downloaded **synchronously** (10s timeout via `QEventLoop`) to `%TEMP%\plainmd_images\`. Privacy toggle can disable this — no network when off.
- **Relative images for printing**: `setBaseUrl()` alone fails for print. `resolveRelativeImages()` converts relative paths to `file:///` URLs before `setMarkdown()`.
- **Fenced code protection**: `resolveExternalImages()` and `resolveRelativeImages()` skip image syntax inside `` ``` `` blocks (regex-based).
- **Frontmatter**: Converted to fenced `yaml` code block before rendering.
- **MDX files**: Treated as plain text (`.mdx` uses JSX syntax that Qt's markdown parser doesn't support). Line breaks preserved but no rich rendering.

## Platform Differences
- **Fonts**: Linux defaults to DejaVu Sans (editor) and Noto Sans (emoji printing). Windows: Segoe UI for both. Code font settings removed due to Qt6 bugs.
- **Path separators**: Display with `QDir::toNativeSeparators()`, store normalized (forward slashes).
- **Reveal in folder**: Windows uses `explorer`, Linux uses `xdg-open`.
- **Menu bar**: File / View / Help only. Preferences under **View** (Ctrl+,).
- **AppImage styling**: On Linux, `build-appimage.sh` bundles the `qgtk3` platform theme plugin to ensure native GTK styling (matching the DEB package) on both X11 (Cinnamon) and Wayland desktops.

## Resources
- `plainmd.rc` + `icon.ico` → Windows exe icon.
- `plainmd.qrc` + `images/*.png` → runtime icons (Tabler Icons, MIT licensed).
- `tabler-icons/` is in `.gitignore`. Copy new icons from `tabler-icons/png/outline/` to `images/` before embedding.

## Verification
- No tests, no CI, no lint config. **Verify by building and running the executable.**
- Use `samples/sample.md`, `samples/sample-frontmatter.md`, `samples/image_test.md` for manual testing.
- C++17 standard (`CONFIG += c++17` in `.pro`).
- Add new source files to `SOURCES`/`HEADERS` in `plainmd.pro`.
- **Build scripts**: Use `build.bat` / `clean.bat` on Windows, or `build.sh` / `clean.sh` on Linux, for consistent builds.
