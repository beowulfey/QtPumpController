// tablemodel.cpp
#include "tablemodel.h"
#include <QMimeData>
#include <QIODevice>

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
        tableData.insert(tableData.begin() + row, QVector<QString>(columnHeaders.size(), "<empty>"));
    }
    endInsertRows();
    return true;
}

bool TableModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                          const QModelIndex &destinationParent, int destinationChild) {
    if (sourceRow < 0 || destinationChild < 0 ||
        sourceRow >= static_cast<int>(tableData.size()) ||
        destinationChild > static_cast<int>(tableData.size()) ||
        sourceRow == destinationChild || count != 1) {
        return false;
    }

    beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild);

    auto movedRow = tableData[sourceRow];
    tableData.erase(tableData.begin() + sourceRow);

    // adjust destination index if it follows the removed row
    if (destinationChild > sourceRow)
        destinationChild -= 1;

    tableData.insert(tableData.begin() + destinationChild, movedRow);

    endMoveRows();
    return true;
}


bool TableModel::removeRows(int position, int rows, const QModelIndex& parent) {
    if (position < 0 || position + rows > static_cast<int>(tableData.size())) return false;
    beginRemoveRows(parent, position, position + rows - 1);
    tableData.erase(tableData.begin() + position, tableData.begin() + position + rows);
    endRemoveRows();
    return true;
}





void TableModel::addSegment(double timeMinutes, int startConc, int endConc, int insertRow) {
    // Clamp the insert row
    if (insertRow < 0 || insertRow > static_cast<int>(tableData.size()))
        insertRow = static_cast<int>(tableData.size());

    beginInsertRows(QModelIndex(), insertRow, insertRow);
    QVector<QString> row;
    row.push_back(QString::number(timeMinutes));
    row.push_back(QString::number(startConc));
    row.push_back(QString::number(endConc));
    tableData.insert(tableData.begin() + insertRow, row);
    endInsertRows();
    emit segmentsChanged();
}

void TableModel::removeSegment(int pos) {
    if (tableData.empty()) return;
    if (pos < 0 || pos >= static_cast<int>(tableData.size())) {
        pos = static_cast<int>(tableData.size()) - 1;
    }
    beginRemoveRows(QModelIndex(), pos, pos);
    tableData.erase(tableData.begin() + pos);
    endRemoveRows();
    emit segmentsChanged();
}

QVector<QVector<double>> TableModel::getSegments() const {
    QVector<QVector<double>> numericSegments;

    for (const auto& row : tableData) {
        QVector<double> numericRow;
        for (const auto& cell : row) {
            bool ok;
            double val = cell.toDouble(&ok);
            numericRow.push_back(ok ? val : 0.0);  // Optionally handle invalid values
        }
        numericSegments.push_back(numericRow);
    }

    return numericSegments;
}

void TableModel::clearSegments() {
    beginResetModel();
    tableData.clear();
    endResetModel();
    emit segmentsChanged();
}

void TableModel::updateSegments() {
    emit segmentsChanged();
}
