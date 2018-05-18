#include "fileloader.h"
#include <QDebug>
#include <QSettings>

FileLoader::FileLoader(QString path, QString name){
    file = new QFile(path + "/" + name);
    file->open(QIODevice::ReadOnly);
    stream = new QTextStream(file);
    if(name.endsWith(".csv", Qt::CaseInsensitive)){
        signal = load();
        signal.first = -1;
    }else if(name.endsWith(".par", Qt::CaseInsensitive)){
        file->close();
        delete stream;
        QSettings par(path + "/" + name, QSettings::IniFormat);
        par.beginGroup("file1");
            QString file1 = par.value("filename", "undefined").toString();
            file = new QFile(path + "/" + file1);
            file->open(QIODevice::ReadOnly);
            stream = new QTextStream(file);
            signal = load();
            file->close();
            delete stream;
            signal.first = par.value("sensetivity", -1).toInt();
        par.endGroup();
        par.beginGroup("file2");
            QString file2 = par.value("filename", "undefined").toString();
            file = new QFile(path + "/" + file2);
            file->open(QIODevice::ReadOnly);
            stream = new QTextStream(file);
            background = load();
            background.first = par.value("sensetivity", -1).toInt();
            loaded = true;
        par.endGroup();
    }else{
        qDebug() << "wrong file";
    }
    file->close();
}

FileLoader::~FileLoader(){
    delete stream;
    delete file;
}

bool FileLoader::isLoaded(){
    return loaded;
}

QPair<int, QVector<QPair<double, QPair<double, double>>>> FileLoader::getSignal(){
    return signal;
}

QPair<int, QVector<QPair<double, QPair<double, double>>>> FileLoader::getBackground(){
    return background;
}

QPair<int, QVector<QPair<double, QPair<double, double>>>> FileLoader::load(){
    CSVTable data;
    while(!stream->atEnd()) {
        QString line = stream->readLine();
        if(line.length() > 0){
            data.insertRow(data.rowCount());
            QStringList args = line.split(",");
            if(data.columnCount() < args.length()){
                data.insertColumns(data.columnCount(), args.length()-data.columnCount());
            }
            for(int column = 0; column < args.length(); column++){
                data.setData(data.index(data.rowCount() - 1, column, QModelIndex()), args.at(column));
            }
        }
    }
    QPair<int, QVector<QPair<double, QPair<double, double>>>> curr;
    double x;
    double y;
    double z;
    double xScale = data.data(data.index(8, 1, QModelIndex()), Qt::DisplayRole).toDouble();
    double xMult = 290/0.0393; //in order to fit -3 +3 mT
    double yScale = data.data(data.index(8, 7, QModelIndex()), Qt::DisplayRole).toDouble();
    double zScale = data.data(data.index(8, 19, QModelIndex()), Qt::DisplayRole).toDouble();
    //double yZero = data.data(data.index(13, 7, QModelIndex()), Qt::DisplayRole).toDouble();
    double yZero = 0;
    for(int i = 0; i < data.rowCount() ; i++){
        x = data.data(data.index(i, 4, QModelIndex()), Qt::DisplayRole).toDouble();
        y = data.data(data.index(i, 10, QModelIndex()), Qt::DisplayRole).toDouble();
        z = data.data(data.index(i, 22, QModelIndex()), Qt::DisplayRole).toDouble();
        curr.second.append(QPair<double, QPair<double, double>>(x*xScale*xMult, QPair<double, double>((y + yZero)*yScale, z*zScale)));
    }
    int zeroIndex;
    double pv = smoothedX(curr.second, curr.second.length() - 1);
    double cv = smoothedX(curr.second, 0);
    double nv = smoothedX(curr.second, 1);
    if(cv >= 0 && pv < 0 && cv <= nv){
        zeroIndex = 0;
    }else{
        zeroIndex = 1;
        pv = cv;
        cv = nv;
        nv = smoothedX(curr.second, zeroIndex + 1);
        while(!((cv >= 0) && (pv < 0) && (cv <= nv))){
            zeroIndex++;
            pv = cv;
            cv = nv;
            nv = smoothedX(curr.second, zeroIndex);
        }
    }
    QVector<QPair<double, QPair<double, double>>> tmp = curr.second.mid(0, zeroIndex);
    curr.second.remove(0, zeroIndex);
    curr.second.append(tmp);
    return curr;
}

double FileLoader::smoothedX(QVector<QPair<double, QPair<double, double>>> data, int ind, int count){
    //count is odd!
    double res = 0;
    if(count > 0){
        if(data.length() < count){
            for(int i = 0; i < data.length(); i++){
                res += data.at(i).first;
            }
            res = res/data.length();
        }else{
            int c = count;
            for(int i = ind - (count-1)/2; c > 0; c --){
                if(i < 0){
                    i = data.length() + i;
                }else if(i >= data.length()){
                    i = i - data.length();
                }
                res += data.at(i).first;
                i++;
            }
            res = res/count;
        }
    }
    return res;
}
