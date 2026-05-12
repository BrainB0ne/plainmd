/*
 * PlainMD
 * Copyright (C) 2026 BrainByteZ
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "filterproxymodel.h"
#include "preferencesdialog.h"
#include "finddialog.h"
#include "searchindialog.h"
#include "aboutdialog.h"
#include "minimap.h"

#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHelpEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPrinter>
#include <QPrintDialog>
#include <QProcess>
#include <QRegularExpression>
#include <QStyle>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextList>
#include <QTextStream>
#include <QStringDecoder>
#include <QTimer>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QSizePolicy>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QApplication::organizationName(), QApplication::applicationName())
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setAcceptDrops(true);
    setWindowTitle(tr("PlainMD"));
    setWindowIcon(QIcon(":/icon.png"));
    resize(1200, 800);

    // Track if we should show welcome page (folder loaded but no file opened)
    bool folderLoadedNoFile = false;

    // Restore last opened folder if privacy setting allows (do this first)
    if (m_settings.value("privacy/rememberLastFolder", true).toBool()) {
        QString lastFolder = m_settings.value("lastFolder").toString();
        if (!lastFolder.isEmpty() && QDir(lastFolder).exists()) {
            loadFolder(lastFolder);
            folderLoadedNoFile = true;  // Folder loaded, but no file yet
        }
    }

    // Restore last opened file if privacy setting allows (loads file's folder only if no lastFolder)
    if (m_settings.value("privacy/rememberLastFile", true).toBool()) {
        QString lastFile = m_settings.value("lastFile").toString();
        if (!lastFile.isEmpty() && QFile::exists(lastFile)) {
            // If no folder was loaded, openFile will load the file's folder
            // If a folder was already loaded, don't load the file's folder (keep lastFolder)
            openFile(lastFile, m_currentFolder.isEmpty());
            folderLoadedNoFile = false;  // File was opened
        }
    }

    // If no file/folder was restored, show welcome page
    // Also show welcome page if folder was loaded but no file was opened (rememberLastFile is OFF)
    if (m_currentFile.isEmpty() && m_currentFolder.isEmpty()) {
        showWelcomePage();
    } else if (folderLoadedNoFile && m_currentFile.isEmpty()) {
        showWelcomePage();
    }
    refreshRecentFilesMenu();
    refreshRecentFoldersMenu();

    // Restore file tree visibility (default to visible)
    bool showTree = m_settings.value("view/showFileTree", true).toBool();
    if (m_showFileTreeAction) {
        m_showFileTreeAction->setChecked(showTree);
    }
    if (m_fileTree) {
        m_fileTree->setVisible(showTree);
    }
    
    // Show/hide welcome label based on whether folder is loaded
    if (m_fileTreeWelcome) {
        m_fileTreeWelcome->setVisible(m_currentFolder.isEmpty());
    }

    // Restore minimap action state (but keep it hidden on welcome page until a file is loaded)
    bool showMinimap = m_settings.value("view/showMinimap", false).toBool();
    if (m_showMinimapAction) {
        m_showMinimapAction->blockSignals(true);
        m_showMinimapAction->setChecked(showMinimap);
        m_showMinimapAction->blockSignals(false);
    }
    // Minimap stays hidden on welcome page - will be shown when a file is loaded in loadFile()

    // Restore word wrap setting (default: enabled)
    bool wordWrap = m_settings.value("view/wordWrap", true).toBool();
    if (m_wordWrapAction) {
        m_wordWrapAction->blockSignals(true);
        m_wordWrapAction->setChecked(wordWrap);
        m_wordWrapAction->blockSignals(false);
    }
    // Update status bar button to match restored setting
    if (m_statusWrapBtn) {
        m_statusWrapBtn->setChecked(wordWrap);
        m_statusWrapBtn->setIcon(QIcon(wordWrap ? ":/images/text-wrap.png" : ":/images/text-wrap-disabled.png"));
    }

    // Setup file watcher for auto-reload
    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onFileChanged);

    // Setup debounce timer for external file change notifications
    m_fileChangeDebounceTimer = new QTimer(this);
    m_fileChangeDebounceTimer->setSingleShot(true);
    m_fileChangeDebounceTimer->setInterval(500); // 500ms debounce
    connect(m_fileChangeDebounceTimer, &QTimer::timeout, this, &MainWindow::onFileChangeDebounceTriggered);
}

void MainWindow::setupUI()
{
    m_splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_splitter);

    setupFileTree();
    setupEditor();
}

void MainWindow::setupFileTree()
{
    // Create the file tree
    m_fileTree = new QTreeView(this);
    m_fileTree->setSortingEnabled(true);
    m_fileTree->setAnimated(true);
    m_fileTree->setMinimumWidth(200);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setNameFilters(QStringList() << "*.md" << "*.markdown" << "*.mdx" << "*.txt");
    m_proxyModel->setSourceModel(m_fileModel);

    connect(m_fileTree, &QTreeView::clicked, this, &MainWindow::onFileTreeClicked);
    connect(m_fileTree, &QTreeView::activated, this, &MainWindow::onFileTreeClicked);
    m_fileTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_fileTree, &QTreeView::customContextMenuRequested, this, &MainWindow::onFileTreeContextMenu);

    // Create welcome page as a child of the tree view (shown when tree is empty)
    m_fileTreeWelcome = new QLabel(m_fileTree->viewport());
    m_fileTreeWelcome->setObjectName("fileTreeWelcome");
    m_fileTreeWelcome->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_fileTreeWelcome->setWordWrap(true);
    
    // Build rich text welcome content
    QString welcomeText = tr(
        "<center>"
        "<div style='margin-bottom: 20px;'><img src=':/images/folder-open.png' width='48' height='48'></div>"
        "<div style='font-size: 16px; font-weight: bold; margin-bottom: 12px;'>No folder opened</div>"
        "<div style='font-size: 13px; color: palette(text); margin-bottom: 20px;'>"
        "Open a folder to browse your<br>markdown files here"
        "</div>"
        "<div style='font-size: 12px; color: palette(highlight);'>"
        "Ctrl+Shift+O to open folder"
        "</div>"
        "</center>"
    );
    m_fileTreeWelcome->setText(welcomeText);
    m_fileTreeWelcome->setTextFormat(Qt::RichText);
    m_fileTreeWelcome->setStyleSheet("background: palette(base); padding: 20px;");
    m_fileTreeWelcome->show();
    
    // Install event filter on viewport to handle resize events
    m_fileTree->viewport()->installEventFilter(this);
    
    // Delay geometry update until viewport has proper size
    QTimer::singleShot(0, this, [this]() {
        if (m_fileTreeWelcome && m_fileTreeWelcome->isVisible()) {
            m_fileTreeWelcome->setGeometry(m_fileTree->viewport()->rect());
        }
    });

    m_splitter->addWidget(m_fileTree);
}

void MainWindow::setupEditor()
{
    m_editor = new QTextEdit(this);
    m_editor->setReadOnly(true);
    m_editor->document()->setDocumentMargin(16);
    m_editor->setAcceptRichText(true);
    m_editor->setMouseTracking(true);
    m_editor->viewport()->setMouseTracking(true);
    m_editor->viewport()->installEventFilter(this);

    // Set up custom context menu for code block copying
    m_editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_editor, &QTextEdit::customContextMenuRequested, this, &MainWindow::onEditorContextMenu);

    // Apply word wrap setting (default: enabled)
    bool wordWrap = m_settings.value("view/wordWrap", true).toBool();
    m_editor->setLineWrapMode(wordWrap ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);

    applyEditorFont();

    // Use default Qt markdown styling (no custom CSS)

    // Create container for editor + minimap
    m_editorContainer = new QWidget(this);
    QHBoxLayout *editorLayout = new QHBoxLayout(m_editorContainer);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    
    // Add editor
    editorLayout->addWidget(m_editor, 1);
    
    // Add minimap (initially hidden)
    m_minimap = new Minimap(m_editor, m_editorContainer);
    m_minimap->setVisible(false);
    editorLayout->addWidget(m_minimap);
    
    // Use default Qt markdown styling (no custom CSS)

    m_splitter->addWidget(m_editorContainer);
    m_splitter->setStretchFactor(1, 1);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File menu
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));

    QAction *openFileAction = new QAction(QIcon(":/images/file-open.png"), tr("&Open File..."), this);
    openFileAction->setShortcut(QKeySequence::Open);
    connect(openFileAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    fileMenu->addAction(openFileAction);

    QAction *openFolderAction = new QAction(QIcon(":/images/folder-open.png"), tr("Open &Folder..."), this);
    openFolderAction->setShortcut(QKeySequence(tr("Ctrl+Shift+O")));
    connect(openFolderAction, &QAction::triggered, this, &MainWindow::onOpenFolder);
    fileMenu->addAction(openFolderAction);

    m_closeFileAction = new QAction(QIcon(":/images/file-x.png"), tr("&Close File"), this);
    m_closeFileAction->setShortcut(QKeySequence(tr("Ctrl+F4")));
    m_closeFileAction->setEnabled(false); // Disabled on welcome page
    connect(m_closeFileAction, &QAction::triggered, this, &MainWindow::onCloseFile);
    fileMenu->addAction(m_closeFileAction);

    fileMenu->addSeparator();

    m_printAction = new QAction(QIcon(":/images/printer.png"), tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    m_printAction->setEnabled(false); // Disabled on welcome page
    connect(m_printAction, &QAction::triggered, this, &MainWindow::onPrint);
    fileMenu->addAction(m_printAction);

    m_exportPdfAction = new QAction(QIcon(":/images/file-type-pdf.png"), tr("&Export to PDF..."), this);
    m_exportPdfAction->setShortcut(QKeySequence(tr("Ctrl+Shift+P")));
    m_exportPdfAction->setEnabled(false); // Disabled on welcome page
    connect(m_exportPdfAction, &QAction::triggered, this, &MainWindow::onExportToPdf);
    fileMenu->addAction(m_exportPdfAction);

    // Export to HTML submenu
    m_exportHtmlMenu = fileMenu->addMenu(QIcon(":/images/file-type-html.png"), tr("Export to &HTML"));
    m_exportHtmlMenu->setEnabled(false); // Disabled on welcome page
    
    QAction *exportSimpleHtmlAction = new QAction(tr("&Simple HTML..."), this);
    exportSimpleHtmlAction->setShortcut(QKeySequence(tr("Ctrl+Shift+H")));
    exportSimpleHtmlAction->setEnabled(false);
    connect(exportSimpleHtmlAction, &QAction::triggered, this, &MainWindow::onExportSimpleHtml);
    m_exportHtmlMenu->addAction(exportSimpleHtmlAction);
    
    QAction *exportSelfContainedHtmlAction = new QAction(tr("&Self-contained HTML..."), this);
    exportSelfContainedHtmlAction->setEnabled(false);
    connect(exportSelfContainedHtmlAction, &QAction::triggered, this, &MainWindow::onExportSelfContainedHtml);
    m_exportHtmlMenu->addAction(exportSelfContainedHtmlAction);
    
    // Store references for enable/disable
    m_exportHtmlSimpleAction = exportSimpleHtmlAction;
    m_exportHtmlSelfContainedAction = exportSelfContainedHtmlAction;

    fileMenu->addSeparator();

    m_recentMenu = fileMenu->addMenu(tr("Recent &Files"));
    m_clearRecentAction = new QAction(tr("&Clear Recent Files"), this);
    connect(m_clearRecentAction, &QAction::triggered, this, &MainWindow::onClearRecent);

    m_recentFoldersMenu = fileMenu->addMenu(tr("Recent F&olders"));
    m_clearRecentFoldersAction = new QAction(tr("&Clear Recent Folders"), this);
    connect(m_clearRecentFoldersAction, &QAction::triggered, this, &MainWindow::onClearRecentFolders);

    fileMenu->addSeparator();

    QAction *exitAction = new QAction(QIcon(":/images/exit.png"), tr("E&xit"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    // View menu
    QMenu *viewMenu = menuBar->addMenu(tr("&View"));

    QAction *zoomInAction = new QAction(QIcon(":/images/zoom-in.png"), tr("Zoom &In"), this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::onZoomIn);
    viewMenu->addAction(zoomInAction);

    QAction *zoomOutAction = new QAction(QIcon(":/images/zoom-out.png"), tr("Zoom &Out"), this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::onZoomOut);
    viewMenu->addAction(zoomOutAction);

    QAction *zoomResetAction = new QAction(QIcon(":/images/zoom-reset.png"), tr("&Reset Zoom"), this);
    zoomResetAction->setShortcut(QKeySequence(tr("Ctrl+0")));
    connect(zoomResetAction, &QAction::triggered, this, &MainWindow::onZoomReset);
    viewMenu->addAction(zoomResetAction);

    viewMenu->addSeparator();

    m_reloadAction = new QAction(QIcon(":/images/reload.png"), tr("&Reload"), this);
    m_reloadAction->setShortcut(QKeySequence::Refresh);
    m_reloadAction->setEnabled(false); // Disabled on welcome page
    connect(m_reloadAction, &QAction::triggered, this, &MainWindow::onReload);
    viewMenu->addAction(m_reloadAction);

    viewMenu->addSeparator();

    m_showFileTreeAction = new QAction(tr("Show &File Tree"), this);
    m_showFileTreeAction->setShortcut(QKeySequence(tr("F9")));
    m_showFileTreeAction->setCheckable(true);
    m_showFileTreeAction->setChecked(true);
    connect(m_showFileTreeAction, &QAction::toggled, this, &MainWindow::onToggleFileTree);
    viewMenu->addAction(m_showFileTreeAction);

    m_showMinimapAction = new QAction(tr("Show &Minimap"), this);
    m_showMinimapAction->setShortcut(QKeySequence(tr("F10")));
    m_showMinimapAction->setCheckable(true);
    m_showMinimapAction->setChecked(false);
    connect(m_showMinimapAction, &QAction::toggled, this, &MainWindow::onToggleMinimap);
    viewMenu->addAction(m_showMinimapAction);

    m_wordWrapAction = new QAction(tr("&Word Wrap"), this);
    m_wordWrapAction->setShortcut(QKeySequence(tr("Ctrl+W")));
    m_wordWrapAction->setCheckable(true);
    connect(m_wordWrapAction, &QAction::toggled, this, &MainWindow::onToggleWordWrap);
    viewMenu->addAction(m_wordWrapAction);

    viewMenu->addSeparator();

    m_findAction = new QAction(QIcon(":/images/binoculars.png"), tr("&Find..."), this);
    m_findAction->setShortcut(QKeySequence::Find);
    m_findAction->setEnabled(false); // Disabled on welcome page
    connect(m_findAction, &QAction::triggered, this, &MainWindow::onFind);
    viewMenu->addAction(m_findAction);

    m_findNextAction = new QAction(QIcon(":/images/arrow-right.png"), tr("Find &Next"), this);
    m_findNextAction->setShortcut(QKeySequence(tr("F3")));
    m_findNextAction->setEnabled(false); // Disabled on welcome page
    connect(m_findNextAction, &QAction::triggered, this, &MainWindow::onFindNext);
    viewMenu->addAction(m_findNextAction);

    QAction *searchInFilesAction = new QAction(QIcon(":/images/file-search.png"), tr("Search in &Files..."), this);
    searchInFilesAction->setShortcut(QKeySequence(tr("Ctrl+Shift+F")));
    connect(searchInFilesAction, &QAction::triggered, this, &MainWindow::onSearchInFiles);
    viewMenu->addAction(searchInFilesAction);

    viewMenu->addSeparator();

    QAction *prefsAction = new QAction(QIcon(":/images/settings.png"), tr("&Preferences..."), this);
    prefsAction->setShortcut(QKeySequence(tr("Ctrl+,")));
    connect(prefsAction, &QAction::triggered, this, &MainWindow::onPreferences);
    viewMenu->addAction(prefsAction);

    // Help menu
    QMenu *helpMenu = menuBar->addMenu(tr("&Help"));

    QAction *aboutAction = new QAction(QIcon(":/images/about.png"), tr("&About"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setObjectName("Main Toolbar");

    QAction *openAction = new QAction(QIcon(":/images/file-open.png"), tr("Open File"), this);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    toolBar->addAction(openAction);

    QAction *openFolderAction = new QAction(QIcon(":/images/folder-open.png"), tr("Open Folder"), this);
    connect(openFolderAction, &QAction::triggered, this, &MainWindow::onOpenFolder);
    toolBar->addAction(openFolderAction);

    // Print action is created in setupMenuBar and stored in m_printAction
    toolBar->addAction(m_printAction);

    // Export to PDF action is created in setupMenuBar and stored in m_exportPdfAction
    toolBar->addAction(m_exportPdfAction);

    toolBar->addSeparator();

    toolBar->addAction(m_reloadAction);

    toolBar->addSeparator();

    QAction *zoomInAction = new QAction(QIcon(":/images/zoom-in.png"), tr("Zoom In"), this);
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::onZoomIn);
    toolBar->addAction(zoomInAction);

    QAction *zoomOutAction = new QAction(QIcon(":/images/zoom-out.png"), tr("Zoom Out"), this);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::onZoomOut);
    toolBar->addAction(zoomOutAction);

    toolBar->addSeparator();

    QAction *findAction = new QAction(QIcon(":/images/binoculars.png"), tr("Find"), this);
    connect(findAction, &QAction::triggered, this, &MainWindow::onFind);
    toolBar->addAction(findAction);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *status = statusBar();

    // Left side (temporary widget area)
    // File Tree toggle button (icon only)
    m_toggleFileTreeBtn = new QPushButton(this);
    m_toggleFileTreeBtn->setIcon(QIcon(":/images/layout-sidebar.png"));
    m_toggleFileTreeBtn->setIconSize(QSize(20, 20));
    m_toggleFileTreeBtn->setFlat(true);
    m_toggleFileTreeBtn->setFixedSize(24, 24);
    m_toggleFileTreeBtn->setStyleSheet(QStringLiteral("QPushButton { padding-left: 2px; padding-top: 2px; padding-bottom: 2px; padding-right: 4px; margin: 0px; }"));
    m_toggleFileTreeBtn->setCheckable(true);
    m_toggleFileTreeBtn->setChecked(m_settings.value("view/showFileTree", true).toBool());
    m_toggleFileTreeBtn->setToolTip(tr("Toggle file tree sidebar"));
    connect(m_toggleFileTreeBtn, &QPushButton::toggled, this, &MainWindow::onToggleFileTree);
    status->addWidget(m_toggleFileTreeBtn);

    // Progress bar for folder loading (hidden by default, left side)
    m_statusProgress = new QProgressBar(this);
    m_statusProgress->setMaximumWidth(80);
    m_statusProgress->setMaximumHeight(14);
    m_statusProgress->setTextVisible(false);
    m_statusProgress->setRange(0, 0);  // Indeterminate mode
    m_statusProgress->hide();
    status->addWidget(m_statusProgress);

    // File loaded message (next to Tree button, auto-clears after 3 sec)
    m_statusFileMsg = new QLabel(this);
    m_statusFileMsg->setMinimumWidth(1);  // Start small, expand when text is set
    status->addWidget(m_statusFileMsg, 1);  // Stretch factor to take available space

    // Timer to clear the file message
    m_statusMsgTimer = new QTimer(this);
    m_statusMsgTimer->setSingleShot(true);
    connect(m_statusMsgTimer, &QTimer::timeout, [this]() {
        if (m_statusFileMsg) {
            m_statusFileMsg->hide();  // Hide completely when empty (removes separator)
        }
    });

    // Right side (permanent widgets) - Order: [Word Count] [Zoom] [File Type] [Encoding] [Minimap Toggle]
    // Style with asymmetric padding - extra 2px on right to center text better
    QString labelStyle = QStringLiteral("QLabel { padding-left: 8px; padding-right: 10px; }");

    // Word count
    m_statusWordCount = new QLabel(tr("Words: 0"), this);
    m_statusWordCount->setStyleSheet(labelStyle);
    m_statusWordCount->setAlignment(Qt::AlignCenter);
    m_statusWordCount->setToolTip(tr("Word count"));
    status->addPermanentWidget(m_statusWordCount);

    // Zoom level
    m_statusZoom = new QLabel(tr("100%"), this);
    m_statusZoom->setStyleSheet(labelStyle);
    m_statusZoom->setAlignment(Qt::AlignCenter);
    m_statusZoom->setToolTip(tr("Zoom level"));
    status->addPermanentWidget(m_statusZoom);

    // File type
    m_statusFileType = new QLabel(tr("Ready"), this);
    m_statusFileType->setStyleSheet(labelStyle);
    m_statusFileType->setAlignment(Qt::AlignCenter);
    status->addPermanentWidget(m_statusFileType);

    // Encoding
    m_statusEncoding = new QLabel(tr("UTF-8"), this);
    m_statusEncoding->setStyleSheet(labelStyle);
    m_statusEncoding->setAlignment(Qt::AlignCenter);
    m_statusEncoding->setToolTip(tr("File encoding"));
    status->addPermanentWidget(m_statusEncoding);

    // Line endings
    m_statusLineEndings = new QLabel(tr("LF"), this);
    m_statusLineEndings->setStyleSheet(labelStyle);
    m_statusLineEndings->setAlignment(Qt::AlignCenter);
    m_statusLineEndings->setToolTip(tr("Line endings (LF or CRLF)"));
    status->addPermanentWidget(m_statusLineEndings);

    // Word wrap toggle button (icon only)
    m_statusWrapBtn = new QPushButton(this);
    bool wordWrapDefault = m_settings.value("view/wordWrap", true).toBool();
    m_statusWrapBtn->setIcon(QIcon(wordWrapDefault ? ":/images/text-wrap.png" : ":/images/text-wrap-disabled.png"));
    m_statusWrapBtn->setIconSize(QSize(20, 20));
    m_statusWrapBtn->setFlat(true);
    m_statusWrapBtn->setFixedSize(24, 24);
    m_statusWrapBtn->setStyleSheet(QStringLiteral(
        "QPushButton { padding-left: 2px; padding-top: 2px; padding-bottom: 2px; padding-right: 4px; margin: 0px; border: none; background: transparent; }"
        "QPushButton:checked { background: transparent; }"));
    m_statusWrapBtn->setCheckable(true);
    m_statusWrapBtn->setChecked(wordWrapDefault);
    m_statusWrapBtn->setToolTip(tr("Toggle word wrap"));
    // Initially disabled on welcome page - will be enabled when file is loaded
    m_statusWrapBtn->setEnabled(!m_currentFile.isEmpty());
    connect(m_statusWrapBtn, &QPushButton::toggled, this, &MainWindow::onToggleWordWrap);
    status->addPermanentWidget(m_statusWrapBtn);

    // Minimap toggle button (icon only, rightmost)
    m_toggleMinimapBtn = new QPushButton(this);
    m_toggleMinimapBtn->setIcon(QIcon(":/images/layout-sidebar-right.png"));
    m_toggleMinimapBtn->setIconSize(QSize(20, 20));
    m_toggleMinimapBtn->setFlat(true);
    m_toggleMinimapBtn->setFixedSize(24, 24);
    m_toggleMinimapBtn->setStyleSheet(QStringLiteral("QPushButton { padding-left: 2px; padding-top: 2px; padding-bottom: 2px; padding-right: 4px; margin: 0px; }"));
    m_toggleMinimapBtn->setCheckable(true);
    m_toggleMinimapBtn->setChecked(m_settings.value("view/showMinimap", false).toBool());
    m_toggleMinimapBtn->setToolTip(tr("Toggle minimap"));
    // Initially disabled on welcome page - will be enabled when file is loaded
    m_toggleMinimapBtn->setEnabled(!m_currentFile.isEmpty());
    connect(m_toggleMinimapBtn, &QPushButton::toggled, this, &MainWindow::onToggleMinimap);
    status->addPermanentWidget(m_toggleMinimapBtn);
}

void MainWindow::updateStatusBar()
{
    if (m_currentFile.isEmpty()) {
        // Welcome page state - show only "Ready" in file type, hide others
        if (m_statusFileType) {
            m_statusFileType->setText(tr("Ready"));
            m_statusFileType->setVisible(true);
            m_statusFileType->setToolTip(QString());  // No tooltip on welcome page
        }
        if (m_statusEncoding) m_statusEncoding->setVisible(false);
        if (m_statusLineEndings) m_statusLineEndings->setVisible(false);
        if (m_statusWordCount) m_statusWordCount->setVisible(false);
        if (m_statusZoom) m_statusZoom->setVisible(false);
        if (m_toggleMinimapBtn) {
            m_toggleMinimapBtn->setVisible(false);
            m_toggleMinimapBtn->setEnabled(false);
        }
        if (m_statusWrapBtn) {
            m_statusWrapBtn->setVisible(false);
            m_statusWrapBtn->setEnabled(false);
        }
        return;
    }

    // File loaded - show all status items
    m_statusFileType->setVisible(true);
    m_statusFileType->setToolTip(tr("File type"));
    m_statusEncoding->setVisible(true);
    m_statusLineEndings->setVisible(true);
    m_statusWordCount->setVisible(true);
    m_statusZoom->setVisible(true);
    // Enable minimap and word wrap toggles when file is loaded
    if (m_toggleMinimapBtn) {
        m_toggleMinimapBtn->setVisible(true);
        m_toggleMinimapBtn->setEnabled(true);
    }
    if (m_statusWrapBtn) {
        m_statusWrapBtn->setVisible(true);
        m_statusWrapBtn->setEnabled(true);
    }

    // Determine file type
    QString ext = QFileInfo(m_currentFile).suffix().toLower();
    QString fileType;
    if (ext == "md" || ext == "markdown") {
        fileType = tr("Markdown");
    } else if (ext == "mdx") {
        fileType = tr("MDX");
    } else if (ext == "txt") {
        fileType = tr("Plain Text");
    } else {
        fileType = ext.toUpper();
    }
    m_statusFileType->setText(fileType);

    // Encoding (detected during file load)
    m_statusEncoding->setText(m_detectedEncoding);

    // Line endings (detected during file load)
    m_statusLineEndings->setText(m_detectedLineEndings);

    // Word count - use plain text for accurate count
    QString plainText = m_editor->toPlainText();
    int words = countWords(plainText);
    m_statusWordCount->setText(tr("Words: %1").arg(words));
}

int MainWindow::countWords(const QString &text) const
{
    if (text.isEmpty())
        return 0;

    // Simple word counting: split on whitespace
    // This is fast and accurate enough for Markdown
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return 0;

    // Split on whitespace sequences
    QStringList words = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    return words.size();
}

void MainWindow::highlightSearchText(const QString &text)
{
    if (text.isEmpty() || !m_editor)
        return;

    // Use QTextEdit's find() to locate and select the first occurrence
    // This works with both plain text and markdown rendered content
    if (m_editor->find(text)) {
        // Found at least one occurrence - QTextEdit will select it
        // The selection will be visible with the default selection colors
        // (usually blue highlight with white text on most themes)
        
        // Ensure the selected text is visible by scrolling to it
        m_editor->ensureCursorVisible();
        
        // Set focus so the selection is shown in color (not grey)
        m_editor->setFocus();
    }
}

void MainWindow::onOpenFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open Markdown File"), QString(),
        tr("Markdown Files (*.md *.markdown *.mdx *.txt);;All Files (*.*)"));

    if (!filePath.isEmpty()) {
        openFile(filePath);
    }
}

void MainWindow::onOpenFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(
        this, tr("Open Folder"), m_settings.value("lastFolder").toString());

    if (!folderPath.isEmpty()) {
        loadFolder(folderPath);
    }
}

void MainWindow::onFileTreeClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    QString filePath = m_fileModel->filePath(sourceIndex);
    QFileInfo info(filePath);

    if (info.isFile() && isMarkdownFile(filePath)) {
        loadFile(filePath);
        if (m_settings.value("privacy/keepRecentFiles", true).toBool()) {
            updateRecentFiles(filePath);
        }
    }
}

void MainWindow::onFileTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = m_fileTree->indexAt(pos);
    if (!index.isValid()) return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    QString filePath = m_fileModel->filePath(sourceIndex);
    QFileInfo info(filePath);

    if (!info.isFile() || !isMarkdownFile(filePath)) return;

    QMenu contextMenu(this);

    // Add external editor action if configured
    QString externalEditor = m_settings.value("editor/externalEditor").toString();
    if (!externalEditor.isEmpty()) {
        QAction *openWithEditorAction = contextMenu.addAction(QIcon(":/images/edit.png"), tr("Open with External Editor"));
        connect(openWithEditorAction, &QAction::triggered, this, [this, filePath]() {
            m_settings.setValue("temp/externalEditorFile", filePath);
            onOpenWithExternalEditor();
        });
        contextMenu.addSeparator();
    }

#ifdef Q_OS_LINUX
    QAction *revealAction = contextMenu.addAction(QIcon(":/images/external-link.png"), tr("Show in File Manager"));
    connect(revealAction, &QAction::triggered, this, [filePath]() {
        QProcess::startDetached("xdg-open", QStringList() << QFileInfo(filePath).absolutePath());
    });
#else
    QAction *revealAction = contextMenu.addAction(QIcon(":/images/external-link.png"), tr("Show in Explorer"));
    connect(revealAction, &QAction::triggered, this, [filePath]() {
        QString path = QDir::toNativeSeparators(filePath);
        QProcess::startDetached("explorer", QStringList() << "/select," << path);
    });
#endif

    contextMenu.exec(m_fileTree->viewport()->mapToGlobal(pos));
}

void MainWindow::onEditorContextMenu(const QPoint &pos)
{
    if (!m_editor || m_currentFile.isEmpty()) return;

    // Get cursor at click position to check formatting
    QTextCursor cursor = m_editor->cursorForPosition(pos);
    int cursorPos = cursor.position();

    // Check if cursor is in code by looking at surrounding characters
    // We scan a small window around the click position to find monospace text
    QString text = m_editor->toPlainText();
    bool inCode = false;
    int codeStart = -1;
    int codeEnd = -1;

    // Scan up to 5 characters in each direction from click position
    for (int offset = 0; offset <= 5; offset++) {
        // Check at click position ± offset
        for (int direction : {0, -1, 1}) {
            int checkPos = cursorPos + (offset * direction);
            if (checkPos < 0 || checkPos >= text.length()) continue;

            // Get format of character AT checkPos (not before)
            QTextCursor checkCursor(m_editor->document());
            checkCursor.setPosition(checkPos);
            // For a cursor at position P, charFormat() returns format of character at P-1 (if at block start, returns block char format)
            // To get format at position P, we need to select the character at P
            if (checkPos < text.length()) {
                checkCursor.setPosition(checkPos);
                checkCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
            }
            QTextCharFormat checkFormat = checkCursor.charFormat();
            QString checkFont = checkFormat.font().family();

            if (checkFont.contains("Mono", Qt::CaseInsensitive) ||
                checkFont.contains("Consolas", Qt::CaseInsensitive) ||
                checkFont.contains("Courier", Qt::CaseInsensitive)) {
                // Found monospace text, now expand to find full boundaries
                inCode = true;

                // Find start by going backward - check each character individually
                codeStart = checkPos;
                while (codeStart > 0) {
                    QTextCursor startCursor(m_editor->document());
                    startCursor.setPosition(codeStart - 1);
                    startCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                    QTextCharFormat startFormat = startCursor.charFormat();
                    QString startFont = startFormat.font().family();
                    if (!(startFont.contains("Mono", Qt::CaseInsensitive) ||
                          startFont.contains("Consolas", Qt::CaseInsensitive) ||
                          startFont.contains("Courier", Qt::CaseInsensitive))) {
                        break;
                    }
                    codeStart--;
                }

                // Find end by going forward - check each character individually
                codeEnd = checkPos + 1;
                while (codeEnd < text.length()) {
                    QTextCursor endCursor(m_editor->document());
                    endCursor.setPosition(codeEnd);
                    endCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                    QTextCharFormat endFormat = endCursor.charFormat();
                    QString endFont = endFormat.font().family();
                    if (!(endFont.contains("Mono", Qt::CaseInsensitive) ||
                          endFont.contains("Consolas", Qt::CaseInsensitive) ||
                          endFont.contains("Courier", Qt::CaseInsensitive))) {
                        break;
                    }
                    codeEnd++;
                }

                // We found and bounded the code
                break;
            }
        }
        if (inCode) break;
    }

    // Create custom context menu (not using standard one so we can control icons)
    QMenu menu(this);

    // Add "Copy Code" action at the top if in code (inline or block)
    if (inCode && codeStart >= 0 && codeEnd > codeStart) {
        QString codeContent = text.mid(codeStart, codeEnd - codeStart);
        // Trim leading/trailing whitespace but preserve internal formatting
        codeContent = codeContent.trimmed();
        if (!codeContent.isEmpty()) {
            QAction *copyCodeAction = menu.addAction(QIcon(":/images/codeblock.png"), tr("Copy Code"));
            connect(copyCodeAction, &QAction::triggered, this, [codeContent]() {
                QClipboard *clipboard = QApplication::clipboard();
                clipboard->setText(codeContent);
            });
            menu.addSeparator();
        }
    }

    // Add Copy and Select All actions with custom icons (read-only, so no Cut/Paste)
    bool hasSelection = m_editor->textCursor().hasSelection();

    QAction *copyAction = menu.addAction(QIcon(":/images/copy.png"), tr("Copy"));
    copyAction->setEnabled(hasSelection);
    connect(copyAction, &QAction::triggered, m_editor, &QTextEdit::copy);

    menu.addSeparator();

    QAction *selectAllAction = menu.addAction(QIcon(":/images/select-all.png"), tr("Select All"));
    connect(selectAllAction, &QAction::triggered, m_editor, &QTextEdit::selectAll);

    // Show the menu
    menu.exec(m_editor->viewport()->mapToGlobal(pos));
}



void MainWindow::onOpenWithExternalEditor()
{
    QString externalEditor = m_settings.value("editor/externalEditor").toString();
    QString filePath = m_settings.value("temp/externalEditorFile").toString();
    m_settings.remove("temp/externalEditorFile");

    if (externalEditor.isEmpty() || filePath.isEmpty()) return;

    QFileInfo editorInfo(externalEditor);
    if (!editorInfo.exists() || !editorInfo.isExecutable()) {
        QMessageBox::warning(this, tr("External Editor Not Found"),
                             tr("The configured external editor was not found or is not executable:\n%1").arg(externalEditor));
        return;
    }

    if (!QProcess::startDetached(externalEditor, QStringList() << filePath)) {
        QMessageBox::warning(this, tr("Failed to Launch"),
                             tr("Failed to launch the external editor.\nEditor: %1\nFile: %2").arg(externalEditor).arg(filePath));
    }
}

void MainWindow::onRecentFileTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        QString filePath = action->data().toString();
        if (QFile::exists(filePath)) {
            openFile(filePath);
        } else {
            QMessageBox::warning(this, tr("File Not Found"),
                                 tr("The file no longer exists:\n%1").arg(filePath));
            updateRecentFiles(QString()); // Refresh to remove missing file
        }
    }
}

void MainWindow::onRecentFolderTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        QString folderPath = action->data().toString();
        if (QDir(folderPath).exists()) {
            loadFolder(folderPath);
        } else {
            QMessageBox::warning(this, tr("Folder Not Found"),
                                 tr("The folder no longer exists:\n%1").arg(folderPath));
            updateRecentFolders(QString()); // Refresh to remove missing folder
        }
    }
}

void MainWindow::onClearRecent()
{
    m_settings.setValue("recentFiles", QStringList());
    refreshRecentFilesMenu();
}

void MainWindow::onClearRecentFolders()
{
    m_settings.setValue("recentFolders", QStringList());
    refreshRecentFoldersMenu();
}

void MainWindow::onZoomIn()
{
    m_editor->zoomIn(2);
    m_zoomLevel = qMin(m_zoomLevel + 10, 500); // Cap at 500%
    m_statusZoom->setText(QStringLiteral("%1%").arg(m_zoomLevel));
    if (m_minimap) {
        m_minimap->updateContent();
    }
}

void MainWindow::onZoomOut()
{
    m_editor->zoomOut(2);
    m_zoomLevel = qMax(m_zoomLevel - 10, 25); // Min at 25%
    m_statusZoom->setText(QStringLiteral("%1%").arg(m_zoomLevel));
    if (m_minimap) {
        m_minimap->updateContent();
    }
}

void MainWindow::onZoomReset()
{
    applyEditorFont();
    m_zoomLevel = 100;
    m_statusZoom->setText(QStringLiteral("100%"));
    if (m_minimap) {
        m_minimap->updateContent();
    }
}

void MainWindow::updateZoomDisplay()
{
    // Calculate zoom percentage from current font size vs base font size
    int currentSize = m_editor->font().pointSize();
    if (m_baseFontSize > 0) {
        m_zoomLevel = (currentSize * 100) / m_baseFontSize;
        m_statusZoom->setText(QStringLiteral("%1%").arg(m_zoomLevel));
    }
}

void MainWindow::onToggleFileTree(bool visible)
{
    if (m_fileTree) {
        m_fileTree->setVisible(visible);
        m_settings.setValue("view/showFileTree", visible);
    }
    // Sync status bar button with menu action (avoid infinite loop by checking first)
    if (m_toggleFileTreeBtn && m_toggleFileTreeBtn->isChecked() != visible) {
        m_toggleFileTreeBtn->setChecked(visible);
    }
    if (m_showFileTreeAction && m_showFileTreeAction->isChecked() != visible) {
        m_showFileTreeAction->setChecked(visible);
    }
}

void MainWindow::onToggleMinimap(bool visible)
{
    if (m_minimap) {
        m_minimap->setVisible(visible);
        m_settings.setValue("view/showMinimap", visible);
        if (visible) {
            m_minimap->updateContent();
        }
    }
    // Sync status bar button with menu action (avoid infinite loop by checking first)
    if (m_toggleMinimapBtn && m_toggleMinimapBtn->isChecked() != visible) {
        m_toggleMinimapBtn->setChecked(visible);
    }
    if (m_showMinimapAction && m_showMinimapAction->isChecked() != visible) {
        m_showMinimapAction->setChecked(visible);
    }
}

void MainWindow::onToggleWordWrap(bool enabled)
{
    if (m_editor) {
        m_editor->setLineWrapMode(enabled ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
    }
    m_settings.setValue("view/wordWrap", enabled);
    // Sync menu action with button (avoid infinite loop by checking first)
    if (m_wordWrapAction && m_wordWrapAction->isChecked() != enabled) {
        m_wordWrapAction->setChecked(enabled);
    }
    if (m_statusWrapBtn) {
        if (m_statusWrapBtn->isChecked() != enabled) {
            m_statusWrapBtn->setChecked(enabled);
        }
        // Update icon based on state
        m_statusWrapBtn->setIcon(QIcon(enabled ? ":/images/text-wrap.png" : ":/images/text-wrap-disabled.png"));
    }
}

void MainWindow::onAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::onPreferences()
{
    PreferencesDialog dlg(this);
    dlg.loadSettings();
    if (dlg.exec() == QDialog::Accepted) {
        dlg.saveSettings();
        applyEditorFont();
        // Use default Qt markdown styling (no custom CSS)
        if (!dlg.keepRecentFiles()) {
            m_settings.setValue("recentFiles", QStringList());
        }
        if (!dlg.keepRecentFolders()) {
            m_settings.setValue("recentFolders", QStringList());
        }
        refreshRecentFilesMenu();
        refreshRecentFoldersMenu();
        if (!m_currentFile.isEmpty()) {
            loadFile(m_currentFile);
        }
    }
}

void MainWindow::onFind()
{
    if (!m_findDialog) {
        m_findDialog = new FindDialog(m_editor, this);
        // Connect to update last search text for F3 "Find Next" support
        connect(m_findDialog, &FindDialog::searchPerformed, this, [this](const QString &text) {
            m_lastSearchText = text;
        });
    }
    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void MainWindow::onFindNext()
{
    if (!m_editor || m_currentFile.isEmpty() || m_lastSearchText.isEmpty()) {
        return;
    }

    // Try to find next occurrence
    bool found = m_editor->find(m_lastSearchText);

    // If not found, wrap around to beginning
    if (!found) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cursor);

        // Search from beginning
        found = m_editor->find(m_lastSearchText);

        if (found) {
            // Brief feedback that we wrapped around
            if (m_statusFileMsg) {
                m_statusFileMsg->setText(tr("Wrapped to beginning"));
                if (m_statusMsgTimer) {
                    m_statusMsgTimer->start(1500);
                }
            }
        }
    }

    // Set focus so the selection is shown in color (not grey)
    if (found) {
        m_editor->setFocus();
    }
}

void MainWindow::onSearchInFiles()
{
    // Need a folder loaded to search
    if (m_currentFolder.isEmpty()) {
        QMessageBox::information(this, tr("Search in Files"),
                                   tr("Please open a folder first to search through its files."));
        return;
    }

    // Create dialog if not exists, or update folder path if changed
    if (!m_searchInDialog) {
        m_searchInDialog = new SearchInDialog(m_currentFolder, this);
        connect(m_searchInDialog, &SearchInDialog::fileSelected, this, [this](const QString &filePath, const QString &searchText) {
            // Load file first (this may clear m_lastSearchText when switching files)
            loadFile(filePath);
            // Set search text AFTER loadFile so it's available for F3
            m_lastSearchText = searchText;
            // Highlight the first occurrence
            highlightSearchText(searchText);
        });
    } else {
        // Update folder path if it changed
        m_searchInDialog->setFolderPath(m_currentFolder);
    }

    m_searchInDialog->show();
    m_searchInDialog->raise();
    m_searchInDialog->activateWindow();
}

void MainWindow::onFileChanged(const QString &path)
{
    if (path != m_currentFile || !QFile::exists(path)) {
        return;
    }

    // Debounce the file change notification - restart timer if already running
    // Don't start timer if dialog is already open
    if (m_fileChangeDebounceTimer && !m_fileChangeDialogOpen) {
        m_fileChangeDebounceTimer->start();
    }
}

void MainWindow::onFileChangeDebounceTriggered()
{
    if (m_currentFile.isEmpty() || !QFile::exists(m_currentFile) || m_fileChangeDialogOpen) {
        return;
    }

    m_fileChangeDialogOpen = true;  // Mark dialog as open

    // File has been modified externally - ask user if they want to reload
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("File Modified"),
        tr("The file has been modified externally:\n%1\n\nDo you want to reload it?").arg(QFileInfo(m_currentFile).fileName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    m_fileChangeDialogOpen = false;  // Dialog closed

    if (reply == QMessageBox::Yes) {
        loadFile(m_currentFile);
    }
}

void MainWindow::applyEditorFont()
{
#ifdef Q_OS_LINUX
    const QString defaultFontFamily = QStringLiteral("DejaVu Sans");
#else
    const QString defaultFontFamily = QStringLiteral("Segoe UI");
#endif
    QString family = m_settings.value("editor/fontFamily", defaultFontFamily).toString();
    int size = m_settings.value("editor/fontSize", 11).toInt();
    m_baseFontSize = size;  // Store base size for zoom calculations
    QFont font(family);
    font.setPointSize(size);
    m_editor->setFont(font);
}

void MainWindow::showWelcomePage()
{
    m_editor->clear();
    m_currentFile.clear();
    setWindowTitle(tr("PlainMD"));

    // Disable print, export, find, and close file actions on welcome page
    if (m_printAction) {
        m_printAction->setEnabled(false);
    }
    if (m_exportPdfAction) {
        m_exportPdfAction->setEnabled(false);
    }
    if (m_findAction) {
        m_findAction->setEnabled(false);
    }
    if (m_findNextAction) {
        m_findNextAction->setEnabled(false);
    }
    if (m_closeFileAction) {
        m_closeFileAction->setEnabled(false);
    }
    if (m_reloadAction) {
        m_reloadAction->setEnabled(false);
    }

    // Stop watching files when showing welcome page
    if (m_fileWatcher && !m_fileWatcher->files().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->files());
    }

    // Hide minimap and disable toggle on welcome page (no document to show)
    if (m_minimap) {
        m_minimap->hide();
    }
    if (m_showMinimapAction) {
        m_showMinimapAction->setEnabled(false);
    }

#ifdef Q_OS_LINUX
    QString fontFamily = QStringLiteral("'DejaVu Sans', 'Noto Sans', 'Noto Color Emoji', 'Apple Color Emoji', 'Helvetica Neue', Arial, sans-serif");
    QString monoFamily = QStringLiteral("'DejaVu Sans Mono', 'SFMono-Regular', Consolas, 'Liberation Mono', monospace");
#else
    QString fontFamily = QStringLiteral("'Segoe UI', 'Segoe UI Emoji', 'Helvetica Neue', Arial, sans-serif");
    QString monoFamily = QStringLiteral("Consolas, 'SFMono-Regular', 'DejaVu Sans Mono', 'Liberation Mono', monospace");
#endif

    QString html = QStringLiteral(R"(
        <html>
        <head>
        <style>
            body {
                font-family: %1;
                color: #444;
                margin: 16px;
                line-height: 1.3;
            }
            .center { text-align: center; }
            h1 {
                font-size: 2em;
                color: #2c3e50;
                font-weight: 300;
                margin: 0;
                text-align: center;
            }
            h2 {
                font-size: 1em;
                color: #34495e;
                border-bottom: 1px solid #ecf0f1;
                padding-bottom: 2px;
                margin: 10px 0 4px 0;
            }
            .shortcut {
                display: inline-block;
                background: #f4f4f4;
                border: 1px solid #ddd;
                border-radius: 3px;
                padding: 1px 6px;
                font-family: %2;
                font-size: 0.8em;
                color: #555;
            }
            ul { padding-left: 18px; margin: 0; }
            li { margin: 1px 0; }
            a { color: #3498db; text-decoration: none; }
            a:hover { text-decoration: underline; }
            .footer {
                margin-top: 12px;
                font-size: 0.85em;
                color: #95a5a6;
                text-align: center;
            }
        </style>
        </head>
        <body>
            <h1>PlainMD</h1>
            <p align="center" style="margin:0; line-height:1.2;">
                <img src=":/icon_96.png" width="96" height="96" alt="" title=""><br>
                <span style="font-size:1em; color:#7f8c8d;">A simple and elegant Markdown viewer</span><br>
                <span style="font-size:0.9em; color:#95a5a6;">Version %3</span>
            </p>

            <h2>Get Started</h2>
            <ul>
                <li><strong>Open a file</strong> — <span class="shortcut">Ctrl+O</span> or drag &amp; drop</li>
                <li><strong>Open a folder</strong> — <span class="shortcut">Ctrl+Shift+O</span> or drag &amp; drop</li>
                <li><strong>Search</strong> — <span class="shortcut">Ctrl+F</span></li>
                <li><strong>Print</strong> — <span class="shortcut">Ctrl+P</span></li>
                <li><strong>Preferences</strong> — <span class="shortcut">Ctrl+,</span></li>
            </ul>

            <h2>Supported Formats</h2>
            <ul>
                <li>Markdown (<span class="shortcut">.md</span>), MDX (<span class="shortcut">.mdx</span>), Plain text (<span class="shortcut">.txt</span>)</li>
            </ul>

            <h2>Tips</h2>
            <ul>
                <li>Click any file in the sidebar to preview it instantly</li>
                <li>External images can be toggled in <strong>Preferences</strong> for privacy</li>
                <li>Recent files and folders history can also be disabled in <strong>Preferences</strong></li>
            </ul>

        </body>
        </html>
    )").arg(fontFamily, monoFamily, QApplication::applicationVersion());

    m_editor->setHtml(html);

    // Reset status bar for welcome page
    updateStatusBar();
}

void MainWindow::onPrint()
{
    // Disable print when welcome page is shown (no file loaded)
    if (m_currentFile.isEmpty()) {
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Clone the document and apply emoji-friendly font for printing
        QTextDocument printDoc;
        printDoc.setHtml(m_editor->toHtml());

        // Check if Nerd Font is configured for emoji printing
        bool useNerdFont = m_settings.value("editor/useNerdFontForEmoji", false).toBool();
#ifdef Q_OS_LINUX
        QString defaultEmojiFont = QStringLiteral("Noto Sans");
#else
        QString defaultEmojiFont = QStringLiteral("Segoe UI");
#endif
        QString emojiFontFamily = m_settings.value("editor/printEmojiFont", defaultEmojiFont).toString();
        int emojiFontSize = m_settings.value("editor/printEmojiFontSize", 11).toInt();

        QString printCss;
        QFont printFont;

#ifdef Q_OS_LINUX
        QString fallbackEmojiFonts = QStringLiteral("'Noto Sans', 'Noto Color Emoji', sans-serif");
#else
        QString fallbackEmojiFonts = QStringLiteral("'Segoe UI', 'Segoe UI Emoji', 'Segoe UI Symbol', sans-serif");
#endif

        if (useNerdFont) {
            // Use Nerd Font for proper monochrome emoji rendering
            printCss = QStringLiteral(R"(
                body { font-family: '%1', %2; }
                td, th { font-family: '%1', %2; }
            )").arg(emojiFontFamily).arg(fallbackEmojiFonts);
            printFont = QFont(emojiFontFamily, emojiFontSize);
            printFont.setStyleHint(QFont::Monospace);
            printFont.setFamilies(QStringList() << emojiFontFamily << defaultEmojiFont);
        } else {
            // Use standard emoji fonts
            printCss = QStringLiteral(R"(
                body { font-family: %1; }
                td, th { font-family: %1; }
            )").arg(fallbackEmojiFonts);
            printFont = QFont(defaultEmojiFont, 11);
            printFont.setStyleHint(QFont::SansSerif);
            printFont.setFamilies(QStringList() << defaultEmojiFont << "Noto Color Emoji" << "Noto Sans");
        }

        printDoc.setDefaultStyleSheet(printCss);
        printDoc.setDefaultFont(printFont);
        printDoc.print(&printer);
    }
}

void MainWindow::onExportToPdf()
{
    // Disable export when welcome page is shown (no file loaded)
    if (m_currentFile.isEmpty()) {
        return;
    }

    QString defaultFileName = QFileInfo(m_currentFile).baseName() + ".pdf";
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export to PDF"),
                                                    QFileInfo(m_currentFile).dir().absoluteFilePath(defaultFileName),
                                                    tr("PDF Files (*.pdf)"));
    if (filePath.isEmpty()) {
        return;
    }

    // Ensure .pdf extension
    if (!filePath.endsWith(".pdf", Qt::CaseInsensitive)) {
        filePath += ".pdf";
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);

    // Clone the document and apply emoji-friendly font for printing
    QTextDocument printDoc;
    printDoc.setHtml(m_editor->toHtml());

    // Check if Nerd Font is configured for emoji printing
    bool useNerdFont = m_settings.value("editor/useNerdFontForEmoji", false).toBool();
#ifdef Q_OS_LINUX
    QString defaultEmojiFont = QStringLiteral("Noto Sans");
    QString fallbackEmojiFonts = QStringLiteral("'Noto Sans', 'Noto Color Emoji', sans-serif");
#else
    QString defaultEmojiFont = QStringLiteral("Segoe UI");
    QString fallbackEmojiFonts = QStringLiteral("'Segoe UI', 'Segoe UI Emoji', 'Segoe UI Symbol', sans-serif");
#endif
    QString emojiFontFamily = m_settings.value("editor/printEmojiFont", defaultEmojiFont).toString();
    int emojiFontSize = m_settings.value("editor/printEmojiFontSize", 11).toInt();

    QString printCss;
    QFont printFont;

    if (useNerdFont) {
        // Use Nerd Font for proper monochrome emoji rendering
        printCss = QStringLiteral(R"(
            body { font-family: '%1', %2; }
            td, th { font-family: '%1', %2; }
        )").arg(emojiFontFamily).arg(fallbackEmojiFonts);
        printFont = QFont(emojiFontFamily, emojiFontSize);
        printFont.setStyleHint(QFont::Monospace);
        printFont.setFamilies(QStringList() << emojiFontFamily << defaultEmojiFont);
    } else {
        // Use standard emoji fonts
        printCss = QStringLiteral(R"(
            body { font-family: %1; }
            td, th { font-family: %1; }
        )").arg(fallbackEmojiFonts);
        printFont = QFont(defaultEmojiFont, 11);
        printFont.setStyleHint(QFont::SansSerif);
        printFont.setFamilies(QStringList() << defaultEmojiFont << "Noto Color Emoji" << "Noto Sans");
    }

    printDoc.setDefaultStyleSheet(printCss);
    printDoc.setDefaultFont(printFont);
    printDoc.print(&printer);

    // Show success message
    QMessageBox::information(this, tr("Export Successful"),
                             tr("Document exported to:\n%1").arg(QDir::toNativeSeparators(filePath)));
}

void MainWindow::onExportSimpleHtml()
{
    exportToHtml(false); // false = simple HTML
}

void MainWindow::onExportSelfContainedHtml()
{
    exportToHtml(true); // true = self-contained HTML
}

void MainWindow::exportToHtml(bool selfContained)
{
    // Disable export when welcome page is shown (no file loaded)
    if (m_currentFile.isEmpty()) {
        return;
    }
    
    QString dialogTitle = selfContained ? tr("Export Self-contained HTML") : tr("Export Simple HTML");
    QString defaultFileName = QFileInfo(m_currentFile).baseName() + ".html";
    QString filePath = QFileDialog::getSaveFileName(this, dialogTitle,
                                                    QFileInfo(m_currentFile).dir().absoluteFilePath(defaultFileName),
                                                    tr("HTML Files (*.html *.htm)"));
    if (filePath.isEmpty()) {
        return;
    }

    // Ensure .html extension
    if (!filePath.endsWith(".html", Qt::CaseInsensitive) && !filePath.endsWith(".htm", Qt::CaseInsensitive)) {
        filePath += ".html";
    }

    QString htmlContent;
    
    if (selfContained) {
        // Self-contained: embed all images as base64
        htmlContent = generateSelfContainedHtml();
    } else {
        // Simple: convert absolute paths back to relative
        htmlContent = generateSimpleHtml(filePath);
    }

    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Could not write to file:\n%1").arg(filePath));
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << htmlContent;
    file.close();

    // Show success message
    QString formatText = selfContained ? tr("Self-contained HTML") : tr("Simple HTML");
    QMessageBox::information(this, tr("Export Successful"),
                             tr("Document exported as %1 to:\n%2").arg(formatText, QDir::toNativeSeparators(filePath)));
}

QString MainWindow::generateSimpleHtml(const QString &outputFilePath)
{
    // Get the base HTML from the editor
    QString html = m_editor->toHtml();
    
    // Get the output directory for calculating relative paths
    QDir outputDir = QFileInfo(outputFilePath).dir();
    
    // Find all image sources in the HTML (handle both single and double quotes)
    QRegularExpression imgRegex("<img[^>]+src=[\"']([^\"']+)[\"']");
    QRegularExpressionMatchIterator matches = imgRegex.globalMatch(html);
    
    // Collect all replacements first
    QHash<QString, QString> replacements;
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString src = match.captured(1);
        
        // Skip already embedded images (data URIs) and processed ones
        if (src.startsWith("data:") || replacements.contains(src)) {
            continue;
        }
        
        // Convert file:// URL to local path
        QString localPath;
        if (src.startsWith("file:///")) {
            // On Windows: file:///C:/path/image.png -> C:/path/image.png
            // On Linux: file:///home/user/path/image.png -> /home/user/path/image.png
            localPath = src.mid(8); // Remove "file:///" prefix
            localPath = QDir::fromNativeSeparators(localPath);
        } else {
            localPath = src;
        }
        
        // Calculate relative path from output file to image
        QString relativePath = outputDir.relativeFilePath(localPath);
        
        // Normalize to forward slashes for HTML compatibility
        relativePath = relativePath.replace("\\", "/");
        
        // If relative path starts with "../", it's truly relative
        // If it doesn't start with "../" and contains ":", it's absolute (different drive)
        if (!relativePath.startsWith("../") && relativePath.contains(":")) {
            // Different drive, can't make relative - keep as-is or use filename only
            relativePath = QFileInfo(localPath).fileName();
        }
        
        replacements.insert(src, relativePath);
    }
    
    // Apply replacements after collecting all (safe to modify HTML now)
    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
        html.replace(it.key(), it.value());
    }
    
    // Add CSS to ensure images are displayed as block elements with spacing
    QString imageCss = R"(
<style>
    img { display: block; margin: 5px 0; max-width: 100%; height: auto; }
    ul, ol { margin: 4px 0; padding-left: 24px; }
    li { margin: 1px 0; }
</style>
)";
    
    // Insert CSS before </head> tag, or after <head> if </head> not found
    if (html.contains("</head>")) {
        html.replace("</head>", imageCss + "\n</head>");
    } else if (html.contains("<head>")) {
        html.replace("<head>", "<head>\n" + imageCss);
    } else {
        // No head tag, insert after <html> or at the beginning
        if (html.contains("<html>")) {
            html.replace("<html>", "<html>\n<head>\n" + imageCss + "</head>");
        } else {
            html = "<html><head>\n" + imageCss + "</head>\n<body>\n" + html + "\n</body></html>";
        }
    }
    
    return html;
}

QString MainWindow::generateSelfContainedHtml()
{
    // Get the base HTML from the editor
    QString html = m_editor->toHtml();
    
    // Find all image sources in the HTML (handle both single and double quotes)
    QRegularExpression imgRegex("<img[^>]+src=[\"']([^\"']+)[\"']");
    QRegularExpressionMatchIterator matches = imgRegex.globalMatch(html);
    
    // Collect all unique image sources first (to avoid modifying while iterating)
    QHash<QString, QString> replacements;
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString src = match.captured(1);
        
        // Skip already embedded images (data URIs) and processed ones
        if (src.startsWith("data:") || replacements.contains(src)) {
            continue;
        }
        
        // Convert file:// URL to local path if needed
        QString imagePath;
        if (src.startsWith("file:///")) {
            // file:///C:/path/image.png -> C:/path/image.png
            imagePath = QUrl(src).toLocalFile();
        } else if (QFileInfo(src).isRelative()) {
            // Relative path: resolve against markdown file's directory
            imagePath = QFileInfo(m_currentFile).dir().absoluteFilePath(src);
        } else {
            imagePath = src;
        }
        
        // Normalize path separators for consistent comparison
        imagePath = QDir::toNativeSeparators(imagePath);
        
        // Try to embed the image
        if (QFile::exists(imagePath)) {
            QFile imgFile(imagePath);
            if (imgFile.open(QIODevice::ReadOnly)) {
                QByteArray imgData = imgFile.readAll();
                imgFile.close();
                
                // Get MIME type based on extension
                QString mimeType = getMimeType(imagePath);
                if (!mimeType.isEmpty()) {
                    QString base64Data = imgData.toBase64();
                    QString dataUri = QString("data:%1;base64,%2").arg(mimeType, base64Data);
                    replacements.insert(src, dataUri);
                }
            }
        }
    }
    
    // Apply replacements after collecting all (safe to modify HTML now)
    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
        html.replace(it.key(), it.value());
    }
    
    // Add CSS to ensure images are displayed as block elements with spacing
    QString imageCss = R"(
<style>
    img { display: block; margin: 5px 0; max-width: 100%; height: auto; }
    ul, ol { margin: 4px 0; padding-left: 24px; }
    li { margin: 1px 0; }
</style>
)";
    
    // Insert CSS before </head> tag, or after <head> if </head> not found
    if (html.contains("</head>")) {
        html.replace("</head>", imageCss + "\n</head>");
    } else if (html.contains("<head>")) {
        html.replace("<head>", "<head>\n" + imageCss);
    } else {
        // No head tag, insert after <html> or at the beginning
        if (html.contains("<html>")) {
            html.replace("<html>", "<html>\n<head>\n" + imageCss + "</head>");
        } else {
            html = "<html><head>\n" + imageCss + "</head>\n<body>\n" + html + "\n</body></html>";
        }
    }
    
    return html;
}

QString MainWindow::getMimeType(const QString &filePath) const
{
    QString suffix = QFileInfo(filePath).suffix().toLower();
    
    if (suffix == "png") return "image/png";
    if (suffix == "jpg" || suffix == "jpeg") return "image/jpeg";
    if (suffix == "gif") return "image/gif";
    if (suffix == "svg") return "image/svg+xml";
    if (suffix == "bmp") return "image/bmp";
    if (suffix == "webp") return "image/webp";
    if (suffix == "ico") return "image/x-icon";
    
    return QString(); // Unknown type
}

void MainWindow::onCloseFile()
{
    // Disable close when welcome page is shown (no file loaded)
    if (m_currentFile.isEmpty()) {
        return;
    }

    // Stop watching the file
    if (m_fileWatcher) {
        m_fileWatcher->removePaths(m_fileWatcher->files());
    }

    // Clear current file state
    m_currentFile.clear();

    // Clear last search text
    m_lastSearchText.clear();

    // Clear editor content
    m_editor->clear();

    // Reset window title
    setWindowTitle(tr("PlainMD"));

    // Show welcome page
    showWelcomePage();

    // Update status bar - will show "Ready" state since m_currentFile is now empty
    updateStatusBar();

    // Disable file-related actions
    if (m_printAction) {
        m_printAction->setEnabled(false);
    }
    if (m_exportPdfAction) {
        m_exportPdfAction->setEnabled(false);
    }
    if (m_exportHtmlSimpleAction) {
        m_exportHtmlSimpleAction->setEnabled(false);
    }
    if (m_exportHtmlSelfContainedAction) {
        m_exportHtmlSelfContainedAction->setEnabled(false);
    }
    if (m_exportHtmlMenu) {
        m_exportHtmlMenu->setEnabled(false);
    }
    if (m_findAction) {
        m_findAction->setEnabled(false);
    }
    if (m_findNextAction) {
        m_findNextAction->setEnabled(false);
    }
    if (m_closeFileAction) {
        m_closeFileAction->setEnabled(false);
    }
    if (m_reloadAction) {
        m_reloadAction->setEnabled(false);
    }

    // Hide minimap on welcome page
    if (m_minimap) {
        m_minimap->hide();
    }
}

void MainWindow::onReload()
{
    if (!m_currentFile.isEmpty()) {
        loadFile(m_currentFile);
    }
}

void MainWindow::openFile(const QString &filePath, bool loadFileFolder)
{
    if (!QFile::exists(filePath)) {
        QMessageBox::warning(this, tr("Error"),
                             tr("File not found:\n%1").arg(filePath));
        return;
    }

    QFileInfo info(filePath);
    if (!isMarkdownFile(filePath)) {
        QMessageBox::warning(this, tr("Unsupported File"),
                             tr("The file format is not supported:\n%1").arg(filePath));
        return;
    }

    loadFile(filePath);

    if (loadFileFolder) {
        QString folderPath = info.absolutePath();
        if (folderPath != m_currentFolder) {
            loadFolder(folderPath, false);  // Don't remember as lastFolder (file takes priority)
        }
    }

    if (m_settings.value("privacy/keepRecentFiles", true).toBool()) {
        updateRecentFiles(filePath);
    }
}

void MainWindow::openPath(const QString &path)
{
    if (!QFile::exists(path) && !QDir(path).exists()) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Path not found:\n%1").arg(path));
        return;
    }

    QFileInfo info(path);
    QString absPath = info.absoluteFilePath();
    if (info.isDir()) {
        loadFolder(absPath);
    } else if (info.isFile()) {
        openFile(absPath);
    }
}

void MainWindow::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Could not open file:\n%1").arg(filePath));
        return;
    }

    // Detect encoding: try UTF-8 first, fall back to system locale (ANSI/Windows-1252)
    QByteArray rawData = file.readAll();
    file.close();
    
    QString content;
    QString detectedEncoding;
    
    // Try UTF-8 first using QStringDecoder
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    content = utf8Decoder(rawData);
    
    if (!utf8Decoder.hasError()) {
        // Valid UTF-8
        detectedEncoding = QStringLiteral("UTF-8");
    } else {
        // Fall back to system default encoding (Windows-1252 on Windows, Latin1 on Linux, etc.)
        QStringDecoder systemDecoder(QStringDecoder::System);
        content = systemDecoder(rawData);
        detectedEncoding = QStringLiteral("ANSI");
    }
    
    m_detectedEncoding = detectedEncoding;

    // Detect line endings - sample first 8KB to avoid scanning huge files
    const int sampleSize = qMin(8192, rawData.size());
    QByteArray sample = rawData.left(sampleSize);
    if (sample.contains("\r\n")) {
        m_detectedLineEndings = QStringLiteral("CRLF");
    } else if (sample.contains("\n")) {
        // Has newlines but no CRLF, so it's LF
        m_detectedLineEndings = QStringLiteral("LF");
    } else {
        // No newlines detected, assume LF (Unix default)
        m_detectedLineEndings = QStringLiteral("LF");
    }

    m_imageUrlMap.clear();

    bool previewExternal = m_settings.value("privacy/previewExternalImages", true).toBool();
    QString processedContent = resolveFrontMatter(content);
    processedContent = resolveExternalImages(processedContent, previewExternal);
    processedContent = resolveRelativeImages(processedContent, QFileInfo(filePath).absolutePath());

    m_editor->clear();

    QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == "txt" || suffix == "mdx") {
        // Plain text and MDX files - preserve formatting without markdown processing
        // MDX contains JSX syntax that Qt's markdown parser doesn't handle well
        m_editor->setPlainText(content);
        if (m_minimap) {
            m_minimap->setPlainTextMode(true);  // Skip markdown detection in minimap
        }
    } else {
        // Resolve relative image paths against the markdown file's directory
        QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(filePath).absolutePath() + "/");
        m_editor->document()->setBaseUrl(baseUrl);

        m_editor->setMarkdown(processedContent);
        if (m_minimap) {
            m_minimap->setPlainTextMode(false);  // Enable full markdown detection
        }
    }

    // Clear last search text when switching to a different file (regular Find context)
    if (!m_currentFile.isEmpty() && m_currentFile != filePath) {
        m_lastSearchText.clear();
    }

    m_currentFile = filePath;

    // Save last opened file if privacy setting allows
    if (m_settings.value("privacy/rememberLastFile", true).toBool()) {
        m_settings.setValue("lastFile", filePath);
    }

    // Format window title based on user preference
    int titleFormat = m_settings.value("view/windowTitleFormat", 0).toInt();
    if (titleFormat == 1) {
        // Full path format: "PlainMD - E:\\path\\to\\file.md"
        setWindowTitle(tr("PlainMD - %1").arg(QDir::toNativeSeparators(filePath)));
    } else {
        // Filename only format (default): "PlainMD - file.md"
        setWindowTitle(tr("PlainMD - %1").arg(QFileInfo(filePath).fileName()));
    }

    // Watch the file for external changes
    if (m_fileWatcher) {
        if (!m_fileWatcher->files().isEmpty()) {
            m_fileWatcher->removePaths(m_fileWatcher->files());
        }
        m_fileWatcher->addPath(filePath);
    }

    // Enable print, export, find, and close file actions when a file is loaded
    if (m_printAction) {
        m_printAction->setEnabled(true);
    }
    if (m_exportPdfAction) {
        m_exportPdfAction->setEnabled(true);
    }
    if (m_exportHtmlSimpleAction) {
        m_exportHtmlSimpleAction->setEnabled(true);
    }
    if (m_exportHtmlSelfContainedAction) {
        m_exportHtmlSelfContainedAction->setEnabled(true);
    }
    if (m_exportHtmlMenu) {
        m_exportHtmlMenu->setEnabled(true);
    }
    if (m_findAction) {
        m_findAction->setEnabled(true);
    }
    if (m_findNextAction) {
        m_findNextAction->setEnabled(true);
    }
    if (m_closeFileAction) {
        m_closeFileAction->setEnabled(true);
    }
    if (m_reloadAction) {
        m_reloadAction->setEnabled(true);
    }

    // Select the file in the tree if visible
    if (m_fileTree && m_fileTree->model() && m_fileModel && m_proxyModel) {
        QModelIndex sourceIndex = m_fileModel->index(filePath);
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);
        if (proxyIndex.isValid()) {
            m_fileTree->setCurrentIndex(proxyIndex);
            m_fileTree->scrollTo(proxyIndex);
        }
    }

    // Show minimap when a file is loaded (if enabled in settings)
    if (m_minimap && m_settings.value("view/showMinimap", false).toBool()) {
        m_minimap->show();
        m_minimap->updateContent();
    }
    // Enable minimap toggle action when a file is loaded
    if (m_showMinimapAction) {
        m_showMinimapAction->setEnabled(true);
    }

    // Update status bar with file info
    updateStatusBar();

    // Brief message for user feedback (shown next to Tree button)
    if (m_statusFileMsg) {
        m_statusFileMsg->show();  // Make sure it's visible
        m_statusFileMsg->setText(tr("Loaded: %1").arg(QDir::toNativeSeparators(filePath)));
        if (m_statusMsgTimer) {
            m_statusMsgTimer->start(3000);
        }
    }
}

void MainWindow::loadFolder(const QString &folderPath, bool rememberAsLastFolder)
{
    if (!QDir(folderPath).exists()) return;

    // Block root drives (cheap check first)
    QDir dir(folderPath);
    if (dir.isRoot()) {
        QMessageBox::warning(this, tr("Large Folder"),
            tr("Opening the root drive (%1) may cause the application to become unresponsive.\n\n"
               "Please open a specific folder instead.").arg(QDir::toNativeSeparators(folderPath)));
        return;
    }

    // Block only exact system root folders (not subfolders)
    QString pathLower = QDir::cleanPath(folderPath).toLower();
#ifdef Q_OS_WIN
    QStringList systemRoots = QStringList() << "c:/windows" << "c:/program files" << "c:/program files (x86)"
                                              << "c:/programdata";
    if (systemRoots.contains(pathLower)) {
        QMessageBox::warning(this, tr("System Folder"),
            tr("Opening %1 directly may cause the application to become unresponsive.\n\n"
               "Please open a specific subfolder instead (e.g., %2\\YourApp)."
               ).arg(QDir::toNativeSeparators(folderPath))
                .arg(QDir::toNativeSeparators(folderPath)));
        return;
    }
    if (pathLower.contains("/system volume information")) {
        QMessageBox::warning(this, tr("System Folder"),
            tr("Cannot open system folder (%1).").arg(QDir::toNativeSeparators(folderPath)));
        return;
    }
#else
    QStringList systemRoots = QStringList() << "/bin" << "/sbin" << "/usr" << "/etc" << "/lib"
                                            << "/lib64" << "/dev" << "/proc" << "/sys" << "/boot";
    if (systemRoots.contains(pathLower)) {
        QMessageBox::warning(this, tr("System Folder"),
            tr("Opening %1 directly may cause the application to become unresponsive.\n\n"
               "Please open a specific subfolder instead."
               ).arg(QDir::toNativeSeparators(folderPath)));
        return;
    }
#endif

    // Check if folder has any valid markdown files (expensive scan - do last)
    if (!folderHasValidFiles(folderPath)) {
        // No valid files found - show info but don't change tree
        if (m_statusFileMsg) {
            m_statusFileMsg->show();
            m_statusFileMsg->setText(tr("No markdown files found in: %1").arg(QDir::toNativeSeparators(folderPath)));
            m_statusMsgTimer->start(3000);
        }
        return;
    }

    if (!m_fileTree->model()) {
        m_fileTree->setModel(m_proxyModel);
        m_fileTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_fileTree->hideColumn(1);
        m_fileTree->hideColumn(2);
        m_fileTree->hideColumn(3);
    }

    m_currentFolder = folderPath;
    m_proxyModel->setExemptPath(folderPath);
    m_fileModel->setRootPath(folderPath);
    if (rememberAsLastFolder && m_settings.value("privacy/rememberLastFolder", true).toBool()) {
        m_settings.setValue("lastFolder", folderPath);
    }

    QModelIndex sourceRoot = m_fileModel->index(folderPath);
    QModelIndex proxyRoot = m_proxyModel->mapFromSource(sourceRoot);
    m_fileTree->setRootIndex(proxyRoot);

    // Hide welcome page and show file tree
    if (m_fileTreeWelcome) {
        m_fileTreeWelcome->hide();
    }
    m_fileTree->show();

    // Track recent folder
    if (m_settings.value("privacy/keepRecentFolders", true).toBool()) {
        updateRecentFolders(folderPath);
    }
}

void MainWindow::updateRecentFiles(const QString &filePath)
{
    if (!m_settings.value("privacy/keepRecentFiles", true).toBool()) {
        return;
    }

    QStringList recentFiles = m_settings.value("recentFiles").toStringList();

    if (!filePath.isEmpty()) {
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while (recentFiles.size() > 10) {
            recentFiles.removeLast();
        }
        m_settings.setValue("recentFiles", recentFiles);
    } else {
        // Remove non-existent files
        QStringList validFiles;
        for (const QString &f : recentFiles) {
            if (QFile::exists(f)) validFiles.append(f);
        }
        m_settings.setValue("recentFiles", validFiles);
    }

    refreshRecentFilesMenu();
}

void MainWindow::refreshRecentFilesMenu()
{
    if (!m_recentMenu) return;

    m_recentMenu->clear();
    m_recentActions.clear();

    bool keepRecent = m_settings.value("privacy/keepRecentFiles", true).toBool();
    QStringList recentFiles = keepRecent ? m_settings.value("recentFiles").toStringList() : QStringList();

    if (recentFiles.isEmpty()) {
        QAction *emptyAction = new QAction(tr("No Recent Files"), this);
        emptyAction->setEnabled(false);
        m_recentMenu->addAction(emptyAction);
    } else {
        for (int i = 0; i < recentFiles.size(); ++i) {
            QString filePath = recentFiles.at(i);
            // Convert to native separators for display
            QString displayPath = QDir::toNativeSeparators(filePath);
            QString displayName = QString("&%1 %2").arg(i + 1).arg(displayPath);

            QAction *action = new QAction(displayName, this);
            action->setData(filePath);
            action->setToolTip(displayPath);
            connect(action, &QAction::triggered, this, &MainWindow::onRecentFileTriggered);
            m_recentMenu->addAction(action);
            m_recentActions.append(action);
        }
    }

    m_recentMenu->addSeparator();
    m_recentMenu->addAction(m_clearRecentAction);
}

void MainWindow::updateRecentFolders(const QString &folderPath)
{
    if (!m_settings.value("privacy/keepRecentFolders", true).toBool()) {
        return;
    }

    QStringList recentFolders = m_settings.value("recentFolders").toStringList();

    if (!folderPath.isEmpty()) {
        recentFolders.removeAll(folderPath);
        recentFolders.prepend(folderPath);
        while (recentFolders.size() > 10) {
            recentFolders.removeLast();
        }
        m_settings.setValue("recentFolders", recentFolders);
    } else {
        // Remove non-existent folders
        QStringList validFolders;
        for (const QString &f : recentFolders) {
            if (QDir(f).exists()) validFolders.append(f);
        }
        m_settings.setValue("recentFolders", validFolders);
    }

    refreshRecentFoldersMenu();
}

void MainWindow::refreshRecentFoldersMenu()
{
    if (!m_recentFoldersMenu) return;

    m_recentFoldersMenu->clear();
    m_recentFolderActions.clear();

    bool keepRecent = m_settings.value("privacy/keepRecentFolders", true).toBool();
    QStringList recentFolders = keepRecent ? m_settings.value("recentFolders").toStringList() : QStringList();

    if (recentFolders.isEmpty()) {
        QAction *emptyAction = new QAction(tr("No Recent Folders"), this);
        emptyAction->setEnabled(false);
        m_recentFoldersMenu->addAction(emptyAction);
    } else {
        for (int i = 0; i < recentFolders.size(); ++i) {
            QString folderPath = recentFolders.at(i);
            // Convert to native separators for display
            QString displayPath = QDir::toNativeSeparators(folderPath);
            QString displayName = QString("&%1 %2").arg(i + 1).arg(displayPath);

            QAction *action = new QAction(displayName, this);
            action->setData(folderPath);
            action->setToolTip(displayPath);
            connect(action, &QAction::triggered, this, &MainWindow::onRecentFolderTriggered);
            m_recentFoldersMenu->addAction(action);
            m_recentFolderActions.append(action);
        }
    }

    m_recentFoldersMenu->addSeparator();
    m_recentFoldersMenu->addAction(m_clearRecentFoldersAction);
}

bool MainWindow::isMarkdownFile(const QString &filePath) const
{
    QString suffix = QFileInfo(filePath).suffix().toLower();
    return suffix == "md" || suffix == "markdown" || suffix == "mdx" || suffix == "txt";
}

bool MainWindow::folderHasValidFiles(const QString &folderPath)
{
    // Show progress bar (indeterminate mode = busy indicator)
    if (m_statusProgress) {
        // Ensure progress bar is visible and animate
        m_statusProgress->setValue(0);
        m_statusProgress->setRange(0, 0);  // Reset to indeterminate mode
        m_statusProgress->show();
        m_statusProgress->repaint();  // Force immediate paint
        statusBar()->repaint();  // Force status bar repaint
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    // Recursive check with depth limit and file count limit
    // to avoid scanning massive folders
    const int maxDepth = 3;  // Check up to 3 levels deep
    const int maxFiles = 100; // Stop after finding 100 files
    
    std::function<bool(const QString&, int)> checkRecursive;
    int filesFound = 0;
    
    checkRecursive = [&](const QString &path, int depth) -> bool {
        if (depth > maxDepth || filesFound >= maxFiles) {
            return filesFound > 0;
        }
        
        QDir dir(path);
        // Check files in current directory
        QFileInfoList entries = dir.entryInfoList(QStringList() << "*.md" << "*.markdown" << "*.mdx" << "*.txt",
                                                    QDir::Files, QDir::Name);
        if (!entries.isEmpty()) {
            filesFound += entries.size();
            if (filesFound >= maxFiles) {
                return true;
            }
        }
        
        // Check subdirectories
        QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &subdir : subdirs) {
            if (checkRecursive(subdir.filePath(), depth + 1)) {
                return true;
            }
        }
        
        return filesFound > 0;
    };
    
    bool result = checkRecursive(folderPath, 0);
    
    // Hide progress bar
    if (m_statusProgress) {
        m_statusProgress->hide();
    }
    
    return result;
}

QString MainWindow::resolveExternalImages(const QString &markdownContent, bool previewEnabled)
{
    QString result = markdownContent;

    struct Match {
        qsizetype urlPos;
        qsizetype urlLen;
        QString url;
        qsizetype fullPos;
        qsizetype fullLen;
        QString altText;
    };
    QList<Match> matches;

    // Markdown images: ![alt](url) or ![alt](url "title")
    QRegularExpression mdRe(QStringLiteral(R"(!\[([^\]]*)\]\((https?://[^)\s\"]+)[^)]*\))"));
    {
        QRegularExpressionMatchIterator it = mdRe.globalMatch(result);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            matches.append({m.capturedStart(2), m.capturedLength(2), m.captured(2),
                            m.capturedStart(0), m.capturedLength(0), m.captured(1)});
        }
    }

    // HTML <img> tags
    QRegularExpression htmlRe(QStringLiteral(R"(<img[^>]+src\s*=\s*["'](https?://[^"']+)["'])"));
    {
        QRegularExpressionMatchIterator it = htmlRe.globalMatch(result);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            matches.append({m.capturedStart(1), m.capturedLength(1), m.captured(1),
                            m.capturedStart(0), m.capturedLength(0), QString()});
        }
    }

    if (matches.isEmpty()) return result;

    // Locate fenced code blocks and inline code spans so we skip image-looking syntax inside them
    QList<QPair<qsizetype, qsizetype>> codeBlocks;

    // First, collect fenced code blocks
    QRegularExpression fenceRe(QStringLiteral("```[\\s\\S]*?```"));
    QRegularExpressionMatchIterator fenceIt = fenceRe.globalMatch(result);
    while (fenceIt.hasNext()) {
        QRegularExpressionMatch fm = fenceIt.next();
        codeBlocks.append({fm.capturedStart(), fm.capturedEnd()});
    }

    // Then, collect inline code spans, but skip any that fall inside fenced code blocks
    QRegularExpression inlineCodeRe(QStringLiteral("`[^`]+`"));
    QRegularExpressionMatchIterator inlineIt = inlineCodeRe.globalMatch(result);
    while (inlineIt.hasNext()) {
        QRegularExpressionMatch im = inlineIt.next();
        qsizetype spanStart = im.capturedStart();
        qsizetype spanEnd = im.capturedEnd();

        // Check if this inline span is inside any fenced code block
        bool insideFence = false;
        for (const auto &fence : codeBlocks) {
            if (spanStart >= fence.first && spanEnd <= fence.second) {
                insideFence = true;
                break;
            }
        }

        if (!insideFence) {
            codeBlocks.append({spanStart, spanEnd});
        }
    }

    // Sort by position descending so replacements do not shift earlier indices
    std::sort(matches.begin(), matches.end(),
              [](const Match &a, const Match &b) { return a.urlPos > b.urlPos; });

    for (const Match &match : matches) {
        // Skip anything inside a fenced code block or inline code span
        bool insideCodeBlock = false;
        for (const auto &range : codeBlocks) {
            // Use <= for end check because image can start right at the boundary of inline code
            if (match.fullPos >= range.first && match.fullPos <= range.second) {
                insideCodeBlock = true;
                break;
            }
        }
        if (insideCodeBlock) continue;
        if (!previewEnabled) {
            QString replacement = match.altText.isEmpty()
                ? QStringLiteral("[External Image]")
                : QStringLiteral("[%1]").arg(match.altText);
            result.replace(match.fullPos, match.fullLen, replacement);
            continue;
        }

        QString urlStr = match.url;
        QString ext = QFileInfo(QUrl(urlStr).path()).suffix();
        if (ext.isEmpty()) ext = "png";

        QByteArray hash = QCryptographicHash::hash(urlStr.toUtf8(), QCryptographicHash::Md5).toHex();
        QString localPath = QDir::tempPath() + "/plainmd_images/" + QString::fromLatin1(hash) + "." + ext;

        if (!QFile::exists(localPath)) {
            QDir().mkpath(QDir::tempPath() + "/plainmd_images");
            QNetworkAccessManager nam;
            QUrl imgUrl(urlStr);
            QNetworkRequest req(imgUrl);
            req.setHeader(QNetworkRequest::UserAgentHeader, "MarkdownViewer/1.0");
            QNetworkReply *reply = nam.get(req);

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

            QTimer timer;
            timer.setSingleShot(true);
            timer.setInterval(10000);
            connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
            timer.start();

            loop.exec();

            if (reply->error() == QNetworkReply::NoError) {
                QFile file(localPath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(reply->readAll());
                    file.close();
                }
            }
            reply->deleteLater();
        }

        if (QFile::exists(localPath)) {
            m_imageUrlMap[localPath] = urlStr;
            result.replace(match.urlPos, match.urlLen, localPath);
        }
    }

    return result;
}

QString MainWindow::resolveRelativeImages(const QString &markdownContent, const QString &basePath)
{
    QString result = markdownContent;
    QDir baseDir(basePath);

    struct Match {
        qsizetype urlPos;
        qsizetype urlLen;
        QString url;
        qsizetype fullPos;
        qsizetype fullLen;
    };
    QList<Match> matches;

    // Markdown images: ![alt](url) or ![alt](url "title")
    QRegularExpression mdRe(QStringLiteral(R"(!\[[^\]]*\]\(([^)\s\"]+)[^)]*\))"));
    QRegularExpressionMatchIterator it = mdRe.globalMatch(result);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        matches.append({m.capturedStart(1), m.capturedLength(1), m.captured(1),
                        m.capturedStart(0), m.capturedLength(0)});
    }

    // HTML <img> tags
    QRegularExpression htmlRe(QStringLiteral(R"(<img[^>]+src\s*=\s*["']([^"']+)["'])"));
    it = htmlRe.globalMatch(result);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        matches.append({m.capturedStart(1), m.capturedLength(1), m.captured(1),
                        m.capturedStart(0), m.capturedLength(0)});
    }

    // Locate fenced code blocks and inline code spans so we skip image-looking syntax inside them
    QList<QPair<qsizetype, qsizetype>> codeBlocks;

    // First, collect fenced code blocks
    QRegularExpression fenceRe(QStringLiteral("```[\\s\\S]*?```"));
    QRegularExpressionMatchIterator fenceIt = fenceRe.globalMatch(result);
    while (fenceIt.hasNext()) {
        QRegularExpressionMatch fm = fenceIt.next();
        codeBlocks.append({fm.capturedStart(), fm.capturedEnd()});
    }

    // Then, collect inline code spans, but skip any that fall inside fenced code blocks
    QRegularExpression inlineCodeRe(QStringLiteral("`[^`]+`"));
    QRegularExpressionMatchIterator inlineIt = inlineCodeRe.globalMatch(result);
    while (inlineIt.hasNext()) {
        QRegularExpressionMatch im = inlineIt.next();
        qsizetype spanStart = im.capturedStart();
        qsizetype spanEnd = im.capturedEnd();

        // Check if this inline span is inside any fenced code block
        bool insideFence = false;
        for (const auto &fence : codeBlocks) {
            if (spanStart >= fence.first && spanEnd <= fence.second) {
                insideFence = true;
                break;
            }
        }

        if (!insideFence) {
            codeBlocks.append({spanStart, spanEnd});
        }
    }

    // Sort by position descending so replacements do not shift earlier indices
    std::sort(matches.begin(), matches.end(),
              [](const Match &a, const Match &b) { return a.urlPos > b.urlPos; });

    // Skip anything inside a fenced code block or inline code span
    for (const Match &match : matches) {
        bool insideCodeBlock = false;
        for (const auto &range : codeBlocks) {
            // Use <= for end check because image can start right at the boundary of inline code
            if (match.fullPos >= range.first && match.fullPos <= range.second) {
                insideCodeBlock = true;
                break;
            }
        }
        if (insideCodeBlock) continue;
        QUrl url(match.url);
        if (url.scheme().isEmpty() && !QDir::isAbsolutePath(match.url)) {
            QString absoluteUrl = QUrl::fromLocalFile(baseDir.absoluteFilePath(match.url)).toString();
            result.replace(match.urlPos, match.urlLen, absoluteUrl);
        }
    }

    return result;
}

QString MainWindow::resolveFrontMatter(const QString &markdownContent)
{
    // Frontmatter appears at the very top of the file between --- delimiters.
    // Qt's markdown parser hides it, so we convert it to a fenced code block
    // with a special language tag. Our post-processor will detect it and
    // apply a distinct background color.
    QString result = markdownContent;

    result.replace("\r\n", "\n");
    result.replace("\r", "\n");

    QRegularExpression fmRe(QStringLiteral(R"(^---\n([\s\S]*?)\n---)"));
    QRegularExpressionMatch m = fmRe.match(result);
    if (!m.hasMatch()) return result;

    QString fmBody = m.captured(1);

    // Wrap in a fenced block so Qt renders it as monospace.
    QString replacement = QStringLiteral("```yaml\n%1\n```").arg(fmBody);
    result.replace(m.capturedStart(), m.capturedLength(), replacement);
    return result;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        QString filePath = url.toLocalFile();
        QFileInfo info(filePath);

        if (info.isDir()) {
            loadFolder(filePath);
        } else if (info.isFile() && isMarkdownFile(filePath)) {
            openFile(filePath);
            break;
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings.setValue("geometry", saveGeometry());
    m_settings.setValue("windowState", saveState());
    m_settings.setValue("splitterState", m_splitter->saveState());
    event->accept();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Handle file tree viewport resize to update welcome label geometry
    // Guard against early initialization when m_fileTree may not be fully set up
    if (m_fileTree && obj == m_fileTree->viewport() && event->type() == QEvent::Resize) {
        if (m_fileTreeWelcome && m_fileTreeWelcome->isVisible()) {
            m_fileTreeWelcome->setGeometry(m_fileTree->viewport()->rect());
        }
    }
    
    // Handle wheel events for Ctrl+Scroll zoom to update minimap and status bar
    // Guard against early initialization when m_editor may not be fully set up
    if (m_editor && obj == m_editor->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            // Ctrl+Scroll triggers zoom in QTextEdit, update minimap and zoom display after
            if (m_minimap) {
                // Use a small timer to ensure zoom has been applied
                QTimer::singleShot(0, m_minimap, &Minimap::updateContent);
            }
            // Update zoom level in status bar
            QTimer::singleShot(0, this, &MainWindow::updateZoomDisplay);
        }
    }
    
    if (m_editor && obj == m_editor->viewport() && event->type() == QEvent::ToolTip) {
        // Skip custom image tooltips on the welcome page (no file loaded)
        if (m_currentFile.isEmpty())
            return QMainWindow::eventFilter(obj, event);
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        QPoint pos = m_editor->viewport()->mapFromGlobal(helpEvent->globalPos());

        QTextDocument *doc = m_editor->document();

        // Scan every image fragment and test whether the mouse is inside its
        // visual bounding rect.  cursorRect() gives us viewport coordinates for
        // the fragment edges; the line height gives us the vertical span.
        for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
            for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
                QTextFragment frag = it.fragment();
                if (!frag.isValid()) continue;
                QTextCharFormat fmt = frag.charFormat();
                if (!fmt.isImageFormat()) continue;

                int startPos = frag.position();
                int endPos   = startPos + frag.length();

                QTextCursor c1(doc);
                c1.setPosition(startPos);
                QRect r1 = m_editor->cursorRect(c1);

                QTextCursor c2(doc);
                c2.setPosition(endPos);
                QRect r2 = m_editor->cursorRect(c2);

                if (!r1.isValid() || !r2.isValid()) continue;

                // Horizontal span: from the left edge of the first cursor to the
                // left edge of the second cursor.
                QRect imgRect;
                imgRect.setLeft(qMin(r1.left(), r2.left()));
                imgRect.setRight(qMax(r1.left(), r2.right()));
                imgRect.setTop(qMin(r1.top(), r2.top()));

                // Vertical span: try to use the real line height so tall images
                // are fully covered.
                int lineHeight = r1.height();
                QTextLayout *layout = block.layout();
                for (int i = 0; i < layout->lineCount(); ++i) {
                    QTextLine line = layout->lineAt(i);
                    int lineStart = line.textStart();
                    int lineEnd   = lineStart + line.textLength();
                    int fragStartInBlock = startPos - block.position();
                    if (fragStartInBlock >= lineStart && fragStartInBlock < lineEnd) {
                        lineHeight = qMax(lineHeight, (int)line.height());
                        break;
                    }
                }
                imgRect.setBottom(imgRect.top() + lineHeight);

                if (imgRect.contains(pos)) {
                    QString imgName = fmt.toImageFormat().name();
                    // Strip file:/// prefix for cleaner display
                    QString displayName = imgName;
                    if (displayName.startsWith("file:///")) {
                        displayName = displayName.mid(8);
                    }
                    // Clean up path (resolve .. and .)
                    displayName = QDir::cleanPath(displayName);
                    QString tooltip;
                    if (m_imageUrlMap.contains(imgName)) {
                        tooltip = tr("Original: %1\nCached: %2")
                                      .arg(m_imageUrlMap[imgName], QDir::toNativeSeparators(displayName));
                    } else {
                        tooltip = QDir::toNativeSeparators(displayName);
                    }
                    QToolTip::showText(helpEvent->globalPos(), tooltip, m_editor);
                    return true;
                }
            }
        }

        // Also check for link/anchor fragments (show URL on hover)
        QTextCursor cursor = m_editor->cursorForPosition(pos);
        if (!cursor.isNull()) {
            QTextCharFormat fmt = cursor.charFormat();
            if (fmt.isAnchor()) {
                QString url = fmt.anchorHref();
                if (!url.isEmpty()) {
                    // Resolve relative URLs to absolute paths
                    QUrl urlObj(url);
                    QString displayUrl;
                    if (urlObj.isRelative() && !m_currentFile.isEmpty()) {
                        QFileInfo currentInfo(m_currentFile);
                        QString absolutePath = currentInfo.dir().absoluteFilePath(url);
                        // Clean up path (resolve .. and .)
                        absolutePath = QDir::cleanPath(absolutePath);
                        displayUrl = QDir::toNativeSeparators(absolutePath);
                    } else {
                        displayUrl = url;
                    }
                    QToolTip::showText(helpEvent->globalPos(), displayUrl, m_editor);
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Handle Escape key to clear search selection in editor
    if (event->key() == Qt::Key_Escape && m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        if (cursor.hasSelection()) {
            // Clear selection by moving cursor to end of selection
            cursor.setPosition(cursor.selectionEnd());
            m_editor->setTextCursor(cursor);
            return;  // Event handled
        }
    }
    QMainWindow::keyPressEvent(event);
}
