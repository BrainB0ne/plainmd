# PlainMD

A simple and elegant **Markdown Viewer** built with **Qt6**.

**PlainMD is partially vibe-coded with OpenCode: Kimi K2.5 Turbo / Kimi K2.6**

![PlainMD Icon](icon.png)

## Features

- **Native Qt6 markdown rendering** - Fast, native rendering without external dependencies
- **File browser sidebar** - Browse and open markdown files with a tree view
- **Multiple format support** - Markdown (.md), MDX (.mdx), and plain text (.txt)
- **Drag and drop support** - Open files and folders by dragging them into the window
- **Recent files & folders** - Quick access to recently opened files and folders with separate history and privacy toggles
- **Auto-reload on file change** - Detects external file modifications and prompts to reload
- **Zoom controls** - Zoom in/out with Ctrl++ and Ctrl+-
- **Find/Search** - Search within documents with Ctrl+F
- **Print & Export** - Print to physical printer or export directly to PDF (with better emoji support)
- **External editor integration** - Open files with your preferred editor from the context menu
- **Customizable fonts** - Configure editor font and emoji print font
- **Privacy options** - Toggle external image loading, recent files, and recent folders history independently
- **URL tooltips** - Hover over links to see the resolved absolute path
- **YAML frontmatter display** - Shows frontmatter as a code block

## Screenshot
![PlainMD Screenshot](screenshots/plainmd-screenshot.png)

## Building from Source

### Prerequisites

- Qt 6.x (with Qt6Core, Qt6Gui, Qt6Widgets, Qt6Network, Qt6PrintSupport)
- C++17 compatible compiler

### Windows (Visual Studio 2022)

**Quick build using provided scripts:**
```batch
:: Build
build.bat

:: Clean build artifacts
clean.bat

:: Deploy Qt dependencies (after building)
windeployqt release\plainmd.exe
```

**Manual build:**
```batch
:: Setup MSVC environment (or run vcvarsall.bat x64 directly)
setenv.bat

:: Generate build files and build
qmake plainmd.pro && nmake

:: Deploy Qt dependencies
windeployqt release\plainmd.exe

:: Create installer (optional, requires Inno Setup)
build-installer.bat
```

**Editor integration:** Build tasks are configured for Zed (`.zed/tasks.json`) and VS Code (`.vscode/tasks.json`) for integrated development.

### Linux

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install qt6-base-dev build-essential

# Generate build files
qmake plainmd.pro

# Build
make

# Output: release/plainmd
```

## Usage

### Opening Files

- **File Menu** → Open File (Ctrl+O) or Open Folder (Ctrl+Shift+O)
- **Drag and drop** files or folders into the window
- **Command line:** `plainmd <file.md>`
- **Double-click** .md files (after file association on Windows)

### Navigation

- **File tree** on the left shows all markdown files in the current folder
- **Recent files** in the File menu for quick access
- Click any file in the tree to view it instantly

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+O | Open file |
| Ctrl+Shift+O | Open folder |
| Ctrl+F | Find in document |
| Ctrl+P | Print |
| Ctrl+Shift+P | Export to PDF |
| Ctrl++, | Preferences |
| Ctrl++ | Zoom in |
| Ctrl+- | Zoom out |
| Ctrl+0 | Reset zoom |

## Configuration

Access preferences via **View → Preferences** (Ctrl+,):

- **Editor Font** - Main text font
- **Emoji Print Font** - Font for emoji characters when printing (use a Nerd Font for best results)
- **External Editor** - Path to your preferred external editor
- **Privacy** - Toggle external image preview, recent files, and recent folders history independently

### Emoji Printing on Windows (Experimental)

Color emoji fonts (like Segoe UI Emoji) may not render correctly when printing to PDF. To fix this:

1. Install a Nerd Font (e.g., "CaskaydiaCove Nerd Font", "JetBrainsMono Nerd Font")
2. Open Preferences → Editor
3. Set "Emoji Print Font" to your Nerd Font
4. Check "Use for emoji printing"
5. Now emojis will print correctly as monochrome glyphs

## Architecture

- **Single-window desktop app** - Entry: `src/main.cpp` → `MainWindow`
- **Qt6 native rendering** - Uses `QTextEdit::setMarkdown()` for rendering (no external parser)
- **File system model** - `QFileSystemModel` wrapped with `FilterProxyModel` for file tree
- **Auto-reload** - `QFileSystemWatcher` monitors loaded files for external changes
- **Settings** - `QSettings` (IniFormat) for preferences persistence

## Project Structure

```
PlainMD/
├── src/                    # Source code
│   ├── main.cpp
│   ├── mainwindow.cpp/h
│   ├── preferencesdialog.cpp/h
│   ├── finddialog.cpp/h
│   └── filterproxymodel.cpp/h
├── images/                 # Application icons (Tabler Icons)
├── samples/                # Sample markdown files
├── .zed/                   # Zed Editor configuration
│   └── tasks.json          # Build tasks for Zed
├── .vscode/                # VS Code configuration
│   └── tasks.json          # Build tasks for VS Code
├── plainmd.pro             # qmake project file
├── plainmd.qrc             # Qt resources
├── plainmd.rc              # Windows resources
├── build.bat               # Windows build script
├── clean.bat               # Windows clean script
├── setenv.bat              # Set up MSVC environment
├── build-installer.bat     # Windows installer builder
├── build-deb.sh            # Debian package builder
├── build-appimage.sh       # AppImage builder
├── installer.iss           # Inno Setup installer script
├── README.md               # User documentation
└── AGENTS.md               # Developer documentation
```

## License

GPLv3 License - See [LICENSE](LICENSE) file for details.

## Acknowledgments

- Icons by [Tabler Icons](https://tabler.io/icons) (MIT License)
- Built with [Qt6](https://www.qt.io)

---

**Happy markdown viewing!** 📄✨
