#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineSeries>
#include <QChartView>
#include <QValueAxis>
#include <QSettings>
#include <QFileSystemModel>
#include <QItemSelection>
#include <QHash>
#include <QList>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QDialog>
#include <QMouseEvent>
#include <QMessageBox>
#include <QThread>

#include "calculator.h"

#include <QDebug>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

signals:
    void setGrain(const qreal val);
    void setLoader(const QString loaderPath, const QString filename);
    void setLoadingSmooth(const int count);
    void update(const int signal, const int files, const bool zeroNorm);
    void setCriticalDer(const int file, const qreal val);
    void updateSplit(const int file, const int rise, const int index);
    void exportSeries(const QString path);

private:
    Ui::MainWindow* ui;
    struct Calc{
        QString name;
        QThread* thread;
        Calculator* calc;
        QPointF xRange;
        QPointF yRange;
        std::array<QtCharts::QLineSeries*, 2> series;
        std::array<QVector<QPointF>*, 2> data;
        qreal grain;
        int signal;
        int files;
        std::array<qreal, 2> verticalFit;
        int loadingSmooth;
        bool zeroNorm;
        std::array<bool, 2> visability;
        std::array<qreal, 2> criticalDer;
    };
    void loadSettings();
    void saveSettings();
    void resizeChart();
    QtCharts::QChartView* chartView;
    QtCharts::QChart* chart;
    QtCharts::QValueAxis* axisX;
    QtCharts::QValueAxis* axisY;
    QString path = QCoreApplication::applicationDirPath();
    QFile* save;
    QTextStream* stream;
    QSettings* settings;
    QFileSystemModel* model = new QFileSystemModel(this);
    QString drive;
    QModelIndex currentSelection;
    QString dataDir;
    QFileSystemWatcher* refresh = new QFileSystemWatcher();
    QHash<QString, QString> params;
    QModelIndex tmpInd;
    QDialog* param = new QDialog(this);
    void writeParam(QString);
    QStringList elements;
    QStringList dats;
    QtCharts::QLineSeries* xZeroSer;
    QtCharts::QLineSeries* yZeroSer;
    QtCharts::QLineSeries* splitSeries;
    void loadPar(QString name);
    QString loaded = "";
    QList<QPointF> dots;
    int loadingSmooth = 5;
    QHash<QString, Calc*> calculators;
    qreal grain = 0.5;
    Calc* current();
    void emitUpdate(const Calc* curr);
    std::array<qreal, 2> criticalDer{{0.015, 0.015}};
    std::array<qreal, 2> verticalFit{{0.0, 0.0}};

private slots:
    void buildFileTable(QString);
    void load(QString);
    void updateSplits(const int file, const int index);
};

#endif // MAINWINDOW_H
