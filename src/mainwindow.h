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
#include "filterproxymodel.h"

class FindDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openFile(const QString &filePath);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onOpenFile();
    void onOpenFolder();
    void onFileTreeClicked(const QModelIndex &index);
    void onFileTreeContextMenu(const QPoint &pos);
    void onOpenWithExternalEditor();
    void onRecentFileTriggered();
    void onRecentFolderTriggered();
    void onClearRecent();
    void onClearRecentFolders();
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void onAbout();
    void onPrint();
    void onExportToPdf();
    void onPreferences();
    void onFind();
    void onFileChanged(const QString &path);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupFileTree();
    void setupEditor();
    void loadFile(const QString &filePath);
    void loadFolder(const QString &folderPath);
    void updateRecentFiles(const QString &filePath);
    void updateRecentFolders(const QString &folderPath);
    void refreshRecentFilesMenu();
    void refreshRecentFoldersMenu();

    bool isMarkdownFile(const QString &filePath) const;
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

    QString m_currentFile;
    QString m_currentFolder;
    QSettings m_settings;
    QHash<QString, QString> m_imageUrlMap; // localPath -> originalUrl
    FindDialog *m_findDialog = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
};

#endif // MAINWINDOW_H
