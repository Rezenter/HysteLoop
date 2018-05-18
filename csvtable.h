#ifndef CSVTABLE_H
#define CSVTABLE_H

#include <QAbstractTableModel>
#include <QVector>

class CSVTable : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CSVTable(QObject *parent = 0);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    /*
    QList<QList<QVariant>> storage;
    QList<QVariant> vHeader;
    QList<QVariant> hHeader;
    */
    QVector<QVector<QVariant>> storage; //row<column<value>>
    QVector<QVariant> vHeader;
    QVector<QVariant> hHeader;

};

#endif // CSVTABLE_H
