#ifndef FILTERPROXYMODEL_H
#define FILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QDir>
#include <QDirIterator>

class FilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FilterProxyModel(QObject *parent = nullptr);

    void setNameFilters(const QStringList &filters);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool hasMatchingFiles(const QString &folderPath) const;
    bool fileMatches(const QString &fileName) const;

    QStringList m_nameFilters;
};

#endif // FILTERPROXYMODEL_H
