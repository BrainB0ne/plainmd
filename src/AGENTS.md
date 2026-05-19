# Agent Notes: src/

## OVERVIEW
Flat source directory — all C++ code, UI forms, and the entry point. No subdirectories.

## STRUCTURE
```
src/
├── main.cpp              # Entry point (36 lines)
├── mainwindow.cpp/h      # Core app window (2925/216 lines)
├── minimap.cpp/h         # Document minimap (351/63 lines)
├── filterproxymodel.cpp/h # File tree filter (93/47 lines)
├── finddialog.cpp/h/ui   # Find in document (123/58 lines)
├── searchindialog.cpp/h/ui # Search in files (248/67 lines)
├── preferencesdialog.cpp/h/ui # Settings (201/64 lines)
├── aboutdialog.cpp/h/ui  # About dialog (47/43 lines)
└── licensedialog.cpp/h/ui # License viewer (47/41 lines)
```

## WHERE TO LOOK
| Task | File | Notes |
|------|------|-------|
| Add menu/action | `mainwindow.cpp/h` | All menus, toolbars, actions defined here |
| File loading/rendering | `mainwindow.cpp` | `loadFile()`, `resolveRelativeImages()`, `resolveExternalImages()`, line endings detection, `setCurrentCharFormat()` reset for `.txt` |
| Status bar | `mainwindow.cpp` | `setupStatusBar()`, 8 widgets (word count, zoom, file type, encoding, line endings, wrap, toggles) |
| Auto-reload | `mainwindow.cpp` | `QFileSystemWatcher` with 500ms debounce |
| Drag & drop / CLI | `mainwindow.cpp` | `dragEnterEvent()`, `dropEvent()`, `openPath()` |
| Copy code | `mainwindow.cpp` | Context menu, monospace detection |
| CLI entry | `main.cpp` | Passes `argv[1]` to `MainWindow::openPath()` |
| Close File | `mainwindow.cpp` | File → Close File (Ctrl+F4), `onCloseFile()` |
| Reload | `mainwindow.cpp` | View → Reload (F5), `onReload()`, disabled on welcome page |
| Copy Path | `mainwindow.cpp` | File tree context menu: Copy File/Folder Path, Show in Explorer/File Manager |
| Navigation History | `mainwindow.cpp` | Back/Forward (`Alt+←`/`Alt+→`), session-only, privacy toggle |
| Zen Mode | `mainwindow.cpp` | `F11` toggle, hides sidebar/toolbar/statusbar/menubar, Escape to exit |
| Enhanced Find | `finddialog.cpp/h/ui` | Case sensitive, whole words, regex toggle. Regex uses `QRegularExpression` on rendered plain text. Whole words disabled when regex active |
| Document Outline | `mainwindow.cpp` | Left `QTabWidget` (tabs at bottom), `setupOutline()`, `updateOutline()`, deferred `headingLevel()` scan |
| Portable mode | `mainwindow.cpp` | `isPortable()` checks for `portable` marker next to exe |
| Zoom | `mainwindow.cpp` | `zoomIn(2)`/`zoomOut(2)` — Qt6 only |
| Minimap colors | `minimap.cpp` | Catppuccin palette, content type detection |
| Minimap click/scroll | `minimap.cpp` | `mousePressEvent()`, `mouseMoveEvent()` |
| File tree filter | `filterproxymodel.cpp` | `hasMatchingFiles()` limited to 1000 files |
| Find wrap-around | `finddialog.cpp` | `QTextEdit::find()` with cursor manipulation |
| Search results | `searchindialog.cpp` | `QDirIterator`, match counting, snippet preview |
| Preferences UI | `preferencesdialog.cpp` | `QSettings` read/write |

## CONVENTIONS
- **Flat structure** — No `ui/`, `models/`, `widgets/` subdirectories. All files in `src/`.
- **UI files** — `.ui` forms for dialogs only. MainWindow is code-only.
- **Signal naming** — `fileSelected(path, text)` from SearchInDialog to MainWindow.
- **Member prefix** — `m_` for all private members (`m_editor`, `m_proxyModel`, etc.).

## ANTI-PATTERNS
- **Do not add subdirectories** — qmake handles flat fine; subdirs complicate `.pro`.
- See root `AGENTS.md` for Qt6 API and QTextCursor restrictions.
