#include "csvtable.h"
#include <QDebug>

CSVTable::CSVTable(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant CSVTable::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole){
        if(orientation == Qt::Vertical){
            if(section < vHeader.length()){
                return vHeader.at(section);
            }else{
                return section;
            }
        }else{
            if(section < hHeader.length()){
                return hHeader.at(section);
            }else{
                return section;
            }
        }
    }
    return QVariant();
}

bool CSVTable::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    //header should NOT be changed before inserting rows, but after.
    if (value != headerData(section, orientation, role)) {
        if(orientation == Qt::Vertical){
            if(section < vHeader.length()){
                vHeader.replace(section, value);
                emit headerDataChanged(orientation, section, section);
                return true;
            }
        }else{
            if(section < hHeader.length()){
                hHeader.replace(section, value);
                emit headerDataChanged(orientation, section, section);
                return true;
            }
        }
    }
    return false;
}


int CSVTable::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);   //parrent is not supported yet
    return storage.length();
}

int CSVTable::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);   //parrent is not supported yet
    if(storage.length() > 0)
        return storage.at(0).length();
    return 0;
}

QVariant CSVTable::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    switch (role) {
        case Qt::DisplayRole:
            return storage.at(index.row()).at(index.column());
        default:
            return QVariant(); //other roles not supported yet
    }
    qDebug() << "last";
    return QVariant();
}

bool CSVTable::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && data(index, role) != value) { //mb additional bounds check needed
        QVector<QVariant> tmp = storage.at(index.row());
        tmp.replace(index.column(), value);
        storage.replace(index.row(), tmp);
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags CSVTable::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable; //other flags not supported yet
}

bool CSVTable::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);   //parrent is not supported yet
    if(row < 0){
        row = 0;
    }else if(row > rowCount()){
        row = rowCount();
    }
    if(count > 0){
        beginInsertRows(parent, row, row + count - 1);
        storage.insert(row, count, QVector<QVariant>(columnCount()));
        while(count > 0){
            vHeader.insert(row, QVariant(row + count));
            count--;
        }
        endInsertRows();
        return true;
    }
    return false;
    //header should be modyfied manually if needed
}

bool CSVTable::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    //parrent is not supported yet
    if(column < 0){
        column = 0;
    }else if(column >= columnCount()){
        column = columnCount();
    }
    if(count > 0){
        beginInsertColumns(parent, column, column + count - 1);
        int curr = 0;
        foreach (QVector<QVariant> row, storage) {
            row.insert(column, count, QVariant());
            storage.replace(curr, row);
            curr++;
        }
        while(count > 0){
            hHeader.insert(column, QVariant(column + count));
            count--;
        }
        endInsertColumns();
        return true;
    }
    return false;
    //header should be modyfied manually if needed
}

bool CSVTable::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    //parrent is not supported yet
    if(row >= 0 && row+count <= rowCount()){
        beginRemoveRows(parent, row, row + count - 1);
        storage.remove(row, count);
        vHeader.remove(row, count);
        endRemoveRows();
        return true;
    }
    return false;
}

bool CSVTable::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    //parrent is not supported yet
    if(column >= 0 && column+count <= columnCount()){
        beginRemoveColumns(parent, column, column + count - 1);
        foreach (QVector<QVariant> row, storage) {
            row.remove(column, count);
        }
        hHeader.remove(column, count);
        endRemoveColumns();
        return true;
    }
    return false;
}
