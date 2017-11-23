#include "mainwindow.h"
#include "ui_mainwindow.h"


/*
 *To-do:
 *  base
 *  save/load

 *  store params in dictionary!
 *      add warning in collisions
 *  params dialog
 *      param file in correlation with SM`s programm
 *  multiple loop comparaison
 *      moovable legend
 *      mouse position
 *  multiple file selection from the keyboard
 *      file selection dialog for measurments with multiple pair variants
 *  export dialog
 *      export in correlation with SM`s programm
 *  load from param file
 *  calculate coercitivity, amplitude and offset
 *  errors dialog
 *  table filters
 *  zoom
 *      mouse zoom
 *  smooth
 *  mb same sample filter?
 *
*/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Hysteresis loop viewer");
    ui->splitter->setHandleWidth(5);
    settings = new QSettings(QApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat);
    chart = new QtCharts::QChart();
    axisX = new QtCharts::QValueAxis;
    axisX->setMinorGridLineVisible(true);
    axisY = new QtCharts::QValueAxis();
    chartView = new QtCharts::QChartView(chart);
    ui->splitter->addWidget(chartView);
    chartView->show();
    ui->fileTable->setModel(table);
    ui->fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fileTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->fileTable->horizontalHeader()->setStretchLastSection(true);
    QObject::connect(ui->folderButton, &QPushButton::pressed, this, &MainWindow::folderButton);
    QObject::connect(ui->exportButton, &QPushButton::pressed, this, &MainWindow::exportButton);
    QObject::connect(ui->paramButton, &QPushButton::pressed, this, &MainWindow::paramButton);
    QObject::connect(ui->zoomOButton, &QPushButton::pressed, this, &MainWindow::zO);
    QObject::connect(ui->zoomPButton, &QPushButton::pressed, this, &MainWindow::zP);
    QObject::connect(ui->zoomMButton, &QPushButton::pressed, this, &MainWindow::zM);
    QObject::connect(ui->splitter, &QSplitter::splitterMoved, this, &MainWindow::tmp);
    QObject::connect(model, &QFileSystemModel::directoryLoaded, this, &MainWindow::buildFileTable);
    QObject::connect(refresh, &QFileSystemWatcher::directoryChanged, this, &MainWindow::load);
    QObject::connect(ui->datBox, &QCheckBox::clicked, this, &MainWindow::checkBoxes);
    QObject::connect(ui->parBox, &QCheckBox::clicked, this, &MainWindow::checkBoxes);
    loadSettings();
    uid.setupUi(param);
    QObject::connect(uid.acceptButton, &QPushButton::pressed, this, &MainWindow::accept);
    QObject::connect(uid.discardButton, &QPushButton::pressed, param, &QDialog::reject);
    QObject::connect(param, &QDialog::finished, this, &MainWindow::resetDialog);
    param->setWindowTitle("Assign/edit .PAR");
    param->setModal(true);

}

MainWindow::~MainWindow()
{
    saveSettings();
    delete settings;
    delete model;
    QtCharts::QAbstractSeries *delTmp;
    foreach(delTmp, chart->series()){
        delete &delTmp;
    }
    //delete axis?
    delete chart;
    delete chartView;
    delete ui;
}


void MainWindow::loadSettings(){
    settings->beginGroup("gui");
        resize(settings->value("windowSize", QSize(800, 600)).toSize());
        move(settings->value("windowPosition", QPoint(0, 0)).toPoint());
        settings->beginGroup("splitter");
            settings->beginWriteArray("splitterSizes");
                QList<int> sizes;
                for(int i=0; i< ui->splitter->count(); i++){
                    settings->setArrayIndex(i);
                    sizes.append(settings->value("size", 50).toInt());
                }
                ui->splitter->setSizes(sizes);
            settings->endArray();
            settings->beginGroup("fileTable");
                settings->beginWriteArray("columns");
                    for(int i = 0; i < table->columnCount(QModelIndex()); i++){
                        settings->setArrayIndex(i);
                        ui->fileTable->setColumnWidth(i, settings->value("columnWidth", 50).toInt());
                    }
                settings->endArray();
            settings->endGroup();
        settings->endGroup();
    settings->endGroup();
    settings->beginGroup("var");
        dataDir = settings->value("dataDir", "C:/").toString();
        if(!QDir(dataDir).exists()){
            dataDir = "C:/";
        }
        load(dataDir);
    settings->endGroup();
}

void MainWindow::saveSettings(){
    settings->remove("");
    settings->beginGroup("gui");
        settings->setValue("windowPosition", this->pos());
        settings->setValue("windowSize", this->size());
        settings->beginGroup("splitter");
            settings->beginWriteArray("splitterSizes");
                QList<int> sizes = ui->splitter->sizes();
                for(int i=0; i< ui->splitter->count(); i++){
                    settings->setArrayIndex(i);
                    settings->setValue("size", sizes.at(i));
                }
            settings->endArray();
            settings->beginGroup("fileTable");
                settings->beginWriteArray("columns");
                    for(int i = 0; i < table->columnCount(QModelIndex()); i++){
                        settings->setArrayIndex(i);
                        settings->setValue("columnWidth", ui->fileTable->columnWidth(i));
                    }
                settings->endArray();
            settings->endGroup();
        settings->endGroup();
    settings->endGroup();
    settings->beginGroup("var");
        settings->setValue("dataDir", dataDir);
    settings->endGroup();
}

void MainWindow::buildFileTable(QString d){
    Q_UNUSED(d);
    params.clear();
    table->removeAll();
    ui->dataDirLabel->setText(dataDir);
    QModelIndex childIndex;
    int tableRow = 0;
    for(int i = 0; i < model->rowCount(model->index(dataDir)); i++){
        childIndex = model->index(i, 0, model->index(dataDir));
        if(model->fileName(childIndex).endsWith(".csv", Qt::CaseInsensitive)){
            if(ui->datBox->isChecked()){
                table->insertRow(tableRow, QModelIndex());
                for(int j = 0; j < table->columnCount(); j++){
                    tmpInd = table->index(tableRow, j, QModelIndex());
                    switch (j) {
                        case 0:
                            table->setData(tmpInd, model->fileName(childIndex), Qt::EditRole);
                        break;

                        case 5:
                            table->setData(tmpInd, model->fileInfo(childIndex).lastModified().toString("dd.MM.yyyy   HH:m"), Qt::EditRole);
                        break;

                        default:
                            //qDebug() << i << j;
                        break;
                    }
                }
                tableRow++;
            }
        }else if(model->fileName(childIndex).endsWith(".PAR", Qt::CaseInsensitive)){
            //treat as param
        }else{
            //wtf, do nothing
        }
    }
    ui->fileTable->sortByColumn(0, Qt::AscendingOrder);
}

void MainWindow::folderButton(){
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select data directory"),
                                                        dataDir,
                                                         QFileDialog::DontUseNativeDialog);
        if(dir.length() != 0){
            dataDir = dir;
            buildFileTable(dataDir);
        }
}

void MainWindow::paramButton(){
    param->show();
    QItemSelectionModel *select = ui->fileTable->selectionModel();
    QModelIndexList selection = select->selectedRows();
    for(int i = 0; i < selection.length(); i++){
        uid.file1Box->addItem(selection.at(i).data().toString());
        uid.file2Box->addItem(selection.at(i).data().toString());
    }


}

void MainWindow::exportButton(){

}

void MainWindow::filter(){

}

void MainWindow::zP(){

}

void MainWindow::zM(){

}

void MainWindow::zO(){

}

void MainWindow::tmp(int pos, int index){

}

void MainWindow::load(QString p){
    model->setRootPath(p);
    ui->dataDirLabel->setText("Loading, please, wait.");
}

void MainWindow::checkBoxes(){
    if(!ui->datBox->isChecked() && !ui->parBox->isChecked()){
        ui->datBox->setChecked(dat);
        ui->parBox->setChecked(par);
    }else{
        dat = ui->datBox->isChecked();
        par = ui->parBox->isChecked();
        buildFileTable(dataDir);
    }
}

void MainWindow::accept(){
    bool check = true;
    if(uid.file1Box->currentIndex() == uid.file2Box->currentText()){
        check = false;
    }else if(uid.sens1Box->value() == 0 && uid.sens2Box ==0){
        check = false;
    }
    QStringList file1params = params.values(uid.file1Box->currentText());
    //check for collisions in params
    if(check){
        //save
        param->accept();
    }
}

void MainWindow::resetDialog(int r){
    qDebug() << "reset" << r;
}
