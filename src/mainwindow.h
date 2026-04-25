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
    void onClearRecent();
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void onAbout();
    void onAboutQt();
    void onPrint();
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
    void refreshRecentFilesMenu();
    void setMarkdownStyle();
    bool isMarkdownFile(const QString &filePath) const;
    QString resolveExternalImages(const QString &markdownContent, bool previewEnabled);
    QString resolveRelativeImages(const QString &markdownContent, const QString &basePath);
    QString resolveFrontMatter(const QString &markdownContent);
    void styleCodeBlocks();
    void applyEditorFont();
    void showWelcomePage();

    QTextEdit *m_editor = nullptr;
    QTreeView *m_fileTree = nullptr;
    QFileSystemModel *m_fileModel = nullptr;
    FilterProxyModel *m_proxyModel = nullptr;
    QSplitter *m_splitter = nullptr;

    QMenu *m_recentMenu = nullptr;
    QList<QAction*> m_recentActions;
    QAction *m_clearRecentAction = nullptr;
    QAction *m_printAction = nullptr;

    QString m_currentFile;
    QString m_currentFolder;
    QSettings m_settings;
    QHash<QString, QString> m_imageUrlMap; // localPath -> originalUrl
    FindDialog *m_findDialog = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
};

#endif // MAINWINDOW_H
