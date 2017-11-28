#include "extfsm.h"
#include <qDebug>
#include <QFont>
#include <QBrush>

ExtFSM::ExtFSM(QObject *parent)
     : QAbstractTableModel(parent)
 {
 }

 ExtFSM::ExtFSM(QList<QList<QString>> list, QObject *parent)
     : QAbstractTableModel(parent)
 {
     dataList = list;
 }

 int ExtFSM::rowCount(const QModelIndex &parent) const
  {
      Q_UNUSED(parent);
      return dataList.size();
  }

  int ExtFSM::columnCount(const QModelIndex &parent) const
  {
      Q_UNUSED(parent);

      return cCount;
  }

  QVariant ExtFSM::data(const QModelIndex &index, int role) const
   {
       if (!index.isValid())
           return QVariant();

       if (index.row() >= dataList.size() || index.row() < 0)
           return QVariant();

       if (role == Qt::DisplayRole) {
           QList<QString> list = dataList.at(index.row());
           return list.at(index.column());
       }
       return QVariant();
   }

  QVariant ExtFSM::headerData(int section, Qt::Orientation orientation, int role) const
  {
      if (role != Qt::DisplayRole)
          return QVariant();

      if (orientation == Qt::Horizontal) {
          switch (section) {
              case 0:
                  return tr("filename");

              case 1:
                  return tr("sample");

              case 2:
                  return tr("elem.");

              case 3:
                  return tr("ang.");

              case 4:
                  return tr("rating");

              case 5:
                  return tr("date");

              case 6:
                  return tr("comment");

              default:
                  return QVariant();
          }
      }else{
          return section;
      }
      return QVariant();
  }

  bool ExtFSM::insertRows(int position, int rows, const QModelIndex &index)
   {
       Q_UNUSED(index);
       beginInsertRows(QModelIndex(), position, position+rows-1);

       for (int row=0; row < rows; row++) {
           QList<QString> blank;
           for(int i = this->columnCount(index); i > 0; i --){
               blank << "blank";
           }
           dataList.insert(position, blank);
       }
       endInsertRows();
       return true;
   }

  bool ExtFSM::removeRows(int position, int rows, const QModelIndex &index)
   {
       Q_UNUSED(index);
       beginRemoveRows(QModelIndex(), position, position+rows-1);

       for (int row=0; row < rows; ++row) {
           dataList.removeAt(position);
       }
       endRemoveRows();
       return true;
   }

  void ExtFSM::removeAll(){
      if(rowCount(QModelIndex()) != 0){
          removeRows(0, rowCount(QModelIndex()), QModelIndex());
      }
  }

  bool ExtFSM::setData(const QModelIndex &index, const QVariant &value, int role)
  {
          if (index.isValid() && role == Qt::EditRole) {
            QList<QString> p = dataList.value(index.row());
            p.replace(index.column(), value.toString());
            dataList.replace(index.row(), p);
            emit(dataChanged(index, index));
          return true;
          }
          return false;
  }

  QList<QList<QString>> ExtFSM::getList()
  {
      return dataList;
  }

  void ExtFSM::sort(int column, Qt::SortOrder order){
      if(order == Qt::AscendingOrder){
        switch(column){
            case 0:
                qSort(dataList.begin(), dataList.end(), less0);
                break;
            case 1:
                qSort(dataList.begin(), dataList.end(), less1);
                break;
            case 2:
                qSort(dataList.begin(), dataList.end(), less2);
                break;
            case 3:
                qSort(dataList.begin(), dataList.end(), less3);
                break;
            case 4:
                qSort(dataList.begin(), dataList.end(), less4);
                break;
            case 5:
                qSort(dataList.begin(), dataList.end(), less5);
                break;
        }
    }else{
        switch(column){
            case 0:
                qSort(dataList.begin(), dataList.end(), greater0);
                break;
            case 1:
                qSort(dataList.begin(), dataList.end(), greater1);
                break;
            case 2:
                qSort(dataList.begin(), dataList.end(), greater2);
                break;
            case 3:
                qSort(dataList.begin(), dataList.end(), greater3);
                break;
            case 4:
                qSort(dataList.begin(), dataList.end(), greater4);
                break;
            case 5:
                qSort(dataList.begin(), dataList.end(), greater5);
                break;
        }
    }
    emit(dataChanged(this->index(0, 0, QModelIndex()), this->index(rowCount(QModelIndex()) - 1, 0, QModelIndex())));
  }

bool ExtFSM::less0(QList<QString> a, QList<QString> b){
    return a.at(0) < b.at(0);
}

bool ExtFSM::less1(QList<QString> a, QList<QString> b){
    return a.at(1) < b.at(1);
}

bool ExtFSM::less2(QList<QString> a, QList<QString> b){
    return a.at(2) < b.at(2);
}

bool ExtFSM::less3(QList<QString> a, QList<QString> b){
    return a.at(3) < b.at(3);
}

bool ExtFSM::less4(QList<QString> a, QList<QString> b){
    return a.at(4) < b.at(4);
}

bool ExtFSM::less5(QList<QString> a, QList<QString> b){
    return a.at(5) < b.at(5);
}

bool ExtFSM::greater0(QList<QString> a, QList<QString> b){
    return a.at(0) > b.at(0);
}

bool ExtFSM::greater1(QList<QString> a, QList<QString> b){
    return a.at(1) > b.at(1);
}

bool ExtFSM::greater2(QList<QString> a, QList<QString> b){
    return a.at(2) > b.at(2);
}

bool ExtFSM::greater3(QList<QString> a, QList<QString> b){
    return a.at(3) > b.at(3);
}

bool ExtFSM::greater4(QList<QString> a, QList<QString> b){
    return a.at(4) > b.at(4);
}

bool ExtFSM::greater5(QList<QString> a, QList<QString> b){
    return a.at(5) > b.at(5);
}
