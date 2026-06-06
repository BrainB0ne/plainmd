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

#include "filterproxymodel.h"
#include <QRegularExpression>
#include <functional>

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void FilterProxyModel::setNameFilters(const QStringList &filters)
{
    m_nameFilters = filters;
    invalidateFilter();
}

void FilterProxyModel::setExemptPath(const QString &path)
{
    m_exemptPath = QDir::cleanPath(path);
    invalidateFilter();
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QFileSystemModel *fsm = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fsm) return true;

    QModelIndex index = fsm->index(sourceRow, 0, sourceParent);
    QString filePath = fsm->filePath(index);
    QFileInfo info(filePath);

    if (QDir::cleanPath(filePath) == m_exemptPath) {
        return true;
    }

    if (info.isFile()) {
        return fileMatches(info.fileName());
    }

    if (info.isDir()) {
        return hasMatchingFiles(filePath);
    }

    return true;
}

bool FilterProxyModel::hasMatchingFiles(const QString &folderPath) const
{
    const int maxDepth = 3;
    const int maxEntries = 1000;
    int entriesVisited = 0;

    std::function<bool(const QString&, int)> checkDirectory;
    checkDirectory = [&](const QString &path, int depth) -> bool {
        if (depth > maxDepth || entriesVisited >= maxEntries) {
            return false;
        }

        QDir dir(path);
        const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entry : entries) {
            if (++entriesVisited > maxEntries) {
                return false;
            }

            if (entry.isFile() && fileMatches(entry.fileName())) {
                return true;
            }

            if (entry.isDir() && checkDirectory(entry.filePath(), depth + 1)) {
                return true;
            }
        }

        return false;
    };

    return checkDirectory(folderPath, 0);
}

bool FilterProxyModel::fileMatches(const QString &fileName) const
{
    for (const QString &filter : m_nameFilters) {
        QString pattern = filter;
        // Convert "*.md" style filter to extension match
        if (pattern.startsWith("*.")) {
            QString suffix = pattern.mid(2);
            if (QFileInfo(fileName).suffix().compare(suffix, Qt::CaseInsensitive) == 0) {
                return true;
            }
        } else {
            QRegularExpression re(QRegularExpression::wildcardToRegularExpression(pattern),
                                  QRegularExpression::CaseInsensitiveOption);
            if (re.match(fileName).hasMatch()) {
                return true;
            }
        }
    }
    return false;
}
