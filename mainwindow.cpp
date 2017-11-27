#include "mainwindow.h"
#include "ui_mainwindow.h"


/*
 *To-do:
 *  base
 *  save/load



 *  params dialog

 *      fill fields when file selected
 *      param file in correlation with SM`s programm
 *      save dialog size
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
    uip.setupUi(param);
    uip.element1Box->setEditable(true);
    uip.element1Box->addItems(elements);
    uip.element1Box->setEditable(false);
    uip.element2Box->setEditable(true);
    uip.element2Box->addItems(elements);
    uip.element2Box->setEditable(false);
    QObject::connect(uip.acceptButton, &QPushButton::pressed, this, &MainWindow::accept);
    QObject::connect(uip.discardButton, &QPushButton::pressed, param, &QDialog::reject);
    QObject::connect(param, &QDialog::finished, this, &MainWindow::resetDialog);
    param->setWindowTitle("Assign/edit .PAR");
    param->setModal(true);
    resetDialog(0);
    uic.setupUi(coll);
    QObject::connect(coll, &QDialog::finished, this, &MainWindow::resetColl);
    QObject::connect(uic.save1Button, &QPushButton::pressed, this, &MainWindow::save1);
    QObject::connect(uic.save2Button, &QPushButton::pressed, this, &MainWindow::save2);
    QObject::connect(uic.cancelButton, &QPushButton::pressed, coll, &QDialog::reject);
    coll->setWindowTitle("WARNING! Collision occured.");
    coll->setModal(true);
    resetColl(1);
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
        settings->beginReadArray("elements");
        int wtf = settings->value("size", 0).toInt(); //do not know why i need this
        for(int i = 0; i < wtf; i++){
            settings->setArrayIndex(i);
            elements.append(settings->value("element", "unknown").toString());
        }
        settings->endArray();
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
        settings->beginWriteArray("elements");
            for(int i = 0; i < elements.length(); i++){
                settings->setArrayIndex(i);
                settings->setValue("element", elements.at(i));
            }
        settings->endArray();
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
            QSettings *par = new QSettings(dataDir + "/" + model->fileName(childIndex), QSettings::IniFormat);
            QString tmp;
            par->beginGroup("common");
                /*
                par->setValue("sample", uid.sampleEdit->text());
                par->setValue("angle", uid.angleEdit->text());
                par->setValue("amplitude", uid.ampEdit->text());
                par->setValue("rating", uid.ratingEdit->text());
                par->setValue("comment", uid.commentEdit->toPlainText());
                */
            par->endGroup();
            par->beginGroup("file1");
                tmp = par->value("filename", "none").toString();
                if(tmp != "none"){
                    params.insertMulti(tmp, model->fileName(childIndex));
                }
                /*
                par->setValue("element", uid.element1Edit->text());
                par->setValue("energy", uid.energy1Edit->text());
                par->setValue("rating", uid.rating1Edit->text());
                */
            par->endGroup();
            par->beginGroup("file2");
                tmp = par->value("filename", "none").toString();
                if(tmp != "none"){
                    params.insertMulti(tmp, model->fileName(childIndex));
                }
                /*
                par->setValue("element", uid.element2Edit->text());
                par->setValue("energy", uid.energy2Edit->text());
                par->setValue("rating", uid.rating2Edit->text());
                */
            par->endGroup();
            delete par;
            //params.insert()
            //treat as param
            //save without extension
            //fill table
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
    QItemSelectionModel *select = ui->fileTable->selectionModel();
    QModelIndexList selection = select->selectedRows();
    if(selection.length() == 0){
        //show message
    }else{
        param->show();
        for(int i = 0; i < selection.length(); i++){
            uip.file1Box->addItem(selection.at(i).data().toString());
            uip.file2Box->addItem(selection.at(i).data().toString());
        }
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
    check = 0;
    if(uip.file1Box->currentIndex() == uip.file2Box->currentIndex()){
        check = 1;
        uip.file1Box->setStyleSheet("QComboBox { background-color : red; }");
        uip.file2Box->setStyleSheet("QComboBox { background-color : red; }");
    }else{
        uip.file1Box->setStyleSheet("QComboBox { background-color : white; }");
        uip.file2Box->setStyleSheet("QComboBox { background-color : white; }");
        if(uip.sens1Box->value() == 0 && uip.sens2Box->value() ==0){
            check = 2;
            uip.sens1Box->setStyleSheet("QSpinBox { background-color : red; }");
            uip.sens2Box->setStyleSheet("QSpinBox { background-color : red; }");
        }else{
            uip.sens1Box->setStyleSheet("QSpinBox { background-color : white; }");
            uip.sens2Box->setStyleSheet("QSpinBox { background-color : white; }");
            if(uip.filenameEdit->text().length() == 0){
                check = 3;
                uip.filenameEdit->setStyleSheet("QLineEdit { background-color : red; }");
            }else{
                uip.filenameEdit->setStyleSheet("QLineEdit { background-color : white; }");
                QString entry;
                QSettings *tmp = new QSettings();
                QStringList fileparams = params.values(uip.file1Box->currentText());
                foreach (entry, fileparams) {
                    if(check == 0){
                        delete(tmp);
                        tmp = new QSettings(dataDir + "/" + entry, QSettings::IniFormat);
                        QStringList files = params.keys(entry);
                        tmp->beginGroup("file1");
                        if((tmp->value("filename", "unknown").toString()) == files.at(0)){
                            if(tmp->value("element", "unknown").toString() != uip.element1Box->currentText()){
                                check = 4;
                            }else if(tmp->value("energy", "unknown").toString() != uip.energy1Edit->text()){
                                check = 5;
                            }else if(tmp->value("sensetivity", 0).toInt() != uip.sens1Box->value()){
                                check = 6;
                            }else if(tmp->value("rating", "unknown").toString() != uip.rating1Edit->text()){
                                check = 7;
                            }
                        }else if(check == 0){
                            tmp->endGroup();
                            tmp->beginGroup("file2");
                            if(tmp->value("element", "unknown").toString() != uip.element1Box->currentText()){
                                check = 8;
                            }else if(tmp->value("energy", "unknown").toString() != uip.energy1Edit->text()){
                                check = 9;
                            }else if(tmp->value("sensetivity", 0).toInt() != uip.sens1Box->value()){
                                check = 10;
                            }else if(tmp->value("rating", "unknown").toString() != uip.rating1Edit->text()){
                                check = 11;
                            }
                            if(check == 0){
                                tmp->endGroup();
                                tmp->beginGroup("common");
                                if(tmp->value("sample", "unknown").toString() != uip.sampleEdit->text()){
                                    check = 12;
                                }else if(tmp->value("angle", "unknown").toString() != uip.angleEdit->text()){
                                    check = 13;
                                }
                            }
                        }
                    }else{
                        break;
                    }
                }
                if(check == 0){
                    tmp->endGroup();
                    tmp = new QSettings();
                    fileparams = params.values(uip.file2Box->currentText());
                    foreach (entry, fileparams) {
                        if(check == 0){
                            delete tmp;
                            tmp = new QSettings(dataDir + "/" + entry, QSettings::IniFormat);
                            QStringList files = params.keys(entry);
                            tmp->beginGroup("file1");
                            if((tmp->value("filename", "unknown").toString()) == files.at(0)){
                                if(tmp->value("element", "unknown").toString() != uip.element2Box->currentText()){
                                    check = 4;
                                }else if(tmp->value("energy", "unknown").toString() != uip.energy2Edit->text()){
                                    check = 5;
                                }else if(tmp->value("sensetivity", 0).toInt() != uip.sens2Box->value()){
                                    check = 6;
                                }else if(tmp->value("rating", "unknown").toString() != uip.rating2Edit->text()){
                                    check = 7;
                                }
                            }else if(check == 0){
                                tmp->endGroup();
                                tmp->beginGroup("file2");
                                if(tmp->value("element", "unknown").toString() != uip.element2Box->currentText()){
                                    check = 8;
                                }else if(tmp->value("energy", "unknown").toString() != uip.energy2Edit->text()){
                                    check = 9;
                                }else if(tmp->value("sensetivity", 0).toInt() != uip.sens2Box->value()){
                                    check = 10;
                                }else if(tmp->value("rating", "unknown").toString() != uip.rating2Edit->text()){
                                    check = 11;
                                }
                                if(check == 0){
                                    tmp->endGroup();
                                    tmp->beginGroup("common");
                                    if(tmp->value("sample", "unknown").toString() != uip.sampleEdit->text()){
                                        check = 12;
                                    }else if(tmp->value("angle", "unknown").toString() != uip.angleEdit->text()){
                                        check = 13;
                                    }
                                }
                            }
                        }else{
                            break;
                        }
                    }
                }
                QString name;
                QString v1;
                QString v2;
                switch (check) {
                    case 4:
                        name = "element";
                        v1 = tmp->value("element", "unknown").toString();
                        v2 = uip.element1Box->currentText();
                        break;
                    case 5:
                        name = "energy";
                        v1 = tmp->value("energy", "unknown").toString();
                        v2 = uip.energy1Edit->text();
                        break;
                    case 6:
                        name = "sensetivity";
                        v1 = tmp->value("sensetivity", "unknown").toString();
                        v2 = QString::number(uip.sens1Box->value());
                        break;
                    case 7:
                        name = "rating";
                        v1 = tmp->value("rating", "unknown").toString();
                        v2 = uip.rating1Edit->text();
                        break;
                    case 8:
                        name = "element";
                        v1 = tmp->value("element", "unknown").toString();
                        v2 = uip.element2Box->currentText();
                        break;
                    case 9:
                        name = "energy";
                        v1 = tmp->value("energy", "unknown").toString();
                        v2 = uip.energy2Edit->text();
                        break;
                    case 10:
                        name = "sensetivity";
                        v1 = tmp->value("sensetivity", "unknown").toString();
                        v2 = QString::number(uip.sens2Box->value());
                        break;
                    case 11:
                        name = "rating";
                        v1 = tmp->value("rating", "unknown").toString();
                        v2 = uip.rating2Edit->text();
                        break;
                    case 12:
                        name = "sample";
                        v1 = tmp->value("sample", "unknown").toString();
                        v2 = uip.sampleEdit->text();
                        break;
                    case 13:
                        name = "angle";
                        v1 = tmp->value("angle", "unknown").toString();
                        v2 = uip.angleEdit->text();
                        break;
                    default:
                        break;
                }
                tmp->endGroup();
                delete(tmp);
                if(check != 0){
                    collision(name, entry, v2, v1);
                    error = check;
                }
            }
        }
    }
    if(check == 0){
        param->accept();
    }
}

void MainWindow::resetDialog(int r){
    if(r == 1){
        if(!params.contains(uip.filenameEdit->text())){
            writeParam(uip.filenameEdit->text());
        }
    }
    uip.file1Box->clear();
    uip.file2Box->clear();
    uip.file1Box->addItem("none");
    uip.file2Box->addItem("none");
    uip.file1Box->setStyleSheet("QComboBox { background-color : white; }");
    uip.file2Box->setStyleSheet("QComboBox { background-color : white; }");
    uip.sens1Box->setStyleSheet("QSpinBox { background-color : white; }");
    uip.sens2Box->setStyleSheet("QSpinBox { background-color : white; }");
    uip.filenameEdit->setStyleSheet("QLineEdit { background-color : white; }");
    uip.filenameEdit->clear();
    uip.sampleEdit->clear();
    uip.angleEdit->clear();
    uip.element1Box->setCurrentIndex(0);
    uip.energy1Edit->clear();
    uip.element2Box->setCurrentIndex(0);
    uip.energy2Edit->clear();
    uip.ampEdit->clear();
    uip.ratingEdit->clear();
    uip.rating1Edit->clear();
    uip.rating2Edit->clear();
    uip.commentEdit->clear();
    uip.sens1Box->clear();
    uip.sens2Box->clear();
}

void MainWindow::writeParam(QString name){
    QSettings *par = new QSettings(dataDir + "/" + name + ".PAR", QSettings::IniFormat);
    par->beginGroup("common");
        par->setValue("sample", uip.sampleEdit->text());
        par->setValue("angle", uip.angleEdit->text());
        par->setValue("amplitude", uip.ampEdit->text());
        par->setValue("rating", uip.ratingEdit->text());
        par->setValue("comment", uip.commentEdit->toPlainText());
    par->endGroup();
    par->beginGroup("file1");
        par->setValue("filename", uip.file1Box->currentText());
        par->setValue("element", uip.element1Box->currentText());
        par->setValue("energy", uip.energy1Edit->text());
        par->setValue("rating", uip.rating1Edit->text());
        par->setValue("sensetivity", uip.sens1Box->value());
    par->endGroup();
    par->beginGroup("file2");
        par->setValue("filename", uip.file2Box->currentText());
        par->setValue("element", uip.element2Box->currentText());
        par->setValue("energy", uip.energy2Edit->text());
        par->setValue("rating", uip.rating2Edit->text());
        par->setValue("sensetivity", uip.sens1Box->value());
    par->endGroup();
    delete par;
}

void MainWindow::save1(){
    QString val = uic.value1Label->text();
    QSettings *par = new QSettings(dataDir + "/" + uic.file2Box->title(), QSettings::IniFormat);
    if(error >= 4 && error < 8){
        par->beginGroup("file1");
    }else if(error >=8 && error < 12){
        par->beginGroup("file2");
    }else if(error >=12 && error < 14){
        par->beginGroup("common");
    }
    switch (error) {
        case 4:
            par->setValue("element", val);
            break;
        case 5:
            par->setValue("energy", val);
            break;
        case 6:
            par->setValue("sensetivity", val);
            break;
        case 7:
            par->setValue("rating", val);
            break;
        case 8:
            par->setValue("element", val);
            break;
        case 9:
            par->setValue("energy", val);
            break;
        case 10:
            par->setValue("sensetivity", val);
            break;
        case 11:
            par->setValue("rating", val);
            break;
        case 12:
            par->setValue("sample", val);
            break;
        case 13:
            par->setValue("angle", val);
            break;
        default:
            break;
    }
    coll->accept();
    accept();
    par->endGroup();
    delete par;
}

void MainWindow::save2(){
    QString val = uic.value2Label->text();
    switch (error) {
        case 4:
            uip.element1Box->setCurrentText(val);
            break;
        case 5:
            uip.energy1Edit->setText(val);
            break;
        case 6:
            uip.sens1Box->setValue(val.toInt());
            break;
        case 7:
            uip.rating1Edit->setText(val);
            break;
        case 8:
            uip.element2Box->setCurrentText(val);
            break;
        case 9:
            uip.energy2Edit->setText(val);
            break;
        case 10:
            uip.sens2Box->setValue(val.toInt());
            break;
        case 11:
            uip.rating2Edit->setText(val);
            break;
        case 12:
            uip.sampleEdit->setText(val);
            break;
        case 13:
            uip.angleEdit->setText(val);
            break;
        default:
            break;
    }
    coll->accept();
    accept();
}

void MainWindow::resetColl(int r){
    Q_UNUSED(r);
    error = 0;
    uic.valueName->setText("");
    uic.file2Box->setObjectName("");
    uic.value1Label->setText("");
    uic.value2Label->setText("");
}

void MainWindow::collision(QString n, QString p, QString v1, QString v2){
    uic.valueName->setText(n);
    uic.file2Box->setTitle(p);
    uic.value1Label->setText(v1);
    uic.value2Label->setText(v2);
    coll->show();
}
