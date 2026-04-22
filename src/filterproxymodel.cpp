#include "filterproxymodel.h"
#include <QRegularExpression>

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void FilterProxyModel::setNameFilters(const QStringList &filters)
{
    m_nameFilters = filters;
    invalidateFilter();
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QFileSystemModel *fsm = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fsm) return true;

    QModelIndex index = fsm->index(sourceRow, 0, sourceParent);
    QString filePath = fsm->filePath(index);
    QFileInfo info(filePath);

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
    QDirIterator it(folderPath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isFile() && fileMatches(it.fileName())) {
            return true;
        }
    }
    return false;
}

bool FilterProxyModel::fileMatches(const QString &fileName) const
{
    for (const QString &filter : m_nameFilters) {
        QString pattern = filter;
        // Convert "*.md" style filter to wildcard match
        if (pattern.startsWith("*.")) {
            QString suffix = pattern.mid(2);
            if (fileName.endsWith(suffix, Qt::CaseInsensitive)) {
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
