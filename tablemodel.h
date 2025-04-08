#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#pragma once

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QStringList>
#include <QHash>
#include <QByteArray>

#include <vector>


class TableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit TableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex& parent = QModelIndex()) override;

    void addSegment(double timeMinutes, int startConc, int endConc);
    void removeSegment(int pos = -1);
    std::vector<std::vector<QString>> getSegments() const;
    void clearSegments();

private:
    std::vector<QString> columnHeaders;
    std::vector<std::vector<QString>> tableData;
};

#endif // TABLEMODEL_H
