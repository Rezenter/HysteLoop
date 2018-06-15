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
    void setLoader(const QString loaderPath, const QString filename);
    void setSmooth(const int count, const int file);
    void update();

private:
    Ui::MainWindow* ui;
    void loadSettings();
    void saveSettings();
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
    QHash<QString, QPair<QThread*, Calculator*>> calculators;
    int loadingSmooth = 5; //from par

private slots:
    void buildFileTable(QString);
    void load(QString);
};

#endif // MAINWINDOW_H
