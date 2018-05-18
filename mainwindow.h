#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRegExpValidator>
#include <QLineSeries>
#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>
#include <QSettings>
#include <QFileSystemModel>
#include "extfsm.h"
#include <QItemSelection>
#include "fileloader.h"
#include <QHash>
#include <QList>
#include <QCoreApplication>
#include <QFileDialog>
#include <QDebug>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QMouseEvent>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow* ui;
    void loadSettings();
    void saveSettings();
    QtCharts::QChartView* chartView;
    QtCharts::QChart* chart;
    QtCharts::QLineSeries* series;
    QtCharts::QValueAxis* axisX;
    QtCharts::QValueAxis* axisY;
    QString path = QCoreApplication::applicationDirPath();
    QFile* save;
    QTextStream* stream;
    QSettings* settings;
    QFileSystemModel* model = new QFileSystemModel(this);
    ExtFSM* table = new ExtFSM(this);
    QString drive;
    QModelIndex currentSelection;
    QString dataDir;
    QFileSystemWatcher* refresh = new QFileSystemWatcher();
    QHash<QString, QString> params;
    QModelIndex tmpInd;
    QDialog* param = new QDialog(this);
    void writeParam(QString);
    QStringList elements;
    int error = 0;
    QStringList dats;
    QSortFilterProxyModel proxy;
    QtCharts::QLineSeries* xZeroSer;
    QtCharts::QLineSeries* yZeroSer;
    void loadPar(QString name);
    QString loaded = "";

private slots:
    void buildFileTable(QString);
    void load(QString);
};

#endif // MAINWINDOW_H
