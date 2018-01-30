#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QDebug>

#include <QObject>
#include "fileloader.h"
#include <QPointF>
#include <QtMath>

class Calculator : public QObject{

    Q_OBJECT

public:
    Calculator(QObject *parent = 0);
    ~Calculator();

signals:
    void dead();
    void processedData(const QVector<QPair<qreal, QPointF>> points, const int file);
    void rawData(const QVector<QPair<qreal, QPointF>> points, const int file);
    void iZero(const QVector<QPair<qreal, QPointF>> points, const int file);
    void XMCD(const QVector<QPointF> points, const int file);
    void integrals(const qreal summ, const qreal dl2, const qreal dl3, const qreal mSE, const qreal mO, const int file);
    void moments(const qreal mOP, const qreal mOO, const qreal ms, const qreal mt);

public slots:
    void setLoader(FileLoader *loader, const int file);
    void setLimits(const qreal left, const qreal right, const int file);
    void setEnergyShift(const qreal shift);
    void setShadowCurrent(const qreal signal, const qreal iZero, const int file);
    void setSmooth(const int count, const int file);
    void setDiff(const bool needed, const int file);
    void setLinearIntervals(const QPointF interval, bool needed, const int file);
    void setNormalizationCoeff(const qreal coeff, bool needed, const int file);
    void setStepped(const qreal coeff, bool needed, const int file);
    void setIntegrate(const bool needed, const int index, const int file);
    void setIntegrationConstants(const qreal newPc, const qreal newNh);
    void setCalculate(const bool needed, const QPointF newPhi, const QPointF newTheta);

private:
    QVector<QPair<qreal, QPointF>> data[2];
    QVector<QPair<qreal, QPointF>> bare[2];
    QVector<QPair<qreal, QPointF>> zero[2];
    QVector<QPair<qreal, QPointF>> smoothedData[2];
    QVector<QPair<qreal, QPointF>> normData[2];
    QVector<QPair<qreal, QPointF>> fitData[2];
    QVector<QPair<qreal, QPointF>> finalData[2];
    QVector<QPointF> diff[2];
    QVector<QPointF> finalDiff[2];
    FileLoader *loader[2];
    FileLoader *tmpLoader[2];
    bool loaderChanged[2] = {false};
    bool loaded[2] = {false};
    bool shadowChanged[2] = {false};
    bool smoothChanged[2] = {false};
    bool normalizationChanged[2] = {false};
    bool normalizationNeeded[2] = {false};
    bool tmpNormalizationNeeded[2] = {false};
    bool linearIntervalsChanged[2] = {false};
    bool linearNeeded[2] = {false};
    bool tmpLinearNeeded[2] = {false};
    bool diffNeeded[2] = {false};
    bool tmpDiffNeeded[2] = {false};
    bool diffChanged[2] = {false};
    bool steppedNeeded[2] = {false};
    bool tmpSteppedNeeded[2] = {false};
    bool steppedCoeffChanged[2] = {false};
    bool integrationChanged[2] = {false};
    bool integrateNeeded[2] = {false};
    bool tmpIntegrateNeeded[2] = {false};
    bool stepFitNeeded[2] = {false};//currently constantly false
    bool ready = true;
    bool constantsChanged = false;
    bool calculateNeeded = false;
    bool calculateChanged = false;
    bool tmpCalculateNeeded = false;
    QPointF linearCoeff[2][2]; //[file][left, right](a, b)
    QPointF limits[2] = {QPointF(std::numeric_limits<qreal>::min(), std::numeric_limits<qreal>::max())};
    QPointF tmpLimits[2] = {QPointF(std::numeric_limits<qreal>::min(), std::numeric_limits<qreal>::max())};
    QPointF shadow[2] = {QPointF(0.0, 0.0)};
    QPointF tmpShadow[2] = {QPointF(0.0, 0.0)};
    QPointF max[2] = {QPointF(0.0, 0.0)};
    QPointF linearIntervals[2] = {QPointF(2.0, 2.0)};
    QPointF tmpLinearIntervals[2] = {QPointF(2.0, 2.0)};
    QPointF phi = {QPointF(0.0 , 0.0)};
    QPointF tmpPhi = {QPointF(0.0 , 0.0)};
    QPointF theta = {QPointF(0.0 , 0.0)};
    QPointF tmpTheta = {QPointF(0.0 , 0.0)};
    qreal energyShift = 0.0;
    qreal tmpEnergyShift = 0.0;
    qreal normalizationCoeff[2][3] = {}; //[file][a, b, c]
    qreal tmpNormalizationCoeff[2][3] = {};
    qreal steppedCoeff[2] = {10.0};
    qreal tmpSteppedCoeff[2] = {10.0};
    qreal summInt[2] = {0.0};
    qreal dl2Int[2] = {0.0};
    qreal dl3Int[2] = {0.0};
    qreal mSEff[2] = {0.0};
    qreal mOrb[2] = {0.0};
    qreal mOrbP = 0.0;
    qreal mOrbO = 0.0;
    qreal mS = 0.0;
    qreal mT = 0.0;
    qreal constant;
    qreal tmpConstant;
    void load(const int file);
    void smooth(const int file);
    void calcData(const int file);
    void normalize(const int file);
    void apply();
    void linear(const int file);
    void calcDiff(const int file);
    void stepped(const int file);
    void integrate(const int file);
    void calculate();
    int reset();// -1 = nothing to do, 0 = running calculations, 1+ = stages
    int smoothPoints[2] = {1};
    int tmpSmoothPoints[2] = {1};
    int lEdges[2] = {0};
    int rEdges[2] = {0};
    int separator[2];
    int tmpSeparator[2];
};

#endif // Calculator_H
