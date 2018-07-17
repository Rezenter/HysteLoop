#include <QFile>
#include <QSettings>

#include "calculator.h"

#include "QDebug"

//store constants in settings
int Calculator::type = 0;
int Calculator::files = 0;
bool Calculator::zeroNorm = false;

Calculator::Calculator(const QString name, QObject *parent) : QObject(parent){
    qDebug() << this->metaObject()->className()  <<  name <<  "::" << __FUNCTION__;
    this->name = name;
    mutex = new QMutex(QMutex::Recursive);
    for(int file = 0; file < 2 ; ++file){
        output[file] = new QVector<QPointF>;
        splits[file][0] = new QList<Calculator::Split*>;
        splits[file][1] = new QList<Calculator::Split*>;
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

Calculator::~Calculator(){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__;
    mutex->lock();
    for(int file = 0; file < 2; ++file){
        for(int j = 0; j < storage[file].size(); ++j){
            delete[] storage[file].at(j);
        }
        storage[file].clear();
        output[file]->clear();
        delete output[file];
        for(int rise = 0; rise < 2; ++rise){
            grid[file][rise].clear();
            fixedGrid[file][rise].clear();
            for(int i = 0; i < splits[file][rise]->size(); ++i){
                splits[file][rise]->at(i)->original->clear();
                delete splits[file][rise]->at(i)->original;
                delete splits[file][rise]->at(i);
            }
            splits[file][rise]->clear();
            delete splits[file][rise];
        }
    }
    mutex->unlock();
    delete mutex;
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
    emit dead();
    qDebug() << this->metaObject()->className()  <<  this->name <<  ":: DEAD";
}

void Calculator::setLoader(const QString loaderPath, const QString filename){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << loaderPath << filename;
    if((path != loaderPath) || (filename != this->filename)){
        path = loaderPath;
        this->filename = filename;
        load();
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::setLoadingSmooth(const int count){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << count;
    if(count > 0 && loadingSmooth != count){
        loadingSmooth = count;
        load();
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::load(){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__;
    if(filename.size() != 0){
        mutex->lock();
        xRange = QPointF(std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest());
        firstRange = xRange;
        for(int file = 0; file < 2; ++file){
            for(int j = 0; j < storage[file].size(); ++j){
                delete[] storage[file].at(j);
            }
            storage[file].clear();
            output[file]->clear();
            grid[file][0].clear();
            grid[file][1].clear();
            loaded[file] = false;
            tableFilled[file] = false;
            shuffled[file] = false;
            exported[file] = false;
            yRange[file] = xRange;
            for(int rise = 0; rise < 2; ++rise){
                for(int i = 0; i < splits[file][rise]->size(); ++i){
                    splits[file][rise]->at(i)->original->clear();
                    delete splits[file][rise]->at(i)->original;
                    delete splits[file][rise]->at(i);
                }
                splits[file][rise]->clear();
            }
        }
        mutex->unlock();
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
                    names.append(filename.chopped(3) + "par");
                par.endGroup();
            }
        }
        dataFile.close();
        emit dataPointers(output[0], output[1], names);
        //synchronize with MainWindow and wait for update() signal
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::fillTable(const int file, QTextStream *stream){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file;
    double xMult = 290.0/0.0393; //emperical value needed in order to fit [-3, +3] mT
    QStringList buffer;
    for(int i = 0; i < 9; ++i){ //bufferisation needed in order to read scale from 8th line
        buffer.append(stream->readLine());
    }
    QStringList args = buffer.at(8).split(",");
    double xScale = args.at(1).toDouble()*xMult;
    double yScale = args.at(7).toDouble();
    double zScale = args.at(19).toDouble();
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
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::shuffle(const int file){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file;
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
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}


void Calculator::setGrain(const qreal val){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << val;
    if(val != grain){
        grain = val;
        for(int file = 0; file < 2; ++file){
            if(tableFilled[file]){
                fillGrid(file);
            }
        }
        update();
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::setVerticalOffset(const int file, const qreal val){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file << val;
    if(verticalOffset[file] != val){
        verticalOffset[file] = val;
        fillGrid(file);
        update();
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::fillGrid(const int file){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file;
    if(shuffled[file]){
        xRange = QPointF(std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest());
        yRange[file] = yRange[file];
        qreal offset = verticalOffset[file]/2.0;
        for(int rise = 0; rise < 2; ++rise){
            grid[file][rise].clear();
            grid[file][rise].resize(qCeil((600.0 + grain - 0.1)/grain));
        }
        int count = 0;
        foreach (qreal* row, storage[file]) {
            int gridIndex = qFloor((row[0] + 300.0 + grain/2.0)/grain);
            qreal x = gridIndex*grain - 300.0;
            qreal y;
            qreal z;
            int rise = 1;
            if(count < qFloor(storage[file].size()/2)){
                rise = 0;
            }
            int sign = qPow(-1.0, rise + 1.0);
            if(grid[file][rise].at(gridIndex).second.x() == 0.0 && grid[file][rise].at(gridIndex).second.y() == 0.0){
                y = row[1] - sign*offset;
                z = row[2];
            }else{
                y = (row[1] - sign*offset + grid[file][rise].at(gridIndex).second.x())/2.0;
                z = (row[2] + grid[file][rise].at(gridIndex).second.y())/2.0;
            }
            grid[file][rise].replace(gridIndex, QPair<qreal, QPointF>(x, QPointF(y, z)));
            count ++;
        }
        for(int rise = 0; rise < 2; ++rise){
            for(int i = 0; i < grid[file][rise].size(); ++i){
                if(grid[file][rise].at(i).first == 0.0 && grid[file][rise].at(i).second.x() == 0.0 && grid[file][rise].at(i).second.y() == 0.0){
                    grid[file][rise].remove(i);
                    i--;
                }
            }
        }
        fixedGrid = grid;
        loaded[file] = true;
        findSplits(file);
    }else{
        qDebug() << "not shuffled yet";
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::update(const int signal, const int newFiles, const bool newZeroNorm){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << signal << newFiles << newZeroNorm;
    type = signal;
    files = newFiles;
    zeroNorm = newZeroNorm;
    firstRange = QPointF();
    exported[0] = false;
    exported[1] = false;
    mutex->lock();
    if(type == 0 || type == 1){
        if(files == 0 || files == 2){
            fillOutput(0);
        }
        if(files == 1 || files == 2){
            fillOutput(1);
        }
    }else if(type == 2){
        fillOutput(0);
    }else{
        qDebug() << "wtf:: type == " << signal;
    }
    for(int file = 0; file < 2; ++file){
        if(exported[file]){
            offsets[file] = (yRange[file].x() + yRange[file].y())/2.0;
            QPointF offsetP(offsets[file], offsets[file]);
            for(int i = 0; i < output[file]->size(); ++i){
                output[file]->replace(i, output[file]->at(i) - offsetP);
            }
            yRange[file] -= offsetP;
        }
    }
    mutex->unlock();
    if(exported[0] && exported[1]){
        emit range(xRange, QPointF(qMin(yRange[0].x(), yRange[1].x()), qMax(yRange[0].y(), yRange[1].y())));
    }else if(exported[0]){
        emit range(xRange, yRange[0]);
    }else if(exported[1]){
        emit range(xRange, yRange[1]);
    }
    for(int i = 0; i < 2; ++i){
        if(exported[i]){
            emit ready(i);
        }
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::checkRange(QPointF point, const int file){
    if(point.x() < xRange.x()){
        xRange.setX(point.x());
    }
    if(point.x() > xRange.y()){
        xRange.setY(point.x());
    }
    if(point.y() < yRange[file].x()){
        yRange[file].setX(point.y());
    }
    if(point.y() > yRange[file].y()){
        yRange[file].setY(point.y());
    }
}

void Calculator::fillOutput(const int file){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file;
    if(loaded[file]){
        xRange = QPointF(std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest());
        yRange[file] = xRange;
        exported[file] = true;
        output[file]->clear();
        switch (type) {
        case 0:
            {
                for(int i = fixedGrid[file][0].size() - 1; i >= 0 ; --i){
                    output[file]->append(QPointF(fixedGrid[file][0].at(i).first, fixedGrid[file][0].at(i).second.x()));
                    checkRange(output[file]->last(), file);
                }
                //filter here

                for(int i = 0; i < fixedGrid[file][1].size(); ++i){
                    output[file]->append(QPointF(fixedGrid[file][1].at(i).first, fixedGrid[file][1].at(i).second.x()));
                    checkRange(output[file]->last(), file);
                }
            }
            break;
        case 1:
            for(int i = fixedGrid[file][0].size() - 1; i >= 0 ; --i){
                output[file]->append(QPointF(fixedGrid[file][0].at(i).first, fixedGrid[file][0].at(i).second.y()));
                checkRange(output[file]->last(), file);
            }
            for(int i = 0; i < fixedGrid[file][1].size(); ++i){
                output[file]->append(QPointF(fixedGrid[file][1].at(i).first, fixedGrid[file][1].at(i).second.y()));
                checkRange(output[file]->last(), file);
            }
            break;
        case 2:
            if(loaded[1]){
                qreal sensRelation = sensetivity[0]/sensetivity[1];
                for(int rise = 0; rise < 2; ++rise){
                    int sign = qPow(-1.0, rise + 1.0);
                    for(int i = rise*(fixedGrid[0][rise].size() - 1); i >= 0 && i < fixedGrid[0][rise].size(); i -= sign){
                        qreal x = fixedGrid[0][rise].at(i).first;
                        int zeroIndex = i;
                        if(fixedGrid[1][rise].size() <= zeroIndex){
                            zeroIndex = fixedGrid[1][rise].size() - 1;
                        }
                        while(true){
                            if(fixedGrid[1][rise].at(zeroIndex).first > x){
                                if(zeroIndex > 0){
                                    zeroIndex--;
                                    if(zeroIndex > 0 && fixedGrid[1][rise].at(zeroIndex).first > x){
                                        continue;
                                    }
                                }
                            }else if(fixedGrid[1][rise].at(zeroIndex).first < x){
                                if(zeroIndex < fixedGrid[1][rise].size()){
                                    zeroIndex++;
                                    if(zeroIndex < fixedGrid[1][rise].size() && fixedGrid[1][rise].at(zeroIndex).first < x){
                                        continue;
                                    }
                                }
                            }
                            break;
                        }
                        if(zeroIndex < fixedGrid[1][rise].size() && fixedGrid[1][rise].at(0).first < x){ //if the point is not hanging beyond edges
                            qreal y = fixedGrid[1][rise].at(zeroIndex).second.x();
                            qreal z = fixedGrid[1][rise].at(zeroIndex).second.y();
                            if(fixedGrid[1][rise].at(zeroIndex).first < x){
                                if(zeroIndex < fixedGrid[1][rise].size() - 1){
                                    y = fixedGrid[1][rise].at(zeroIndex).second.x() + ((x - fixedGrid[1][rise].at(zeroIndex).first)/
                                            (fixedGrid[1][rise].at(zeroIndex).first + fixedGrid[1][rise].at(zeroIndex + 1).first))*
                                            (fixedGrid[1][rise].at(zeroIndex + 1).second.x() - fixedGrid[1][rise].at(zeroIndex).second.x());
                                    z = fixedGrid[1][rise].at(zeroIndex).second.y() + ((x - fixedGrid[1][rise].at(zeroIndex).first)/
                                            (fixedGrid[1][rise].at(zeroIndex).first + fixedGrid[1][rise].at(zeroIndex + 1).first))*
                                            (fixedGrid[1][rise].at(zeroIndex + 1).second.y() - fixedGrid[1][rise].at(zeroIndex).second.y());
                                }
                            }else if(fixedGrid[1][rise].at(zeroIndex).first != x){
                                if(zeroIndex > 0){
                                    y = fixedGrid[1][rise].at(zeroIndex - 1).second.x() + ((x - fixedGrid[1][rise].at(zeroIndex - 1).first)/
                                            (fixedGrid[1][rise].at(zeroIndex - 1).first + fixedGrid[1][rise].at(zeroIndex).first))*
                                            (fixedGrid[1][rise].at(zeroIndex).second.x() - fixedGrid[1][rise].at(zeroIndex - 1).second.x());
                                    z = fixedGrid[1][rise].at(zeroIndex - 1).second.y() + ((x - fixedGrid[1][rise].at(zeroIndex - 1).first)/
                                            (fixedGrid[1][rise].at(zeroIndex - 1).first + fixedGrid[1][rise].at(zeroIndex).first))*
                                            (fixedGrid[1][rise].at(zeroIndex).second.y() - fixedGrid[1][rise].at(zeroIndex - 1).second.y());
                                }
                            }
                            if(zeroNorm){
                                y = (fixedGrid[0][rise].at(i).second.x()/z)*sensRelation/y;
                            }else{
                                y = fixedGrid[0][rise].at(i).second.x()*sensRelation/y;
                            }
                            output[0]->append(QPointF(fixedGrid[0][rise].at(i).first, y));
                            checkRange(output[0]->last(), 0);
                        }
                    }
                }
            }else{
                qDebug() << "gui error: not loaded";
            }
            break;
        default:
            break;
        }
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::setCriticalDer(const int file, const qreal val){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << val;
    if(criticalDer[file] != val){
        criticalDer[file] = val;
        findSplits(0);
        findSplits(1);
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::findSplits(const int file){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file << criticalDer[file];
    if(loaded[file]){
        mutex->lock();
        for(int rise = 0; rise < 2; ++rise){
            for(int i = 0; i < splits[file][rise]->size(); ++i){
                splits[file][rise]->at(i)->original->clear();
                delete splits[file][rise]->at(i)->original;
                delete splits[file][rise]->at(i);
            }
            splits[file][rise]->clear();
            if(grid[file][rise].size() > 1){
                QPair<qreal, QPointF> prev = grid[file][rise].at(0);
                for(int i = 1; i < grid[file][rise].size(); ++i){
                    QPair<qreal, QPointF> point = grid[file][rise].at(i);
                    if(qAbs((point.second.x() - prev.second.x())/(point.first - prev.first)) > criticalDer[file]){
                        splits[file][rise]->append(new Calculator::Split);
                        splits[file][rise]->last()->x = point.first;
                        splits[file][rise]->last()->index = i;
                        splits[file][rise]->last()->original = new QVector<QPointF>;
                        updateSplit(file, rise, splits[file][rise]->size() - 1);
                    }
                    prev = point;
                }
            }
        }
        mutex->unlock();
        emit updateSplits(file);
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::updateSplit(const int file, const int rise, const int index){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file << rise << index;
    Split* curr = splits[file][rise]->at(index);
    mutex->lock();
    int l = qMax(curr->index - curr->oldL, 0);
    int r = qMin(curr->index + curr->oldR, grid[file][rise].size() - 1);
    for(int i = l; i <= r; ++i){
        fixedGrid[file][rise].replace(i, grid[file][rise].at(i));
    }
    curr->oldL = curr->l;
    curr->oldR = curr->r;
    curr->original->clear();
    std::array<qreal, 3> coeff;
    l = qMax(curr->index - curr->l, 0);
    r = qMin(curr->index + curr->r, grid[file][rise].size() - 1);
    QPointF lp = QPointF(grid[file][rise].at(l).first, grid[file][rise].at(l).second.x());
    QPointF rp = QPointF(grid[file][rise].at(r).first, grid[file][rise].at(r).second.x());
    if(curr->index - l != 0 && r - curr->index != 0){
        qreal mid = qAbs(rp.y() - lp.y())*curr->shift + qMin(rp.y(), lp.y()); //mid.y //mid.x = curr.x
        coeff[0] = (lp.y() - mid + ((curr->x - lp.x())*(mid - rp.y()))/(curr->x - rp.x()))/
                (qPow(lp.x(), 2.0) - qPow(curr->x, 2.0) + ((qPow(rp.x(), 2.0) - qPow(curr->x, 2.0))*(lp.x() - curr->x))/(curr->x - rp.x()));
        coeff[1] = (mid - rp.y() - coeff[0]*(qPow(curr->x, 2.0) - qPow(rp.x(), 2.0)))/(curr->x - rp.x());
        coeff[2] = rp.y() - coeff[0]*qPow(rp.x(), 2.0) - coeff[1]*rp.x();
        for(int i = l; i <= r; ++i){
            curr->original->append(QPointF(grid[file][rise].at(i).first, grid[file][rise].at(i).second.x()));
            qreal x = grid[file][rise].at(i).first;
            fixedGrid[file][rise].replace(i, QPair<qreal, QPointF>(x, QPointF(coeff[0]*qPow(x, 2.0) + coeff[1]*x + coeff[2],
                    grid[file][rise].at(i).second.y())));
        }
    }
    mutex->unlock();
    emit updateSplits(file, index);
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::setSplines(const int file, const int pow, const int length){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file << pow << length;
    bool flag = false;
    if(frame[file] != length){
        frame[file] = length;
        flag = true;
    }
    if(power[file] != pow){
        power[file] = pow;
        flag = true;
    }
    if(flag){
        update();
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::exportSeries(const QString path){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << path;
    switch (files) {
    case 0:
        exportSeries(0, path + "/edge.DAT");
        break;
    case 1:
        exportSeries(1, path + "/preEdge.DAT");
        break;
    case 2:
        exportSeries(0, path + "/edge.DAT");
        exportSeries(1, path + "/preEdge.DAT");
        break;
    default:
        qDebug() << "wtf";
        break;
    }
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}

void Calculator::exportSeries(const int file, const QString path){
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << file << path;
    QFile outFile(path);
    outFile.open(QIODevice::WriteOnly);
    QTextStream stream(&outFile);
    stream << QString("%1").arg("Time", -5) << "\t" << QString("%1").arg("Temp.", -5) << "\t" << QString("%1").arg("Magn.Field (Oe)", -20)
           << "\t" << QString("%1").arg("Moment (arb.units)", -20) << "\r\n";
    for(int i = 0; i < output[file]->size(); ++i){
        stream << QString("%1").arg(i, -5) << "\t" << QString("%1").arg(300.0, -5) << "\t" << QString("%1").arg(output[file]->at(i).x() * 10.0, -20)
               << QString("%1").arg(output[file]->at(i).y(), -20) << "\r\n";
        emit exportProgress(qFloor(100*i/(output[file]->size() - 1)));
    }
    outFile.close();
    if(name.endsWith(".par", Qt::CaseInsensitive)){
        QSettings oldPar(this->path + "/" + filename, QSettings::IniFormat);
        QSettings newPar(path + ".par", QSettings::IniFormat);
        QString sampleName;
        newPar.beginGroup("settings");
            oldPar.beginGroup("common");
                sampleName += oldPar.value("sample", "").toString();
                newPar.setValue("comment", oldPar.value("comment", "").toString());
                newPar.setValue("rating", oldPar.value("rating", "").toString());
                newPar.setValue("chi", oldPar.value("angle", "").toString()); //CHECK!!! deg or rad
            oldPar.endGroup();
            oldPar.beginGroup("file1");
                sampleName += "_" + oldPar.value("element", "").toString();
            oldPar.endGroup();
            newPar.setValue("sampleName", sampleName);
            newPar.setValue("corrDerivative", "0");
            newPar.setValue("area", "1");
            newPar.setValue("thickness", "1");
        newPar.endGroup();
    }
    QObject::disconnect(this, &Calculator::exportProgress, 0, 0);
    qDebug() << this->metaObject()->className()  <<  this->name <<  "::" << __FUNCTION__ << "exit";
}
