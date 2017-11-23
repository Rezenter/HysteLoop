#include "fileloader.h"
#include <QDebug>

//copypaste, fix

FileLoader::FileLoader(QString path){
    file = new QFile(path);
    file->open(QIODevice::ReadOnly);
    stream = new QTextStream(file);
    bool flag = false;
    if(path.endsWith("txt", Qt::CaseInsensitive)){
        while(!stream->atEnd()) {
            QString line = stream->readLine();
            if(line.length() > 0 && flag){
                QStringList args = line.split("\t");
                if(args.length() < 10){
                    zero.append(QPair<qreal, QPair<qreal, qreal>>(args.at(2).toDouble(), QPair<qreal, qreal>(args.at(4).toDouble(), args.at(4).toDouble())));
                    bareData.append(QPair<qreal, QPair<qreal, qreal>>(args.at(2).toDouble(), QPair<qreal, qreal>(args.at(3).toDouble(), args.at(3).toDouble())));
                    data.append(QPair<qreal, QPair<qreal, qreal>>(args.at(2).toDouble(), QPair<qreal, qreal>(args.at(6).toDouble(), args.at(6).toDouble())));
                }else{

                    zero.append(QPair<qreal, QPair<qreal, qreal>>(args.at(2).toDouble(), QPair<qreal, qreal>(args.at(4).toDouble(), args.at(8).toDouble())));
                    bareData.append(QPair<qreal, QPair<qreal, qreal>>(args.at(2).toDouble(), QPair<qreal, qreal>(args.at(3).toDouble(), args.at(7).toDouble())));
                    data.append(QPair<qreal, QPair<qreal, qreal>>(args.at(2).toDouble(), QPair<qreal, qreal>(args.at(6).toDouble(), args.at(10).toDouble())));
                }
            }else{
                if(line.length() == 0){
                    flag = true;
                }
            }
        }
        ri = data.length()-1;
    }
    file->close();
}

FileLoader::~FileLoader(){
    delete stream;
    file->close();
    delete file;
}

QVector<QPair<qreal, QPair<qreal, qreal>>> FileLoader::getData(){
    QVector<QPair<qreal, QPair<qreal, qreal>>> res;
    for(int i = li; i <= ri; i++){
        res.append(data.at(i));
    }
    return res;
}

QVector<QPair<qreal, QPair<qreal, qreal>>> FileLoader::getBareData(){
    QVector<QPair<qreal, QPair<qreal, qreal>>> res;
    for(int i = li; i <= ri; i++){
        res.append(bareData.at(i));
    }
    return res;
}

QPair<int, int> FileLoader::getLimits(){
    return QPair<int, int>(li, ri);
}

void FileLoader::setLimits(double l, double r){
    this->r = r;
    this->l = l;
    QPair<qreal, QPair<qreal, qreal>> pair;
    bool flagl = true;
    bool flagr = true;
    int i = 0;
    foreach(pair, data){
        if(flagl && pair.first - data.at(0).first >= l){
            flagl = false;
            li = i;
        }else{
            if(flagr && data.at(data.length() - 1).first - pair.first <= r){
                flagr = false;
                ri = i;
                break;
            }
        }
        i++;
    }
}

QVector<QPair<qreal, QPair<qreal, qreal>>> FileLoader::getZero(){
    return zero;
}
