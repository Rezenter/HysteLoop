#ifndef FILELOADER_H
#define FILELOADER_H

#include <QString>
#include <QVector>
#include <QPair>
#include "csvtable.h"


//http:doc.qt.io/qt-5/qabstracttablemodel.html
//subclass QAbstractTableModel

class FileLoader
{
public:
    FileLoader(QString);
    ~FileLoader();

private:
};

#endif // FILELOADER_H
