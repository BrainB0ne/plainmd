# Vibe-MD

A simple and elegant **Markdown Viewer** built with **Qt6**.

![Vibe-MD Icon](icon.png)

## Features

- **Native Qt6 markdown rendering** - Fast, native rendering without external dependencies
- **File browser sidebar** - Browse and open markdown files with a tree view
- **Multiple format support** - Markdown (.md), MDX (.mdx), and plain text (.txt)
- **Drag and drop support** - Open files and folders by dragging them into the window
- **Recent files** - Quick access to recently opened files
- **Auto-reload on file change** - Detects external file modifications and prompts to reload
- **Zoom controls** - Zoom in/out with Ctrl++ and Ctrl+-
- **Find/Search** - Search within documents with Ctrl+F
- **Print support** - Print to PDF or physical printer
- **External editor integration** - Open files with your preferred editor from the context menu
- **Customizable fonts** - Configure editor font, code font, and emoji print font
- **Privacy options** - Toggle external image loading and recent files history
- **YAML frontmatter display** - Shows frontmatter in a styled code block

## Installation

### Windows

Download the latest `vibe-md-setup.exe` and run the installer.

The installer will:
- Install Vibe-MD to your Program Files folder
- Create Start Menu and optional Desktop shortcuts
- Optionally associate .md, .markdown, and .mdx file extensions

### Linux

**Debian/Ubuntu (.deb package):**
```bash
sudo dpkg -i vibe-md_1.2_amd64.deb
```

**AppImage (portable):**
```bash
chmod +x vibe-md-x86_64.AppImage
./vibe-md-x86_64.AppImage
```

**Build from source:**
```bash
qmake vibe-md.pro
make
sudo cp release/vibe-md /usr/local/bin/
```

## Building from Source

### Prerequisites

- Qt 6.x (with Qt6Core, Qt6Gui, Qt6Widgets, Qt6Network, Qt6PrintSupport)
- C++17 compatible compiler

### Windows (Visual Studio 2022)

```batch
:: Setup MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:: Generate build files
qmake vibe-md.pro

:: Build
nmake

:: Deploy Qt dependencies
windeployqt release\vibe-md.exe

:: Create installer (optional, requires Inno Setup)
build-installer.bat
```

### Linux

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install qt6-base-dev build-essential

# Generate build files
qmake vibe-md.pro

# Build
make

# Output: release/vibe-md
```

## Usage

### Opening Files

- **File Menu** → Open File (Ctrl+O) or Open Folder (Ctrl+Shift+O)
- **Drag and drop** files or folders into the window
- **Command line:** `vibe-md <file.md>`
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
| Ctrl++, | Preferences |
| Ctrl++ | Zoom in |
| Ctrl+- | Zoom out |
| Ctrl+0 | Reset zoom |

## Configuration

Access preferences via **View → Preferences** (Ctrl+,):

- **Editor Font** - Main text font
- **Code Font** - Font for code blocks and inline code
- **Emoji Print Font** - Font for emoji characters when printing (use a Nerd Font for best results)
- **External Editor** - Path to your preferred external editor
- **Privacy** - Toggle external image preview and recent files history

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
vibe-md/
├── src/                    # Source code
│   ├── main.cpp
│   ├── mainwindow.cpp/h
│   ├── preferencesdialog.cpp/h
│   ├── finddialog.cpp/h
│   └── filterproxymodel.cpp/h
├── images/                 # Application icons
├── samples/                # Sample markdown files
├── vibe-md.pro             # qmake project file
├── vibe-md.qrc             # Qt resources
├── vibe-md.rc              # Windows resources
├── installer.iss           # Inno Setup installer script
├── build-installer.bat     # Windows build script
├── build-deb.sh            # Debian package builder
├── build-appimage.sh       # AppImage builder
└── AGENTS.md               # Developer documentation
```

## License

GPLv3 License - See [LICENSE](LICENSE) file for details.

## Acknowledgments

- Icons by [Tabler Icons](https://tabler.io/icons) (MIT License)
- Built with [Qt6](https://www.qt.io)

---

**Happy markdown viewing!** 📄✨
