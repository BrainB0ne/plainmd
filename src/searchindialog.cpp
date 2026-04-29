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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QEvent>
#include <QKeyEvent>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QKeyEvent>

SearchInDialog::SearchInDialog(const QString &folderPath, QWidget *parent)
    : QDialog(parent)
    , m_folderPath(folderPath)
{
    setWindowTitle(tr("Search in Files"));
    resize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Search row
    QHBoxLayout *searchLayout = new QHBoxLayout();
    
    QLabel *searchLabel = new QLabel(tr("Search:"), this);
    searchLayout->addWidget(searchLabel);
    
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Enter search text..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SearchInDialog::on_searchText_changed);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SearchInDialog::on_searchButton_clicked);
    searchLayout->addWidget(m_searchEdit, 1);
    
    m_searchButton = new QPushButton(tr("Search"), this);
    m_searchButton->setDefault(true);
    m_searchButton->setEnabled(false);
    connect(m_searchButton, &QPushButton::clicked, this, &SearchInDialog::on_searchButton_clicked);
    searchLayout->addWidget(m_searchButton);
    
    mainLayout->addLayout(searchLayout);

    // Status label
    m_statusLabel = new QLabel(tr("Ready to search in: %1").arg(QDir::toNativeSeparators(m_folderPath)), this);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    // Results list
    m_resultsList = new QListWidget(this);
    m_resultsList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_resultsList, &QListWidget::itemClicked, this, &SearchInDialog::on_resultItem_clicked);
    connect(m_resultsList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        on_resultItem_clicked(item);
        if (!m_selectedFile.isEmpty()) {
            emit fileSelected(m_selectedFile, m_searchText);
            hide();  // Hide but keep results for later
        }
    });
    // Handle Enter key in results list
    m_resultsList->installEventFilter(this);
    mainLayout->addWidget(m_resultsList, 1);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton *closeButton = new QPushButton(tr("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        hide();  // Hide but keep results for later
    });
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);

    m_searchEdit->setFocus();
}

SearchInDialog::~SearchInDialog()
{
}

void SearchInDialog::on_searchButton_clicked()
{
    searchFiles();
}

void SearchInDialog::on_searchText_changed(const QString &text)
{
    m_searchButton->setEnabled(!text.isEmpty());
}

void SearchInDialog::on_resultItem_clicked(QListWidgetItem *item)
{
    if (item) {
        m_selectedFile = item->data(Qt::UserRole).toString();
    }
}

QStringList SearchInDialog::getSearchableFiles() const
{
    QStringList files;
    QDir dir(m_folderPath);
    
    QStringList nameFilters;
    nameFilters << "*.md" << "*.markdown" << "*.mdx" << "*.txt";
    
    QDirIterator it(dir.path(), nameFilters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        files.append(it.filePath());
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
    stream.setEncoding(QStringConverter::Utf8);
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
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll().toLower();
    file.close();
    
    QString searchLower = searchText.toLower();
    int count = 0;
    int pos = 0;
    
    while ((pos = content.indexOf(searchLower, pos)) != -1) {
        count++;
        pos += searchLower.length();
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
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    file.close();
    
    // Find the search text in the content (case-insensitive)
    QString contentLower = content.toLower();
    QString searchLower = searchText.toLower();
    int pos = contentLower.indexOf(searchLower);
    
    if (pos == -1) {
        return QString();
    }
    
    // Extract snippet around the match
    int start = qMax(0, pos - SNIPPET_LENGTH / 2);
    int end = qMin(content.length(), pos + searchText.length() + SNIPPET_LENGTH / 2);
    QString snippet = content.mid(start, end - start);
    
    // Add ellipsis if truncated
    if (start > 0) {
        snippet = "..." + snippet;
    }
    if (end < content.length()) {
        snippet = snippet + "...";
    }
    
    // Clean up newlines for display
    snippet.replace(QRegularExpression("\\s+"), " ");
    
    return snippet.trimmed();
}

void SearchInDialog::searchFiles()
{
    QString searchText = m_searchEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return;
    }
    
    m_searchText = searchText;  // Store for retrieval when opening file
    m_resultsList->clear();
    m_statusLabel->setText(tr("Searching for \"%1\"...").arg(searchText));
    
    QStringList files = getSearchableFiles();
    int matchCount = 0;
    
    for (const QString &filePath : files) {
        if (fileContainsText(filePath, searchText)) {
            QString relativePath = QDir(m_folderPath).relativeFilePath(filePath);
            QString snippet = getSnippet(filePath, searchText);
            int fileMatches = countMatchesInFile(filePath, searchText);
            
            QString displayText = relativePath;
            // Add match count
            if (fileMatches > 1) {
                displayText += tr(" (%1 matches)").arg(fileMatches);
            } else {
                displayText += tr(" (1 match)");
            }
            if (!snippet.isEmpty()) {
                displayText += "\n    " + snippet;
            }
            
            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, filePath);
            item->setToolTip(QDir::toNativeSeparators(filePath));
            m_resultsList->addItem(item);
            
            matchCount++;
            if (matchCount >= MAX_RESULTS) {
                break;
            }
        }
    }
    
    if (matchCount == 0) {
        m_statusLabel->setText(tr("No matches found for \"%1\"").arg(searchText));
    } else if (matchCount >= MAX_RESULTS) {
        m_statusLabel->setText(tr("Found %1+ matches for \"%2\" (showing first %1)").arg(MAX_RESULTS).arg(searchText));
    } else {
        m_statusLabel->setText(tr("Found %1 matches for \"%2\"").arg(matchCount).arg(searchText));
    }
}

void SearchInDialog::setFolderPath(const QString &folderPath)
{
    if (m_folderPath != folderPath) {
        m_folderPath = folderPath;
        m_statusLabel->setText(tr("Ready to search in: %1").arg(QDir::toNativeSeparators(m_folderPath)));
        // Clear previous results when folder changes
        m_resultsList->clear();
        m_searchText.clear();
    }
}

bool SearchInDialog::eventFilter(QObject *watched, QEvent *event)
{
    // Handle Enter key in results list to open selected file
    if (watched == m_resultsList && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QListWidgetItem *item = m_resultsList->currentItem();
            if (item) {
                on_resultItem_clicked(item);
                if (!m_selectedFile.isEmpty()) {
                    emit fileSelected(m_selectedFile, m_searchText);
                    hide();  // Hide but keep results for later
                }
            }
            return true;  // Event handled
        }
    }
    return QDialog::eventFilter(watched, event);
}
