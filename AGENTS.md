# Agent Notes: vibe-md

## Build System
- **qmake only** — `vibe-md.pro` is the source of truth. Do not add CMake files.
- On Windows with MSVC: you must run `vcvarsall.bat x64` before `qmake` and `nmake`, or the compiler (`cl`) will not be found.
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
- File filter: `*.md`, `*.markdown`, `*.mdx`, `*.txt`.
- Settings (`QSettings`, IniFormat) store last folder, recent files (max 10), window geometry, and splitter state.
- **External images**: `QtNetwork` downloads remote images to `%TEMP%\vibe-md_images\` and replaces URLs with local paths before rendering (`resolveExternalImages()`). A privacy toggle in Preferences can disable this — when off, external image markup is replaced with placeholder text and **no network requests** are made.
- **Relative images for printing**: `QTextDocument::setBaseUrl()` alone is not enough for print. `resolveRelativeImages()` pre-processes the markdown to convert relative image paths to absolute `file:///` URLs before `setMarkdown()`. Without this, local images render on screen but appear as broken placeholders when printed.
- **Frontmatter**: YAML frontmatter (`---` delimited) is converted to a fenced `yaml` code block before rendering so Qt displays it; `styleCodeBlocks()` highlights the first region in blue when it contains `title:` and `date:` keys (`resolveFrontMatter()`).
- **Code block styling**: post-processed after `setMarkdown()` by iterating `QTextDocument` blocks, detecting monospace-only blocks, and applying `QTextBlockFormat` background + margins. Code block font is configurable via Preferences (settings key `editor/codeBlockFontFamily`).
- **Fenced code block protection**: Both `resolveExternalImages()` and `resolveRelativeImages()` skip any image-like syntax (`![alt](url)` or `<img src="...">`) that appears inside `` ```...``` `` blocks, so example markdown in code blocks is not corrupted.
- **Find dialog**: Modeless `FindDialog` with case-sensitive and whole-word search. Lives in `src/finddialog.cpp`.
- **Preferences dialog**: `PreferencesDialog` in `src/preferencesdialog.cpp`. Settings: editor font, code block font, and privacy toggle for external images.
- **Menu bar**: File / View / Help. No Edit menu. Preferences is under **View**.

## Resources
- `appicon.rc` + `icon.ico` → Windows exe icon (`RC_FILE` in `.pro`).
- `resources.qrc` + `icon.png` → runtime window icon.
- `images.qrc` + `images/*.png` → toolbar/menu icons (Tabler Icons, MIT licensed).
- `tabler-icons/` is in `.gitignore`; only the copied icons in `images/` are tracked.

## Editing Guidelines
- Keep all source files under `src/`. Update `vibe-md.pro` `SOURCES`/`HEADERS` when adding files.
- C++17 (`CONFIG += c++17`).
- No tests, no lint config, no CI — verify by building and running the executable.

## Runtime Notes
- Accepts an optional file path as a command-line argument.
- Drag-and-drop accepts files (opens) and folders (loads tree).
- `sample.md` and `sample-frontmatter.md` at the repo root are useful for quick manual verification.
