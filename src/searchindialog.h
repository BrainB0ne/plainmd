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

#ifndef SEARCHINDIALOG_H
#define SEARCHINDIALOG_H

#include <QDialog>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class SearchInDialog; }
QT_END_NAMESPACE

class SearchInDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchInDialog(const QString &folderPath, QWidget *parent = nullptr);
    ~SearchInDialog();

    void setFolderPath(const QString &folderPath);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void fileSelected(const QString &filePath, const QString &searchText);

private slots:
    void on_searchButton_clicked();
    void on_resultItem_clicked(QListWidgetItem *item);
    void on_searchText_changed(const QString &text);
    void on_resultItem_doubleClicked(QListWidgetItem *item);

private:
    void searchFiles();
    QStringList getSearchableFiles() const;
    bool fileContainsText(const QString &filePath, const QString &searchText) const;
    int countMatchesInFile(const QString &filePath, const QString &searchText) const;
    QString getSnippet(const QString &filePath, const QString &searchText) const;

    Ui::SearchInDialog *ui;
    
    QString m_folderPath;
    QString m_selectedFile;
    QString m_searchText;
    
    static const int MAX_RESULTS = 100;
    static const int SNIPPET_LENGTH = 60;
};

#endif // SEARCHINDIALOG_H
