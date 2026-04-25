#include "mainwindow.h"
#include "filterproxymodel.h"
#include "preferencesdialog.h"
#include "finddialog.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QFontDatabase>
#include <QHeaderView>
#include <QStyle>
#include <QScreen>
#include <QDebug>
#include <QIcon>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextList>
#include <QPrinter>
#include <QPrintDialog>
#include <QHelpEvent>
#include <QToolTip>
#include <QMenu>
#include <QProcess>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QApplication::organizationName(), QApplication::applicationName())
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setAcceptDrops(true);
    setWindowTitle(tr("Vibe-MD"));
    setWindowIcon(QIcon(":/icon.png"));
    resize(1200, 800);

    QString lastFolder = m_settings.value("lastFolder").toString();
    if (!lastFolder.isEmpty() && QDir(lastFolder).exists()) {
        loadFolder(lastFolder);
    }

    showWelcomePage();
    refreshRecentFilesMenu();
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

    setMarkdownStyle();

    m_splitter->addWidget(m_editor);
    m_splitter->setStretchFactor(1, 1);
}

void MainWindow::setMarkdownStyle()
{
    // Get the code font from settings
#ifdef Q_OS_LINUX
    const QString defaultCodeFontFamily = QStringLiteral("DejaVu Sans Mono");
    const QString bodyFontFamily = QStringLiteral("\"DejaVu Sans\", \"Noto Sans\", \"Helvetica Neue\", Arial, sans-serif");
#else
    const QString defaultCodeFontFamily = QStringLiteral("Consolas");
    const QString bodyFontFamily = QStringLiteral("\"Segoe UI\", \"Helvetica Neue\", Arial, sans-serif");
#endif
    QString codeFontFamily = m_settings.value("editor/codeBlockFontFamily", defaultCodeFontFamily).toString();
    // Escape single quotes for CSS
    codeFontFamily.replace("'", "\\'");

    QString style = QStringLiteral(R"(
        body {
            font-family: %1;
            font-size: 11pt;
            line-height: 1.6;
            color: #333333;
            margin: 20px;
        }
        h1 { font-size: 2em; color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 8px; margin-top: 24px; }
        h2 { font-size: 1.6em; color: #34495e; border-bottom: 1px solid #bdc3c7; padding-bottom: 6px; margin-top: 20px; }
        h3 { font-size: 1.3em; color: #34495e; margin-top: 16px; }
        h4, h5, h6 { color: #34495e; margin-top: 12px; }
        a { color: #3498db; text-decoration: none; }
        a:hover { text-decoration: underline; }
        code {
            background-color: #f0f0f0;
            color: #c7254e;
            padding: 2px 6px;
            border-radius: 4px;
            font-family: '%2', 'SFMono-Regular', 'DejaVu Sans Mono', Consolas, 'Liberation Mono', Menlo, Courier, monospace;
            font-size: 0.9em;
        }
        pre {
            background-color: #f4f4f4;
            color: #333333;
            padding: 16px;
            border-radius: 8px;
            overflow-x: auto;
            margin: 16px 0;
            border-left: 4px solid #3498db;
        }
        pre code {
            background: none;
            color: #333333;
            padding: 0;
            border-radius: 0;
            font-size: 0.95em;
        }
        blockquote {
            border-left: 4px solid #3498db;
            margin: 12px 0;
            padding: 8px 16px;
            background-color: #f8f9fa;
            color: #555555;
        }
        table {
            border-collapse: collapse;
            width: 100%;
            margin: 12px 0;
        }
        th, td {
            border: 1px solid #ddd;
            padding: 8px 12px;
            text-align: left;
        }
        th {
            background-color: #3498db;
            color: white;
            font-weight: bold;
        }
        tr:nth-child(even) {
            background-color: #f2f2f2;
        }
        hr {
            border: none;
            border-top: 1px solid #bdc3c7;
            margin: 20px 0;
        }
        ul, ol {
            margin: 8px 0;
            padding-left: 24px;
        }
        li {
            margin: 4px 0;
        }
        img {
            max-width: 100%;
            height: auto;
        }
    )").arg(bodyFontFamily, codeFontFamily);

    m_editor->document()->setDefaultStyleSheet(style);
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

    fileMenu->addSeparator();

    m_recentMenu = fileMenu->addMenu(tr("Recent &Files"));
    m_clearRecentAction = new QAction(tr("&Clear Recent Files"), this);
    connect(m_clearRecentAction, &QAction::triggered, this, &MainWindow::onClearRecent);

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

    QAction *aboutQtAction = new QAction(tr("About &Qt"), this);
    connect(aboutQtAction, &QAction::triggered, this, &MainWindow::onAboutQt);
    helpMenu->addAction(aboutQtAction);
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

void MainWindow::onClearRecent()
{
    m_settings.setValue("recentFiles", QStringList());
    refreshRecentFilesMenu();
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
    QDialog dlg(this);
    dlg.setWindowTitle(tr("About Vibe-MD"));
    dlg.setMinimumSize(420, 420);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *title = new QLabel(tr("<h2>Vibe-MD</h2>"), &dlg);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel *version = new QLabel(tr("Version 1.2"), &dlg);
    version->setAlignment(Qt::AlignCenter);
    layout->addWidget(version);

    QLabel *desc = new QLabel(tr("A simple and elegant Markdown viewer built with Qt6."), &dlg);
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    layout->addSpacing(8);

    QLabel *featuresLabel = new QLabel(tr("Features:"), &dlg);
    layout->addWidget(featuresLabel);

    QTextEdit *featuresEdit = new QTextEdit(&dlg);
    featuresEdit->setReadOnly(true);
    featuresEdit->setPlainText(
        tr("- Native Qt6 markdown rendering\n"
           "- Markdown, MDX, and plain text support\n"
           "- File browser sidebar\n"
           "- Drag and drop support\n"
           "- Recent files\n"
           "- Zoom controls\n"
           "- Find / Search\n"
           "- Print support\n"
           "- External image preview with privacy toggle\n"
           "- Customizable editor and code-block fonts\n"
           "- YAML frontmatter display"));
    layout->addWidget(featuresEdit);

    QLabel *icons = new QLabel(
        tr("Icons by <a href='https://tabler.io/icons'>Tabler Icons</a> "
           "(<a href='https://github.com/tabler/tabler-icons/blob/main/LICENSE'>MIT License</a>)"), &dlg);
    icons->setOpenExternalLinks(true);
    icons->setAlignment(Qt::AlignCenter);
    icons->setWordWrap(true);
    layout->addWidget(icons);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    layout->addWidget(buttons);

    dlg.exec();
}

void MainWindow::onAboutQt()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::onPreferences()
{
    PreferencesDialog dlg(this);
    dlg.loadSettings();
    if (dlg.exec() == QDialog::Accepted) {
        dlg.saveSettings();
        applyEditorFont();
        setMarkdownStyle(); // Update CSS with new inline code font
        if (!dlg.keepRecentFiles()) {
            m_settings.setValue("recentFiles", QStringList());
        }
        refreshRecentFilesMenu();
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
    setWindowTitle(tr("Vibe-MD"));

    // Disable print action on welcome page
    if (m_printAction) {
        m_printAction->setEnabled(false);
    }

#ifdef Q_OS_LINUX
    QString fontFamily = QStringLiteral("'DejaVu Sans', 'Noto Sans', 'Helvetica Neue', Arial, sans-serif");
    QString defaultMonoFamily = QStringLiteral("DejaVu Sans Mono");
#else
    QString fontFamily = QStringLiteral("'Segoe UI', 'Helvetica Neue', Arial, sans-serif");
    QString defaultMonoFamily = QStringLiteral("Consolas");
#endif
    // Use the configured code block font for welcome page inline code too
    QString monoFamily = m_settings.value("editor/codeBlockFontFamily", defaultMonoFamily).toString();
    monoFamily = QStringLiteral("'%1', 'SFMono-Regular', 'DejaVu Sans Mono', Consolas, 'Liberation Mono', monospace").arg(monoFamily);

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
            <h1>Vibe-MD</h1>
            <p align="center" style="margin:0; line-height:1.2;">
                <img src=":/icon.png" width="64" height="64" alt="" title=""><br>
                <span style="font-size:1em; color:#7f8c8d;">A simple and elegant Markdown viewer</span>
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
                <li>YAML frontmatter is displayed in a styled block at the top</li>
            </ul>

            <div class="footer">
                <p>Version 1.2</p>
            </div>
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
        m_editor->print(&printer);
    }
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

    if (QFileInfo(filePath).suffix().toLower() == "txt") {
        m_editor->setPlainText(content);
    } else {
        // Resolve relative image paths against the markdown file's directory
        QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(filePath).absolutePath() + "/");
        m_editor->document()->setBaseUrl(baseUrl);

        m_editor->setMarkdown(processedContent);
        styleCodeBlocks();
    }

    m_currentFile = filePath;
    setWindowTitle(tr("%1 - Vibe-MD").arg(QFileInfo(filePath).fileName()));

    // Enable print action when a file is loaded
    if (m_printAction) {
        m_printAction->setEnabled(true);
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
        QString localPath = QDir::tempPath() + "/vibe-md_images/" + QString::fromLatin1(hash) + "." + ext;

        if (!QFile::exists(localPath)) {
            QDir().mkpath(QDir::tempPath() + "/vibe-md_images");
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

static bool blockIsCode(const QTextBlock &block)
{
    bool allMonospace = true;
    bool hasText = false;

    for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
        QTextFragment frag = it.fragment();
        if (!frag.isValid()) continue;
        hasText = true;
        QString family = frag.charFormat().font().family();
        bool isMono = frag.charFormat().font().fixedPitch() ||
                      family.contains("mono", Qt::CaseInsensitive) ||
                      family.contains("Courier", Qt::CaseInsensitive) ||
                      family.contains("Consolas", Qt::CaseInsensitive) ||
                      family.contains("Menlo", Qt::CaseInsensitive) ||
                      family.contains("Liberation", Qt::CaseInsensitive);
        if (!isMono) {
            allMonospace = false;
            break;
        }
    }
    return hasText && allMonospace;
}

void MainWindow::styleCodeBlocks()
{
    QTextDocument *doc = m_editor->document();
    if (!doc) return;

    // First pass: collect code-block flags so we can group consecutive blocks.
    QList<QTextBlock> blocks;
    QList<bool> isCode;
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        blocks.append(block);
        isCode.append(blockIsCode(block));
    }

    // Expand: empty blocks between two code blocks become code too.
    for (int i = 0; i < blocks.size(); ) {
        if (!isCode[i]) { ++i; continue; }

        int j = i + 1;
        while (j < blocks.size() && (isCode[j] || blocks[j].text().trimmed().isEmpty())) {
            if (!isCode[j]) isCode[j] = true;
            ++j;
        }
        i = j;
    }

    // Detect which consecutive code region (if any) is frontmatter.
    // Frontmatter is always the first code region and contains YAML keys.
    bool inFrontMatterRegion = false;
    for (int i = 0; i < blocks.size(); ++i) {
        if (!isCode[i]) continue;
        // Scan the first few blocks of this region for frontmatter markers.
        bool hasTitle = false, hasDate = false;
        int j = i;
        while (j < blocks.size() && isCode[j] && (j - i) < 6) {
            QString text = blocks[j].text();
            if (text.contains("title:")) hasTitle = true;
            if (text.contains("date:")) hasDate = true;
            ++j;
        }
        inFrontMatterRegion = hasTitle && hasDate;
        break; // only inspect the first code region
    }

#ifdef Q_OS_LINUX
    const QString defaultCodeBlockFontFamily = QStringLiteral("DejaVu Sans Mono");
#else
    const QString defaultCodeBlockFontFamily = QStringLiteral("Consolas");
#endif
    QString cbFamily = m_settings.value("editor/codeBlockFontFamily", defaultCodeBlockFontFamily).toString();
    int cbSize = m_settings.value("editor/codeBlockFontSize", 11).toInt();
    QFont codeBlockFont(cbFamily);
    codeBlockFont.setPointSize(cbSize);

    QTextCursor cursor(doc);
    cursor.beginEditBlock();

    // Second pass: style each block, collapsing margins for consecutive code lines.
    for (int i = 0; i < blocks.size(); ++i) {
        if (!isCode[i]) continue;

        // Remove from list if nested inside one (prevents bullet markers)
        if (QTextList *list = blocks[i].textList()) {
            int idx = list->itemNumber(blocks[i]);
            if (idx >= 0)
                list->removeItem(idx);
        }

        bool prevCode = (i > 0) && isCode[i - 1];
        bool nextCode = (i + 1 < blocks.size()) && isCode[i + 1];

        QTextBlockFormat bf = blocks[i].blockFormat();
        // Frontmatter gets a blue background; regular code blocks are gray.
        bf.setBackground(QColor(inFrontMatterRegion ? "#e3f2fd" : "#f4f4f4"));
        bf.setTopMargin(prevCode ? 0 : 8);
        bf.setBottomMargin(nextCode ? 0 : 8);
        bf.setLeftMargin(16);
        bf.setRightMargin(16);
        cursor.setPosition(blocks[i].position());
        cursor.setBlockFormat(bf);

        for (QTextBlock::iterator it = blocks[i].begin(); !it.atEnd(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;
            QTextCharFormat cf = frag.charFormat();
            cf.setForeground(QColor("#333333"));
            cf.setFont(codeBlockFont);
            cursor.setPosition(frag.position());
            cursor.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
            cursor.setCharFormat(cf);
        }
    }

    cursor.endEditBlock();

    // Also style inline code spans (fragments with monospace font inside non-code blocks)
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        // Skip blocks that are already styled as code blocks
        bool isBlockCode = false;
        for (int i = 0; i < blocks.size(); ++i) {
            if (blocks[i] == block && isCode[i]) {
                isBlockCode = true;
                break;
            }
        }
        if (isBlockCode) continue;

        // Find inline code fragments in non-code blocks
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;
            QTextCharFormat cf = frag.charFormat();
            // Check if this is inline code (monospace font and red color from CSS)
            if (cf.font().fixedPitch() || cf.font().family().contains("mono", Qt::CaseInsensitive)) {
                cf.setFont(codeBlockFont);
                cursor.setPosition(frag.position());
                cursor.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
                cursor.setCharFormat(cf);
            }
        }
    }
    cursor.endEditBlock();
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
    }
    return QMainWindow::eventFilter(obj, event);
}
