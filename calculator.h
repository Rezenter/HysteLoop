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
    void ready(const int file);
    void dead();

public slots:
    void setLoader(const QString loaderPath, const QString filename);
    void setLoadingSmooth(const int count);
    void update(const int signal = type, const int fileSelection = files, const bool newZeroNorm = zeroNorm);
    void setGrain(const qreal val);
    void setVerticalOffset(const int file, const qreal val);

private:
    QVector<QPointF>* output[2];
    QVector<QPair<qreal, QPointF>> gridFall[2]; //<x, <y, i-zero>> [file] //Reverse order!!!
    QVector<QPair<qreal, QPointF>> gridRise[2]; //<x, <y, i-zero>> [file]
    QString path = "";
    QString filename = "";
    int sensetivity[2] = {1, 1};
    bool loaded[2] = {false, false};
    bool tableFilled[2] = {false, false};
    int loadingSmooth = 5;
    void load();
    void fillTable(const int file, QTextStream* stream);
    void fillGrid(const int file);
    void shuffle(const int file);
    void checkRange(const QPointF point, const int file);
    QPointF xRange;
    QPointF yRange[2];
    static int type;
    static int files; //0 = only first, 1 = only second ,2 = both
    static bool zeroNorm;
    qreal grain = 0.5; //store in settings
    bool shuffled[2] = {false, false};
    QVector<qreal*> storage[2]; //row<value[column]>
    qreal verticalOffset[2] = {0.0, 0.0};
    void fillOutput(const int file);
    bool exported[2] = {false, false};
    QPointF firstRange;
};


#endif // Calculator_H
