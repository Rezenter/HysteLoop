#include <QFile>
#include <QSettings>

#include "calculator.h"

#include "QDebug"

//store constants in settings
int Calculator::type = 0;
int Calculator::files = 0;

Calculator::Calculator(QObject *parent) : QObject(parent){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    for(int i = 0; i < 2 ; ++i){
        output[i] = new QVector<QPointF>;
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

Calculator::~Calculator(){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    for(int i = 0; i < 2 ; ++i){
        delete output[i];
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
    emit dead();
}

void Calculator::setLoader(const QString loaderPath, const QString filename){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << loaderPath << filename;
    if((path != loaderPath) || (filename != this->filename)){
        path = loaderPath;
        this->filename = filename;
        load();
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::load(){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    if(filename.size() != 0){
        QFile dataFile;
        QTextStream* stream;
        QStringList names;
        if(filename.endsWith(".csv", Qt::CaseInsensitive)){
            dataFile.setFileName(path + "/" + filename);
            dataFile.open(QIODevice::ReadOnly);
            stream = new QTextStream(&dataFile);
            fillTable(0, stream);
            dataFile.close();
            delete stream;
            names.append(filename.chopped(4));
        }else if(filename.endsWith(".par", Qt::CaseInsensitive)){
            QSettings par(path + "/" + filename, QSettings::IniFormat);
            for(int i = 0; i < 2; ++i){
                par.beginGroup("file" + QString::number(i + 1));
                    QString filename = par.value("filename", "").toString();
                    dataFile.setFileName(path + "/" + filename);
                    dataFile.open(QIODevice::ReadOnly);
                    stream = new QTextStream(&dataFile);
                    fillTable(i, stream);
                    dataFile.close();
                    delete stream;
                    sensetivity[i] = par.value("sensetivity", 1).toInt();
                    names.append(filename.chopped(4));
                par.endGroup();
            }
        }
        dataFile.close();
        emit dataPointers(output[0], output[1], names);
        //wait for update() signal
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::fillTable(const int file, QTextStream *stream){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << file;
    double xMult = 290.0/0.0393; //emperical value needed in order to fit [-3, +3] mT
    double xScale = 1.0;
    double yScale = 1.0;
    double zScale = 1.0;
    QStringList buffer;
    for(int i = 0; i < 9; ++i){
        buffer.append(stream->readLine());
    }
    QStringList args = buffer.at(8).split(",");
    xScale = args.at(1).toDouble()*xMult;
    yScale = args.at(7).toDouble();
    zScale = args.at(19).toDouble();
    while(!stream->atEnd()) {
        QString line;
        if(!buffer.isEmpty()){
            line = buffer.takeFirst();
        }else{
            line = stream->readLine();
        }
        if(line.length() > 0){
            QStringList args = line.split(",");
            int row = storage[file].size();
            storage[file].append(new qreal[3]);
            for(int column = 0; column < args.size(); column++){
                switch (column) {
                case 4:
                    storage[file].at(row)[0] = args.at(column).toDouble()*xScale; //x
                    break;
                case 10:
                    storage[file].at(row)[1] = args.at(column).toDouble()*yScale; // y
                    break;
                case 22:
                    storage[file].at(row)[2] = args.at(column).toDouble()*zScale; // i-zero
                    break;
                default:
                    break;
                }
            }
        }
    }
    tableFilled[file] = true;
    shuffle(file);
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::setLoadingSmooth(const int count){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << count;
    if(count > 0 && loadingSmooth != count){
        loadingSmooth = count;
        load();
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}


void Calculator::shuffle(const int file){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << file;
    if(tableFilled[file]){
        int count = 0;
        int start = qFloor(loadingSmooth);
        int maxIndex = start;
        qreal average = 0.0;
        for(int i = 0; i < loadingSmooth; ++i){
            average += storage[file].at(i)[0];
        }
        qreal maxValue = average;
        while(count < storage[file].size()){
            average -= storage[file].at(count)[0];
            int index = count + loadingSmooth;
            if(index >= storage[file].size()){
                index -= storage[file].size();
            }
            average += storage[file].at(index)[0];
            if(average > maxValue){
                maxValue = average;
                maxIndex = count + start;
            }
            count++;
        }
        QVector<qreal*> storageCopy;
        storageCopy.append(storage[file].mid(maxIndex + 1));
        storageCopy.append(storage[file].mid(0, maxIndex));
        storage[file].swap(storageCopy);
        shuffled[file] = true;
        fillGrid(file);
    }else{
        qDebug() << "not filled yet";
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}


void Calculator::setGrain(const qreal val){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << val;
    if(val != grain){
        grain = val;
        if(tableFilled[0]){
            fillGrid(0);
        }
        if(tableFilled[1]){
            fillGrid(1);
        }
        update();
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::setVerticalOffset(const int file, const qreal val){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << file << val;
    if(verticalOffset[file] != val){
        verticalOffset[file] = val;
        fillGrid(file);
        update();
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::fillGrid(const int file){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << file;
    if(shuffled[file]){
        xRange = QPointF(std::numeric_limits<double>::max(), std::numeric_limits<double>::min());
        yRange[file] = yRange[file];
        qreal offset = verticalOffset[file]/2.0;
        gridFall[file].clear();
        gridFall[file].resize(qFloor((600.0 + grain - 0.1)/grain));
        gridRise[file].clear();
        gridRise[file].resize(qFloor((600.0 + grain - 0.1)/grain));
        int count = 0;
        foreach (qreal* row, storage[file]) {
            int gridIndex = qFloor((row[0] + 300.0 + grain/2.0)/grain);
            qreal x = gridIndex*grain - 300.0;
            qreal y;
            qreal z;
            if(count < qFloor(storage[file].size()/2)){
                if(gridRise[file].at(gridIndex).second.x() == 0.0 && gridRise[file].at(gridIndex).second.y() == 0.0){
                    y = row[1] - offset;
                    z = row[2];
                }else{
                    y = (row[1] - offset + gridRise[file].at(gridIndex).second.x())/2.0;
                    z = (row[2] + gridRise[file].at(gridIndex).second.y())/2.0;
                }
                gridRise[file].replace(gridIndex, QPair<qreal, QPointF>(x, QPointF(y, z)));
            }else{
                if(gridFall[file].at(gridIndex).second.x() == 0.0 && gridFall[file].at(gridIndex).second.y() == 0.0){
                    y = row[1] + offset;
                    z = row[2];
                }else{
                    y = (row[1] + offset + gridFall[file].at(gridIndex).second.x())/2.0;
                    z = (row[2] + gridFall[file].at(gridIndex).second.y())/2.0;
                }
                gridFall[file].replace(gridIndex, QPair<qreal, QPointF>(x, QPointF(y, z)));
            }
            count ++;
        }
        loaded[file] = true;
    }else{
        qDebug() << "not shuffled yet";
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::update(const int signal, const int newFiles){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << signal << newFiles;
    type = signal;
    files = newFiles;
    bool ok = true;
    firstRange = QPointF();
    exported[0] = false;
    exported[1] = false;
    mutex.lock();
    switch (type) {
    case 0: //raw loops
        if(files == 0 || files == 2){
            rawOutput(0);
        }
        if(files == 1 || files == 2){
            rawOutput(1);
        }
        break;
    case 1://zero

        break;
    case 2://normalized
        break;
    default:
        ok = false;
        qDebug() << "wtf:: type == " << signal;
        break;
    }
    for(int file = 0; file < 2; ++file){
        if(exported[file]){
            qreal offset = (yRange[file].x() + yRange[file].y())/2.0;
            QPointF offsetP(offset, offset);
            for(int i = 0; i < output[file]->size(); ++i){
                output[file]->replace(i, output[file]->at(i) - offsetP);
            }
            yRange[file] -= offsetP;
        }
    }
    mutex.unlock();
    if(ok){
        if(exported[0] && exported[1]){
            emit range(xRange, QPointF(qMin(yRange[0].x(), yRange[1].x()), qMax(yRange[0].y(), yRange[1].y())));
        }else if(exported[0]){
            emit range(xRange, yRange[0]);
        }else{
            emit range(xRange, yRange[1]);
        }
        for(int i = 0; i < 2; ++i){
            if(exported[i]){
                emit ready(i);
            }
        }
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::checkRange(QPointF point, const int file){
    if(point.x() < xRange.x()){
        xRange.setX(point.x());
    }else if(point.x() > xRange.y()){
        xRange.setY(point.x());
    }
    if(point.y() < yRange[file].x()){
        yRange[file].setX(point.y());
    }else if(point.y() > yRange[file].y()){
        yRange[file].setY(point.y());
    }
}

void Calculator::rawOutput(const int file){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << file;
    if(loaded[file]){
        xRange = QPointF(std::numeric_limits<double>::max(), std::numeric_limits<double>::min());
        yRange[file] = xRange;
        exported[file] = true;
        output[file]->clear();
        for(int i = gridFall[file].size() - 1; i >= 0 ; --i){
            if(!(gridFall[file].at(i).first == 0 && gridFall[file].at(i).second.x() == 0 && gridFall[file].at(i).second.y() == 0)){
                output[file]->append(QPointF(gridFall[file].at(i).first, gridFall[file].at(i).second.x()));
                checkRange(output[file]->last(), file);
            }
        }
        for(int i = 0; i < gridRise[file].size(); ++i){
            if(!(gridRise[file].at(i).first == 0 && gridRise[file].at(i).second.x() == 0 && gridRise[file].at(i).second.y() == 0)){
                output[file]->append(QPointF(gridRise[file].at(i).first, gridRise[file].at(i).second.x()));
                checkRange(output[file]->last(), file);
            }
        }
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}
