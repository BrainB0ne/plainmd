# Agent Notes: vibe-md

## Build System
- **qmake only** — `vibe-md.pro` is the source of truth. Do not add CMake files.

### Linux
- Typical flow:
  ```bash
  qmake vibe-md.pro
  make
  ```
- Output lands in `release/vibe-md` (or `debug/vibe-md` if `CONFIG += debug`).
- Packaging scripts at repo root:
  - `build-deb.sh` — builds `.deb` against **system Qt6** (not a local Qt installation)
  - `install-deb.sh` / `uninstall-deb.sh` — convenience wrappers
  - `build-appimage.sh` — builds a portable `.AppImage`

### Windows (MSVC)
- You must run `vcvarsall.bat x64` before `qmake` and `nmake`, or the compiler (`cl`) will not be found.
- Typical flow:
  ```cmd
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
  qmake vibe-md.pro
  nmake
  ```
- Output lands in `release\vibe-md.exe` (or `debug\` if `CONFIG += debug`).
- After building, run `windeployqt release\vibe-md.exe` to copy required Qt DLLs and plugins into the output folder.

## Qt6 Quirks
- `QTextEdit::setZoomFactor()` **does not exist in Qt6**. Zoom reset is done by re-setting the base font point size (see `onZoomReset()`).
- Markdown rendering is native via `QTextEdit::setMarkdown()` (no external parser).
- **CSS limitation**: `QTextDocument` ignores `background-color`, `border-radius`, and `padding` on block-level elements like `<pre>`. Any styling that requires block backgrounds (e.g. code blocks) must be applied **after** `setMarkdown()` via `QTextBlockFormat` / `QTextCharFormat` manipulation (see `styleCodeBlocks()`).

## Architecture
- Single-window desktop app. Entry: `src/main.cpp` → `MainWindow`.
- `MainWindow` owns a `QSplitter` with a `QTreeView` (file tree) and a read-only `QTextEdit` (renderer).
- `QFileSystemModel` is wrapped by a custom `FilterProxyModel` that hides folders with no matching files.
  - **Empty-folder safeguard**: `FilterProxyModel::setExemptPath()` ensures the current root folder is never filtered out, even when it contains no matching files.
- File filter: `*.md`, `*.markdown`, `*.mdx`, `*.txt`.
- **File tree lifecycle**: `setupFileTree()` does **not** attach the model immediately. The tree starts empty; `loadFolder()` attaches `m_proxyModel` on the first open and sets the root index.
- Settings (`QSettings`, IniFormat) store last folder, recent files (max 10), window geometry, and splitter state.
- **External images**: `QtNetwork` downloads remote images **synchronously** (10s timeout via `QEventLoop`) to `%TEMP%\vibe-md_images\` and replaces URLs with local paths before rendering (`resolveExternalImages()`). A privacy toggle in Preferences can disable this — when off, external image markup is replaced with placeholder text and **no network requests** are made.
- **Relative images for printing**: `QTextDocument::setBaseUrl()` alone is not enough for print. `resolveRelativeImages()` pre-processes the markdown to convert relative image paths to absolute `file:///` URLs before `setMarkdown()`. Without this, local images render on screen but appear as broken placeholders when printed.
- **Plain text**: `.txt` files bypass the markdown pipeline and are displayed with `setPlainText()` so line breaks are preserved.
- **Frontmatter**: YAML frontmatter (`---` delimited) is converted to a fenced `yaml` code block before rendering so Qt displays it; `styleCodeBlocks()` highlights the first region in blue when it contains `title:` and `date:` keys (`resolveFrontMatter()`).
- **Code block styling**: post-processed after `setMarkdown()` by iterating `QTextDocument` blocks, detecting monospace-only blocks, and applying `QTextBlockFormat` background + margins. Code block font is configurable via Preferences (settings key `editor/codeBlockFontFamily`).
  - **Detection heuristic**: a block is considered code only if *every* fragment reports `fixedPitch` **or** a font family containing `mono`, `Courier`, `Consolas`, `Menlo`, or `Liberation`. Changing fonts or Qt font resolution can break this.
  - Empty blocks between two code blocks are coerced into the code region, and any list bullet markers are removed so code does not appear inside a `QTextList`.
- **Inline code font**: Post-processed in `styleCodeBlocks()` to use the same configurable font as fenced code blocks (settings key `editor/codeBlockFontFamily`).
- **Fenced code block protection**: Both `resolveExternalImages()` and `resolveRelativeImages()` skip any image-like syntax (`![alt](url)` or `<img src="...">`) that appears inside `` ```...``` `` blocks, so example markdown in code blocks is not corrupted.
- **Find dialog**: Modeless `FindDialog` with case-sensitive and whole-word search. Lives in `src/finddialog.cpp`.
- **Preferences dialog**: `PreferencesDialog` in `src/preferencesdialog.cpp`. Settings: editor font, **code font** (applies to both inline and block code), **external editor path**, privacy toggle for external images, and optional recent-files history.
- **Menu bar**: File / View / Help. No Edit menu. Preferences is under **View**.
- **Print action**: Disabled when the welcome page is shown (`m_printAction->setEnabled(false)` in `showWelcomePage()`, enabled when a file is loaded).
- **Emoji printing**: Color emoji fonts (like Segoe UI Emoji) may not render correctly when printing to PDF on Windows. Configure a **Nerd Font** (e.g., "CaskaydiaCove Nerd Font", "JetBrainsMono Nerd Font") in Preferences for proper monochrome emoji printing. Settings: `editor/printEmojiFont` (family), `editor/printEmojiFontSize`, and `editor/useNerdFontForEmoji`.
- **Platform-specific code**: The file-tree context menu uses `QProcess::startDetached("explorer", ...)` to reveal files on Windows, and `QProcess::startDetached("xdg-open", ...)` to open the parent directory on Linux.
- **External editor**: Configurable in Preferences; context menu on file tree shows "Open with External Editor" action (icon: `edit.png` from Tabler Icons) when an editor path is set. Path stored in `editor/externalEditor`, displayed with native separators but stored normalized.
- **Linux font defaults**: `applyEditorFont()`, `styleCodeBlocks()`, and the welcome-page CSS use `#ifdef Q_OS_LINUX` to default to **DejaVu Sans** (editor) and **DejaVu Sans Mono** (code blocks) instead of Segoe UI / Consolas.
- **Native path separators**: Recent Files menu, status bar messages, and External Editor path field use `QDir::toNativeSeparators()` for display. Paths are stored normalized (forward slashes) via `QDir::fromNativeSeparators()`.

## Resources
- `vibe-md.rc` + `icon.ico` → Windows exe icon (`RC_FILE` in `.pro`).
- `vibe-md.qrc` + `icon.png` + `images/*.png` → runtime window icon and toolbar/menu icons (Tabler Icons, MIT licensed).
- `tabler-icons/` is in `.gitignore`; only the copied icons in `images/` are tracked.
- New icons should be picked from `tabler-icons/png/outline/` (the outline set) and copied into `images/` before embedding.

## Editing Guidelines
- Keep all source files under `src/`. Update `vibe-md.pro` `SOURCES`/`HEADERS` when adding files.
- C++17 (`CONFIG += c++17`).
- No tests, no lint config, no CI — verify by building and running the executable.

## Runtime Notes
- Accepts an optional file path as a command-line argument.
- Drag-and-drop accepts files (opens) and folders (loads tree).
- `samples/sample.md`, `samples/sample-frontmatter.md`, and `samples/image_test.md` are useful for quick manual verification.
