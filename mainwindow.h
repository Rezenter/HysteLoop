#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRegExpValidator>
#include <QLineSeries>
#include <QChartView>
#include <QValueAxis>
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
#include "ui_paramdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Ui::Dialog uid;
    void loadSettings();
    void saveSettings();
    QtCharts::QChartView *chartView;
    QtCharts::QChart *chart;
    QList<QtCharts::QLineSeries *> series;
    QtCharts::QValueAxis *axisX;
    QtCharts::QValueAxis *axisY;
    QString path = QCoreApplication::applicationDirPath();
    QFile *save;
    QTextStream *stream;
    QSettings *settings;
    QFileSystemModel *model = new QFileSystemModel(this);
    ExtFSM *table = new ExtFSM(this);
    QString drive;
    QModelIndex currentSelection;
    QString dataDir;
    QFileSystemWatcher *refresh = new QFileSystemWatcher();
    QHash<QString, QString> params;
    QModelIndex tmpInd;
    bool dat = true;
    bool par = true;
    QDialog *param = new QDialog(this);

private slots:
    void resetDialog(int);
    void folderButton();
    void paramButton();
    void exportButton();
    void filter();
    void zP();
    void zM();
    void zO();
    void tmp(int, int);
    void buildFileTable(QString);
    void load(QString);
    void checkBoxes();
    void accept();
};

#endif // MAINWINDOW_H
