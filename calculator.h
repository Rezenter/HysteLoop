#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QObject>
#include <QPointF>
#include <QtMath>
#include <QMutex>
#include <QTextStream>
#include <QVector>

class Calculator : public QObject{

    Q_OBJECT

public:
    Calculator(QObject *parent = 0);
    QMutex mutex;
    ~Calculator();

signals:
    void dataPointers(QVector<QPointF>* edge, QVector<QPointF>* preEdge, QStringList names);
    void range(QPointF xRange, QPointF yRange);
    void dead();

public slots:
    void setLoader(const QString loaderPath, const QString filename);
    void setLoadingSmooth(const int count, const int file);
    void update(const int signal);
    void setGrainMult(const int file, const qreal mult);

private:
    QVector<QPointF>* output[2];
    QVector<QPair<qreal, QPointF>> gridFall[2]; //<x, <y, i-zero>> [file]
    QVector<QPair<qreal, QPointF>> gridRise[2]; //<x, <y, i-zero>> [file]
    QString path = "";
    QString filename = "";
    int sensetivity[2] = {1, 1};
    bool loaded[2] = {false, false};
    bool tableFilled[2] = {false, false};
    int loadingSmooth[2] = {5, 5};
    bool ready = true;
    void load();
    void fillTable(const int file, QTextStream* stream);
    void fillGrid(const int file);
    void shuffle(const int file);
    void checkRange(QPointF point);
    QPointF xRange;
    QPointF yRange;
    const int types = 1;
    int type = 0;
    qreal grain[2] = {0.5, 0.5}; //store in settings
    qreal grainMult[2] = {1.0, 1.0};
    bool shuffled[2] = {false, false};
    QVector<qreal*> storage[2]; //row<value[column]>
};


#endif // Calculator_H
