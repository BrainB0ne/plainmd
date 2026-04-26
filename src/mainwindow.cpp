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
#include "aboutdialog.h"

#include <QApplication>
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
#include <QTimer>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QApplication::organizationName(), QApplication::applicationName())
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setAcceptDrops(true);
    setWindowTitle(tr("PlainMD"));
    setWindowIcon(QIcon(":/icon.png"));
    resize(1200, 800);

    QString lastFolder = m_settings.value("lastFolder").toString();
    if (!lastFolder.isEmpty() && QDir(lastFolder).exists()) {
        loadFolder(lastFolder);
    }

    showWelcomePage();
    refreshRecentFilesMenu();
    refreshRecentFoldersMenu();

    // Setup file watcher for auto-reload
    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onFileChanged);
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

    applyEditorFont();

    // Use default Qt markdown styling (no custom CSS)
    // setMarkdownStyle() and styleCodeBlocks() disabled due to Qt6 bugs

    m_splitter->addWidget(m_editor);
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

    QAction *findAction = new QAction(QIcon(":/images/binoculars.png"), tr("&Find..."), this);
    findAction->setShortcut(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &MainWindow::onFind);
    viewMenu->addAction(findAction);

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
}

void MainWindow::onZoomOut()
{
    m_editor->zoomOut(2);
}

void MainWindow::onZoomReset()
{
    applyEditorFont();
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
        // setMarkdownStyle() and styleCodeBlocks() disabled - use default Qt styling
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
    }
    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void MainWindow::onFileChanged(const QString &path)
{
    if (path != m_currentFile || !QFile::exists(path)) {
        return;
    }

    // File has been modified externally - ask user if they want to reload
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("File Modified"),
        tr("The file has been modified externally:\n%1\n\nDo you want to reload it?").arg(QFileInfo(path).fileName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (reply == QMessageBox::Yes) {
        loadFile(path);
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
    QFont font(family);
    font.setPointSize(size);
    m_editor->setFont(font);
}

void MainWindow::showWelcomePage()
{
    m_editor->clear();
    m_currentFile.clear();
    setWindowTitle(tr("PlainMD"));

    // Disable print and export actions on welcome page
    if (m_printAction) {
        m_printAction->setEnabled(false);
    }
    if (m_exportPdfAction) {
        m_exportPdfAction->setEnabled(false);
    }

    // Stop watching files when showing welcome page
    if (m_fileWatcher && !m_fileWatcher->files().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->files());
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
                <span style="font-size:0.9em; color:#95a5a6;">Version 1.2.0</span>
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
    )").arg(fontFamily, monoFamily);

    m_editor->setHtml(html);
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
        QString emojiFontFamily = m_settings.value("editor/printEmojiFont", QStringLiteral("Segoe UI")).toString();
        int emojiFontSize = m_settings.value("editor/printEmojiFontSize", 11).toInt();

        QString printCss;
        QFont printFont;

        if (useNerdFont) {
            // Use Nerd Font for proper monochrome emoji rendering
            printCss = QStringLiteral(R"(
                body { font-family: '%1', 'Segoe UI', sans-serif; }
                td, th { font-family: '%1', 'Segoe UI', sans-serif; }
            )").arg(emojiFontFamily);
            printFont = QFont(emojiFontFamily, emojiFontSize);
            printFont.setStyleHint(QFont::Monospace);
            printFont.setFamilies(QStringList() << emojiFontFamily << "Segoe UI");
        } else {
            // Use standard emoji fonts (may not print correctly on Windows PDF)
            printCss = QStringLiteral(R"(
                body { font-family: 'Segoe UI', 'Segoe UI Emoji', 'Segoe UI Symbol', sans-serif; }
                td, th { font-family: 'Segoe UI Emoji', 'Segoe UI Symbol', 'Segoe UI', sans-serif; }
            )");
            printFont = QFont("Segoe UI", 11);
            printFont.setStyleHint(QFont::SansSerif);
            printFont.setFamilies(QStringList() << "Segoe UI" << "Segoe UI Emoji" << "Segoe UI Symbol");
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
    QString emojiFontFamily = m_settings.value("editor/printEmojiFont", QStringLiteral("Segoe UI")).toString();
    int emojiFontSize = m_settings.value("editor/printEmojiFontSize", 11).toInt();

    QString printCss;
    QFont printFont;

    if (useNerdFont) {
        // Use Nerd Font for proper monochrome emoji rendering
        printCss = QStringLiteral(R"(
            body { font-family: '%1', 'Segoe UI', sans-serif; }
            td, th { font-family: '%1', 'Segoe UI', sans-serif; }
        )").arg(emojiFontFamily);
        printFont = QFont(emojiFontFamily, emojiFontSize);
        printFont.setStyleHint(QFont::Monospace);
        printFont.setFamilies(QStringList() << emojiFontFamily << "Segoe UI");
    } else {
        // Use standard emoji fonts (may not print correctly on Windows PDF)
        printCss = QStringLiteral(R"(
            body { font-family: 'Segoe UI', 'Segoe UI Emoji', 'Segoe UI Symbol', sans-serif; }
            td, th { font-family: 'Segoe UI Emoji', 'Segoe UI Symbol', 'Segoe UI', sans-serif; }
        )");
        printFont = QFont("Segoe UI", 11);
        printFont.setStyleHint(QFont::SansSerif);
        printFont.setFamilies(QStringList() << "Segoe UI" << "Segoe UI Emoji" << "Segoe UI Symbol");
    }

    printDoc.setDefaultStyleSheet(printCss);
    printDoc.setDefaultFont(printFont);
    printDoc.print(&printer);

    // Show success message
    QMessageBox::information(this, tr("Export Successful"),
                             tr("Document exported to:\n%1").arg(QDir::toNativeSeparators(filePath)));
}

void MainWindow::openFile(const QString &filePath)
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

    QString folderPath = info.absolutePath();
    if (folderPath != m_currentFolder) {
        loadFolder(folderPath);
    }

    if (m_settings.value("privacy/keepRecentFiles", true).toBool()) {
        updateRecentFiles(filePath);
    }
}

void MainWindow::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Could not open file:\n%1").arg(filePath));
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    file.close();

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
    } else {
        // Resolve relative image paths against the markdown file's directory
        QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(filePath).absolutePath() + "/");
        m_editor->document()->setBaseUrl(baseUrl);

        m_editor->setMarkdown(processedContent);
        // styleCodeBlocks(); // Disabled - causes document corruption on large files
    }

    m_currentFile = filePath;
    setWindowTitle(tr("%1 - PlainMD").arg(QFileInfo(filePath).fileName()));

    // Watch the file for external changes
    if (m_fileWatcher) {
        if (!m_fileWatcher->files().isEmpty()) {
            m_fileWatcher->removePaths(m_fileWatcher->files());
        }
        m_fileWatcher->addPath(filePath);
    }

    // Enable print and export actions when a file is loaded
    if (m_printAction) {
        m_printAction->setEnabled(true);
    }
    if (m_exportPdfAction) {
        m_exportPdfAction->setEnabled(true);
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

    statusBar()->showMessage(tr("Loaded: %1").arg(QDir::toNativeSeparators(filePath)), 3000);
}

void MainWindow::loadFolder(const QString &folderPath)
{
    if (!QDir(folderPath).exists()) return;

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
    m_settings.setValue("lastFolder", folderPath);

    QModelIndex sourceRoot = m_fileModel->index(folderPath);
    QModelIndex proxyRoot = m_proxyModel->mapFromSource(sourceRoot);
    m_fileTree->setRootIndex(proxyRoot);

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
    if (obj == m_editor->viewport() && event->type() == QEvent::ToolTip) {
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
