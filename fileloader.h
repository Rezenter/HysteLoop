#ifndef FILELOADER_H
#define FILELOADER_H

#include <QString>
#include <QVector>
#include <QPair>
#include "csvtable.h"
#include <QFile>
#include <QTextStream>


//move to thread

class FileLoader
{
public:
    FileLoader(QString path, QString name);
    ~FileLoader();
    bool isLoaded();
    QPair<int, QVector<QPair<double, QPair<double, double>>>> getSignal(); //vector<Pair<x, Pair<val, i-zero>>>
    QPair<int, QVector<QPair<double, QPair<double, double>>>> getBackground();

signals:
    void progress(int);

private:
    QFile *file;
    QTextStream *stream;
    QPair<int, QVector<QPair<double, QPair<double, double>>>> signal;
    QPair<int, QVector<QPair<double, QPair<double, double>>>> background;
    bool loaded = false;
    QPair<int, QVector<QPair<double, QPair<double, double>>>> load();
    double smoothedX(QVector<QPair<double, QPair<double, double>>> data, int ind, int count=5);
};

#endif // FILELOADER_H
