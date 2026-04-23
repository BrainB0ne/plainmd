#include "mainwindow.h"
#include "filterproxymodel.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QApplication::organizationName(), QApplication::applicationName())
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setAcceptDrops(true);
    setWindowTitle(tr("Markdown Viewer"));
    setWindowIcon(QIcon(":/icon.png"));
    resize(1200, 800);

    QString lastFolder = m_settings.value("lastFolder").toString();
    if (!lastFolder.isEmpty() && QDir(lastFolder).exists()) {
        loadFolder(lastFolder);
    }

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
    m_fileTree->setHeaderHidden(true);
    m_fileTree->setSortingEnabled(true);
    m_fileTree->setAnimated(true);
    m_fileTree->setMinimumWidth(200);
    m_fileTree->setMaximumWidth(400);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setNameFilters(QStringList() << "*.md" << "*.markdown" << "*.mdx" << "*.txt");
    m_proxyModel->setSourceModel(m_fileModel);

    m_fileTree->setModel(m_proxyModel);
    m_fileTree->hideColumn(1);
    m_fileTree->hideColumn(2);
    m_fileTree->hideColumn(3);
    m_fileTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    connect(m_fileTree, &QTreeView::clicked, this, &MainWindow::onFileTreeClicked);

    m_splitter->addWidget(m_fileTree);
}

void MainWindow::setupEditor()
{
    m_editor = new QTextEdit(this);
    m_editor->setReadOnly(true);
    m_editor->document()->setDocumentMargin(16);
    m_editor->setAcceptRichText(true);

    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(11);
    m_editor->setFont(font);

    setMarkdownStyle();

    m_splitter->addWidget(m_editor);
    m_splitter->setStretchFactor(1, 1);
}

void MainWindow::setMarkdownStyle()
{
    QString style = R"(
        body {
            font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif;
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
            font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace;
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
    )";

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

    QAction *openAction = new QAction(QIcon(":/images/file-open.png"), tr("Open File"), this);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    toolBar->addAction(openAction);

    QAction *openFolderAction = new QAction(QIcon(":/images/folder-open.png"), tr("Open Folder"), this);
    connect(openFolderAction, &QAction::triggered, this, &MainWindow::onOpenFolder);
    toolBar->addAction(openFolderAction);

    toolBar->addSeparator();

    QAction *zoomInAction = new QAction(QIcon(":/images/zoom-in.png"), tr("Zoom In"), this);
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::onZoomIn);
    toolBar->addAction(zoomInAction);

    QAction *zoomOutAction = new QAction(QIcon(":/images/zoom-out.png"), tr("Zoom Out"), this);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::onZoomOut);
    toolBar->addAction(zoomOutAction);
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
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(11);
    m_editor->setFont(font);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About Markdown Viewer"),
        tr("<h2>Markdown Viewer</h2>"
           "<p>A simple and elegant Markdown viewer built with Qt6.</p>"
           "<p>Features:</p>"
           "<ul>"
           "<li>Native Qt6 markdown rendering</li>"
           "<li>File browser sidebar</li>"
           "<li>Drag and drop support</li>"
           "<li>Recent files</li>"
           "<li>Zoom controls</li>"
           "</ul>"
           "<p>Built with qmake and Qt6.</p>"
           "<p>Icons by <a href='https://tabler.io/icons'>Tabler Icons</a> (MIT License).</p>"));
}

void MainWindow::onAboutQt()
{
    QMessageBox::aboutQt(this);
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

    updateRecentFiles(filePath);
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

    QString processedContent = resolveExternalImages(content);

    m_editor->clear();
    m_editor->setMarkdown(processedContent);
    styleCodeBlocks();

    // Resolve relative image paths against the markdown file's directory
    QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(filePath).absolutePath() + "/");
    m_editor->document()->setBaseUrl(baseUrl);

    m_currentFile = filePath;
    setWindowTitle(tr("%1 - Markdown Viewer").arg(QFileInfo(filePath).fileName()));

    // Select the file in the tree if visible
    if (m_fileTree && m_fileModel && m_proxyModel) {
        QModelIndex sourceIndex = m_fileModel->index(filePath);
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);
        if (proxyIndex.isValid()) {
            m_fileTree->setCurrentIndex(proxyIndex);
            m_fileTree->scrollTo(proxyIndex);
        }
    }

    statusBar()->showMessage(tr("Loaded: %1").arg(filePath), 3000);
}

void MainWindow::loadFolder(const QString &folderPath)
{
    if (!QDir(folderPath).exists()) return;

    m_currentFolder = folderPath;
    m_fileModel->setRootPath(folderPath);
    QModelIndex sourceRoot = m_fileModel->index(folderPath);
    QModelIndex proxyRoot = m_proxyModel->mapFromSource(sourceRoot);
    m_fileTree->setRootIndex(proxyRoot);
    m_settings.setValue("lastFolder", folderPath);
}

void MainWindow::updateRecentFiles(const QString &filePath)
{
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

    QStringList recentFiles = m_settings.value("recentFiles").toStringList();

    if (recentFiles.isEmpty()) {
        QAction *emptyAction = new QAction(tr("No Recent Files"), this);
        emptyAction->setEnabled(false);
        m_recentMenu->addAction(emptyAction);
    } else {
        for (int i = 0; i < recentFiles.size(); ++i) {
            QString filePath = recentFiles.at(i);
            QString displayName = QString("&%1 %2").arg(i + 1).arg(QFileInfo(filePath).fileName());

            QAction *action = new QAction(displayName, this);
            action->setData(filePath);
            action->setToolTip(filePath);
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

QString MainWindow::resolveExternalImages(const QString &markdownContent)
{
    QString result = markdownContent;

    // Regex for Markdown images: ![alt](url) or ![alt](url "title")
    QRegularExpression mdRe(QStringLiteral(R"(!\[[^\]]*\]\((https?://[^)\s\"]+)[^)]*\))"));
    // Regex for HTML <img> tags: <img src="url" or <img src='url'
    QRegularExpression htmlRe(QStringLiteral(R"(<img[^>]+src\s*=\s*["'](https?://[^"']+)["'])"));

    struct Match {
        qsizetype pos;
        qsizetype len;
        QString url;
    };
    QList<Match> matches;

    auto collectMatches = [&matches](const QRegularExpression &re, const QString &text) {
        QRegularExpressionMatchIterator it = re.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            matches.append({m.capturedStart(1), m.capturedLength(1), m.captured(1)});
        }
    };

    collectMatches(mdRe, result);
    collectMatches(htmlRe, result);

    if (matches.isEmpty()) return result;

    // Sort by position descending so replacements do not shift earlier indices
    std::sort(matches.begin(), matches.end(),
              [](const Match &a, const Match &b) { return a.pos > b.pos; });

    QString cacheDir = QDir::tempPath() + "/mdviewer_images";
    QDir().mkpath(cacheDir);

    QNetworkAccessManager nam;

    for (const Match &match : matches) {
        QString urlStr = match.url;
        QString ext = QFileInfo(QUrl(urlStr).path()).suffix();
        if (ext.isEmpty()) ext = "png";

        QByteArray hash = QCryptographicHash::hash(urlStr.toUtf8(), QCryptographicHash::Md5).toHex();
        QString localPath = cacheDir + "/" + QString::fromLatin1(hash) + "." + ext;

        if (!QFile::exists(localPath)) {
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
            result.replace(match.pos, match.len, localPath);
        }
    }

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
        bf.setBackground(QColor("#f4f4f4"));
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
            cursor.setPosition(frag.position());
            cursor.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
            cursor.setCharFormat(cf);
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
