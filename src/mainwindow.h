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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTreeView>
#include <QFileSystemModel>
#include <QSettings>
#include <QAction>
#include <QCloseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QRegularExpression>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include "filterproxymodel.h"

class FindDialog;
class SearchInDialog;
class Minimap;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openFile(const QString &filePath, bool loadFileFolder = true);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onOpenFile();
    void onOpenFolder();
    void onFileTreeClicked(const QModelIndex &index);
    void onFileTreeContextMenu(const QPoint &pos);
    void onEditorContextMenu(const QPoint &pos);
    void onOpenWithExternalEditor();
    void onRecentFileTriggered();
    void onRecentFolderTriggered();
    void onClearRecent();
    void onClearRecentFolders();
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void onToggleFileTree(bool visible);
    void onToggleMinimap(bool visible);
    void onToggleWordWrap(bool enabled);
    void onAbout();
    void onPrint();
    void onExportToPdf();
    void onPreferences();
    void onFind();
    void onFindNext();
    void onSearchInFiles();
    void onFileChanged(const QString &path);
    void onFileChangeDebounceTriggered();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupFileTree();
    void setupEditor();
    void loadFile(const QString &filePath);
    void loadFolder(const QString &folderPath, bool rememberAsLastFolder = true);
    void updateRecentFiles(const QString &filePath);
    void updateRecentFolders(const QString &folderPath);
    void refreshRecentFilesMenu();
    void refreshRecentFoldersMenu();

    bool isMarkdownFile(const QString &filePath) const;
    bool folderHasValidFiles(const QString &folderPath);
    QString resolveExternalImages(const QString &markdownContent, bool previewEnabled);
    QString resolveRelativeImages(const QString &markdownContent, const QString &basePath);
    QString resolveFrontMatter(const QString &markdownContent);
    void applyEditorFont();
    void showWelcomePage();

    QTextEdit *m_editor = nullptr;
    QTreeView *m_fileTree = nullptr;
    QFileSystemModel *m_fileModel = nullptr;
    FilterProxyModel *m_proxyModel = nullptr;
    QSplitter *m_splitter = nullptr;

    QMenu *m_recentMenu = nullptr;
    QMenu *m_recentFoldersMenu = nullptr;
    QList<QAction*> m_recentActions;
    QList<QAction*> m_recentFolderActions;
    QAction *m_clearRecentAction = nullptr;
    QAction *m_clearRecentFoldersAction = nullptr;
    QAction *m_printAction = nullptr;
    QAction *m_exportPdfAction = nullptr;
    QAction *m_findAction = nullptr;
    QAction *m_findNextAction = nullptr;
    QAction *m_showFileTreeAction = nullptr;
    QAction *m_showMinimapAction = nullptr;
    QAction *m_wordWrapAction = nullptr;

    QString m_currentFile;
    QString m_currentFolder;
    QSettings m_settings;
    QHash<QString, QString> m_imageUrlMap; // localPath -> originalUrl
    FindDialog *m_findDialog = nullptr;
    SearchInDialog *m_searchInDialog = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    Minimap *m_minimap = nullptr;
    QWidget *m_editorContainer = nullptr;

    // Status bar widgets
    QLabel *m_statusFileType = nullptr;
    QLabel *m_statusEncoding = nullptr;
    QPushButton *m_statusWrapBtn = nullptr;  // Word wrap toggle button
    QProgressBar *m_statusProgress = nullptr;  // Folder loading progress
    QLabel *m_statusWordCount = nullptr;
    QLabel *m_statusZoom = nullptr;
    QPushButton *m_toggleFileTreeBtn = nullptr;
    QPushButton *m_toggleMinimapBtn = nullptr;
    QLabel *m_statusFileMsg = nullptr;  // Shows "Loaded: ..." message
    QTimer *m_statusMsgTimer = nullptr; // Clears the file message after delay
    QTimer *m_fileChangeDebounceTimer = nullptr; // Debounce external file change notifications
    bool m_fileChangeDialogOpen = false; // Prevent multiple file change dialogs

    // Zoom tracking (percentage, 100 = default)
    int m_zoomLevel = 100;
    int m_baseFontSize = 11;  // Default font size for zoom calculation
    
    // Detected file encoding for status bar display
    QString m_detectedEncoding = QStringLiteral("UTF-8");

    // Last search text (from "Search in Files" or Find dialog) for F3 "Find Next"
    QString m_lastSearchText;

    void setupStatusBar();
    void updateStatusBar();
    void updateZoomDisplay();
    int countWords(const QString &text) const;
    void highlightSearchText(const QString &text);
};

#endif // MAINWINDOW_H
