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
    void setSmooth(const int count, const int file);
    void update(const int signal, const int files);

private:
    Ui::MainWindow* ui;
    struct Calc{
        QString name;
        QThread* thread;
        Calculator* calc;
        QPointF xRange;
        QPointF yRange;
        QtCharts::QLineSeries* series[2];
        QVector<QPointF>* data[2];
        qreal grain;
        int signal;
        int files;
        qreal verticalFit[2];
        int loadingSmooth;
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
    void loadPar(QString name);
    QString loaded = "";
    QList<QPointF> dots;
    int loadingSmooth = 5;
    QHash<QString, Calc*> calculators;
    qreal grain = 0.5;
    Calc* current();

private slots:
    void buildFileTable(QString);
    void load(QString);
};

#endif // MAINWINDOW_H
