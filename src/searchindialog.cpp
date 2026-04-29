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

#include "searchindialog.h"
#include "ui_searchindialog.h"

#include <QEvent>
#include <QKeyEvent>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

SearchInDialog::SearchInDialog(const QString &folderPath, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SearchInDialog)
    , m_folderPath(folderPath)
{
    ui->setupUi(this);
    setWindowTitle(tr("Search in Files"));
    resize(500, 400);
    
    ui->statusLabel->setText(tr("Ready to search in: %1").arg(QDir::toNativeSeparators(m_folderPath)));
    
    // Connect UI signals
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &SearchInDialog::on_searchText_changed);
    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &SearchInDialog::on_searchButton_clicked);
    connect(ui->searchButton, &QPushButton::clicked, this, &SearchInDialog::on_searchButton_clicked);
    connect(ui->resultsList, &QListWidget::itemClicked, this, &SearchInDialog::on_resultItem_clicked);
    connect(ui->resultsList, &QListWidget::itemDoubleClicked, this, &SearchInDialog::on_resultItem_doubleClicked);
    
    // Install event filter for keyboard navigation
    ui->resultsList->installEventFilter(this);
}

SearchInDialog::~SearchInDialog()
{
    delete ui;
}

void SearchInDialog::setFolderPath(const QString &folderPath)
{
    m_folderPath = folderPath;
    ui->statusLabel->setText(tr("Ready to search in: %1").arg(QDir::toNativeSeparators(m_folderPath)));
}

void SearchInDialog::on_searchText_changed(const QString &text)
{
    ui->searchButton->setEnabled(!text.isEmpty());
}

void SearchInDialog::on_searchButton_clicked()
{
    searchFiles();
}

void SearchInDialog::on_resultItem_clicked(QListWidgetItem *item)
{
    if (!item) return;
    
    // Single click just selects - store the file path for later
    m_selectedFile = item->data(Qt::UserRole).toString();
    m_searchText = ui->searchEdit->text();
    // Don't emit here - wait for double-click or Enter
}

void SearchInDialog::on_resultItem_doubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    // Double-click emits and hides dialog
    m_selectedFile = item->data(Qt::UserRole).toString();
    m_searchText = ui->searchEdit->text();
    
    if (!m_selectedFile.isEmpty()) {
        emit fileSelected(m_selectedFile, m_searchText);
        hide();  // Hide but keep results for later
    }
}

bool SearchInDialog::eventFilter(QObject *watched, QEvent *event)
{
    // Handle Enter key in results list to open selected file
    if (watched == ui->resultsList && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QListWidgetItem *currentItem = ui->resultsList->currentItem();
            if (currentItem) {
                on_resultItem_doubleClicked(currentItem);
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void SearchInDialog::searchFiles()
{
    QString searchText = ui->searchEdit->text();
    if (searchText.isEmpty()) {
        return;
    }
    
    m_searchText = searchText;
    ui->resultsList->clear();
    ui->statusLabel->setText(tr("Searching in: %1...").arg(QDir::toNativeSeparators(m_folderPath)));
    ui->searchButton->setEnabled(false);
    
    QStringList files = getSearchableFiles();
    int matchCount = 0;
    
    for (const QString &filePath : files) {
        if (fileContainsText(filePath, searchText)) {
            int matches = countMatchesInFile(filePath, searchText);
            QString snippet = getSnippet(filePath, searchText);
            
            QString displayText = QString("%1 (%2 matches)\n%3")
                .arg(QDir::toNativeSeparators(filePath))
                .arg(matches)
                .arg(snippet);
            
            QListWidgetItem *item = new QListWidgetItem(displayText, ui->resultsList);
            item->setData(Qt::UserRole, filePath);
            item->setToolTip(filePath);
            ui->resultsList->addItem(item);
            
            matchCount++;
            if (matchCount >= MAX_RESULTS) {
                break;
            }
        }
    }
    
    if (matchCount == 0) {
        ui->statusLabel->setText(tr("No matches found in: %1").arg(QDir::toNativeSeparators(m_folderPath)));
    } else if (matchCount >= MAX_RESULTS) {
        ui->statusLabel->setText(tr("Found %1+ matches in: %2 (showing first %1)").arg(MAX_RESULTS).arg(QDir::toNativeSeparators(m_folderPath)));
    } else {
        ui->statusLabel->setText(tr("Found %1 matches in: %2").arg(matchCount).arg(QDir::toNativeSeparators(m_folderPath)));
    }
    
    ui->searchButton->setEnabled(true);
}

QStringList SearchInDialog::getSearchableFiles() const
{
    QStringList files;
    QDirIterator it(m_folderPath, QStringList() << "*.md" << "*.markdown" << "*.mdx" << "*.txt", 
                     QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        files.append(it.next());
    }
    
    return files;
}

bool SearchInDialog::fileContainsText(const QString &filePath, const QString &searchText) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    QString content = stream.readAll().toLower();
    file.close();
    
    return content.contains(searchText.toLower());
}

int SearchInDialog::countMatchesInFile(const QString &filePath, const QString &searchText) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    
    QTextStream stream(&file);
    QString content = stream.readAll().toLower();
    file.close();
    
    QString lowerSearch = searchText.toLower();
    int count = 0;
    int pos = 0;
    
    while ((pos = content.indexOf(lowerSearch, pos)) != -1) {
        count++;
        pos += lowerSearch.length();
    }
    
    return count;
}

QString SearchInDialog::getSnippet(const QString &filePath, const QString &searchText) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();
    
    QString lowerContent = content.toLower();
    QString lowerSearch = searchText.toLower();
    
    int pos = lowerContent.indexOf(lowerSearch);
    if (pos == -1) {
        return QString();
    }
    
    // Calculate snippet boundaries
    int start = qMax(0, pos - SNIPPET_LENGTH / 2);
    int end = qMin(content.length(), pos + lowerSearch.length() + SNIPPET_LENGTH / 2);
    
    QString snippet = content.mid(start, end - start);
    
    // Add ellipsis if truncated
    if (start > 0) {
        snippet = "..." + snippet;
    }
    if (end < content.length()) {
        snippet = snippet + "...";
    }
    
    // Replace newlines with spaces for display
    snippet.replace('\n', ' ');
    snippet.replace('\r', ' ');
    
    return snippet.trimmed();
}
