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
    QMutex* mutex;
    struct Split{
        qreal x;
        int index;
        int l = 20;
        int r = 20;
        qreal shift = 0.5;
        QVector<QPointF>* original;
    };
    std::array<qreal, 2> offsets{{0.0, 0.0}};
    std::array<std::array<QList<Calculator::Split*>*, 2> ,2> splits;//[file][rise/fall] // rise = 1 , fall = 0
    ~Calculator();

signals:
    void dataPointers(QVector<QPointF>* edge, QVector<QPointF>* preEdge, QStringList names);
    void range(QPointF xRange, QPointF yRange);
    void ready(const int file);
    void updateSplits(const int file, const int index = -1);
    void dead();

public slots:
    void setLoader(const QString loaderPath, const QString filename);
    void setLoadingSmooth(const int count);
    void update(const int signal = type, const int fileSelection = files, const bool newZeroNorm = zeroNorm);
    void setGrain(const qreal val);
    void setVerticalOffset(const int file, const qreal val);
    void setCriticalDer(const qreal val);
    void updateSplit(const int file, const int rise, const int index);

private:
    std::array<QVector<QPointF>*, 2> output;
    std::array<std::array<QVector<QPair<qreal, QPointF>>, 2>, 2> grid;//<x, <y, i-zero>>[file][rise/fall] // rise = 1 , fall = 0 reverse order!
    std::array<std::array<QVector<QPair<qreal, QPointF>>, 2>, 2> fixedGrid;
    QString path = "";
    QString filename = "";
    std::array<int, 2> sensetivity{{1, 1}};
    std::array<bool, 2> loaded{{false, false}};
    std::array<bool, 2> tableFilled{{false, false}};
    int loadingSmooth = 5;
    void load();
    void fillTable(const int file, QTextStream* stream);
    void fillGrid(const int file);
    void shuffle(const int file);
    void checkRange(const QPointF point, const int file);
    QPointF xRange;
    std::array<QPointF, 2> yRange;
    static int type;
    static int files; //0 = only first, 1 = only second ,2 = both
    static bool zeroNorm;
    qreal grain = 0.5; //store in settings
    std::array<bool, 2> shuffled{{false, false}};
    std::array<QVector<qreal*>, 2> storage; //row<value[column]>[file]
    std::array<qreal, 2> verticalOffset{{0.0, 0.0}};
    void fillOutput(const int file);
    std::array<bool, 2> exported{{false, false}};
    QPointF firstRange;
    qreal criticalDer = 0.015;
    void findSplits(const int file);
};


#endif // Calculator_H
