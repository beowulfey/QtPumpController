// tablemodel.cpp
#include "tablemodel.h"

TableModel::TableModel(QObject* parent)
    : QAbstractTableModel(parent),
    columnHeaders({"Time (min)", "[Start] (mM)", "[End] (mM)"}) {}

int TableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(tableData.size());
}

int TableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(columnHeaders.size());
}

QVariant TableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    if (role == Qt::DisplayRole) {
        return tableData[index.row()][index.column()];
    }

    return QVariant();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();

    if (orientation == Qt::Horizontal) {
        return columnHeaders[section];
    } else {
        return QString::number(section);
    }
}

bool TableModel::insertRows(int row, int count, const QModelIndex& parent) {
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        tableData.insert(tableData.begin() + row, std::vector<QString>(columnHeaders.size(), "<empty>"));
    }
    endInsertRows();
    return true;
}

bool TableModel::removeRows(int position, int rows, const QModelIndex& parent) {
    if (position < 0 || position + rows > static_cast<int>(tableData.size())) return false;
    beginRemoveRows(parent, position, position + rows - 1);
    tableData.erase(tableData.begin() + position, tableData.begin() + position + rows);
    endRemoveRows();
    return true;
}

void TableModel::addSegment(const std::vector<QString>& segment) {
    if (segment.size() != columnHeaders.size()) return;
    beginInsertRows(QModelIndex(), static_cast<int>(tableData.size()), static_cast<int>(tableData.size()));
    tableData.push_back(segment);
    endInsertRows();
}

void TableModel::removeSegment(int pos) {
    if (tableData.empty()) return;
    if (pos < 0 || pos >= static_cast<int>(tableData.size())) {
        pos = static_cast<int>(tableData.size()) - 1;
    }
    beginRemoveRows(QModelIndex(), pos, pos);
    tableData.erase(tableData.begin() + pos);
    endRemoveRows();
}

std::vector<std::vector<QString>> TableModel::getSegments() const {
    return tableData;
}

void TableModel::clearSegments() {
    beginResetModel();
    tableData.clear();
    endResetModel();
}
