#ifndef ExtFSM_H
#define ExtFSM_H

#include <QAbstractTableModel>

//copypaste, fix

class ExtFSM : public QAbstractTableModel
 {

 public:
     ExtFSM(QObject *parent=0);
     ExtFSM(QList<QList<QString>> list, QObject *parent=0);

     int rowCount(const QModelIndex &parent) const;
     int columnCount(const QModelIndex &parent=QModelIndex()) const;
     QVariant data(const QModelIndex &index, int role) const;
     QVariant headerData(int section, Qt::Orientation orientation, int role) const;
     bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
     bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
     bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());
     QList<QList<QString>> getList();
     void removeAll();
     void sort(int column, Qt::SortOrder = Qt::AscendingOrder);
     static bool less1(QList<QString>, QList<QString>);
     static bool less2(QList<QString>, QList<QString>);
     static bool less3(QList<QString>, QList<QString>);
     static bool less4(QList<QString>, QList<QString>);
     static bool less5(QList<QString>, QList<QString>);
     static bool less0(QList<QString>, QList<QString>);
     static bool greater1(QList<QString>, QList<QString>);
     static bool greater2(QList<QString>, QList<QString>);
     static bool greater3(QList<QString>, QList<QString>);
     static bool greater4(QList<QString>, QList<QString>);
     static bool greater5(QList<QString>, QList<QString>);
     static bool greater0(QList<QString>, QList<QString>);


 private:
     int cCount = 7; //comment is hidden
     QList<QList<QString>> dataList;
 };

#endif // ExtFSM_H
