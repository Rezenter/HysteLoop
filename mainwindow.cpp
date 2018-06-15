#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QElapsedTimer>

/*
 *To-do:
 *
 * common grid!!!
 *
 * second file in export
 * default export name
 * table sorting
 * multifile export
 * multifile comparasion with common export
 * smooth
 * artifact correction
 * make xScale != const
 * oscillation smooth?
 * highlight default values in peram panel
 *
 *  base
 *      create common grid instead of finding zero

 *      separate loading and drawing functions
 *      calculate at loading
 *      move loading to thread
 *      check loading
 *          !!!! change vectors to lists !!!! WTF i meant?



 *  multiple loop comparaison
 *      mouse position
 *          subclass smth in order to modify mousemoveevent
 *      loading progress

 *  calculate coercitivity, amplitude and offset

 *
*/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    ui->setupUi(this);
    setWindowTitle("Hysteresis loop viewer");
    settings = new QSettings(QApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat);
    chart = new QtCharts::QChart();
    axisX = new QtCharts::QValueAxis;
    axisX->setMin(0.0);
    axisX->setMax(0.0);
    axisX->setMinorGridLineVisible(true);
    axisY = new QtCharts::QValueAxis();
    axisY->setMin(0.0);
    axisY->setMax(0.0);
    chartView = new QtCharts::QChartView(chart);
    ui->chartWidget->setLayout(new QGridLayout(ui->chartWidget));
    ui->chartWidget->layout()->addWidget(chartView);
    chartView->show();
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    axisX->setTickCount(11);
    axisX->setMinorTickCount(5);
    axisX->setTitleText("Magnetic flux density [mT]");
    xZeroSer = new QtCharts::QLineSeries();
    xZeroSer->append(0, -10);
    xZeroSer->append(0, 10);
    chart->addSeries(xZeroSer);
    xZeroSer->attachAxis(axisX);
    xZeroSer->attachAxis(axisY);
    xZeroSer->setName("(0, y)");
    axisY->setTickCount(11);
    axisY->setMinorTickCount(5);
    axisY->setTitleText("Signal [arb.u.]");
    yZeroSer = new QtCharts::QLineSeries();
    yZeroSer->append(-300, 0);
    yZeroSer->append(300, 0);
    chart->addSeries(yZeroSer);
    yZeroSer->attachAxis(axisX);
    yZeroSer->attachAxis(axisY);
    yZeroSer->setName("(x, 0)");
    QStringList labels;
    labels.append("Filename");
    labels.append("sample");
    labels.append("element");
    labels.append("angle");
    labels.append("rating");
    labels.append("date");
    labels.append("comment");
    labels.append("loaded");
    ui->fileTable->setColumnCount(labels.size());
    ui->fileTable->setHorizontalHeaderLabels(labels);
    ui->fileTable->setColumnHidden(labels.size() - 1, true);
    ui->fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->fileTable->horizontalHeader()->setStretchLastSection(true);
    QObject::connect(ui->folderButton, &QPushButton::pressed, this, [=]{
        qDebug() << this->metaObject()->className() <<  "::folderButton::pressed";
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select data directory"),
                                                            dataDir,
                                                             QFileDialog::DontUseNativeDialog);
        if(dir.length() != 0){
            dataDir = dir;
            load(dataDir);
            buildFileTable(dataDir);
        }
        qDebug() << this->metaObject()->className() <<  "::folderButton::pressed::exit";
    });
    QObject::connect(ui->exportButton, &QPushButton::pressed, this, [=]{
        qDebug() << this->metaObject()->className() << "::exportButton::pressed";
        QString output = QFileDialog::getSaveFileName(this, tr("Select export path"), dataDir, QString(), nullptr,
                                                      QFileDialog::DontConfirmOverwrite);
        if(output.length() != 0){
            QFile file(output + ".DAT");
            file.open(QIODevice::WriteOnly);
            QTextStream stream(&file);
            stream << "Time (unused)" << "\t" << "Temp. (unused)" << "\t" << "Magn.Field (Oe)"  << "\t" << "Moment (arb.units)" << "\r\n";
            int i = 0;

            //fix!___________________________________________________________________________________________debug only

            foreach (QPointF point, dynamic_cast<QtCharts::QLineSeries *>(chart->series().at(0))->points()) {
                stream << i++ << "\t" << 300.0 << "\t" <<point.x()*10 << "\t" << point.y() << "\r\n";
            }

            file.close();
        }
        qDebug() << this->metaObject()->className() << "::exportButton::pressed::exit";
    });
    QObject::connect(model, &QFileSystemModel::directoryLoaded, this, &MainWindow::buildFileTable);
    QObject::connect(refresh, &QFileSystemWatcher::directoryChanged, this, &MainWindow::load);
    QObject::connect(ui->verticalFitSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](qreal val){
        qDebug() << this->metaObject()->className() << "::verticalFitSpinBox::valueChanged" << val;
        //not implemented yet
        qDebug() << this->metaObject()->className() << "::verticalFitSpinBox::valueChanged::exit";
    });
    QObject::connect(ui->smoothSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](qreal val){
        qDebug() << this->metaObject()->className() << "::smoothSpinBox::valueChanged" << val;
        //not implemented. separeted calculator needed.
        qDebug() << this->metaObject()->className() << "::smoothSpinBox::valueChanged::exit";
    });
    QObject::connect(ui->fileTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]{
        qDebug() << this->metaObject()->className() << "::selectionChanged";
        QElapsedTimer timer;
        //timer.start();
        axisX->setMin(0.0);
        axisX->setMax(0.0);
        axisY->setMin(0.0);
        axisY->setMax(0.0);
        chart->removeAllSeries();
        foreach (QModelIndex ind, ui->fileTable->selectionModel()->selectedRows()) { //overkill, 0 or 1 element only
            if(ui->fileTable->item(ind.row(), ui->fileTable->columnCount() - 1)->text() == "0"){
                QString name = ui->fileTable->item(ind.row(), 0)->text();
                QThread* currThread = new QThread();
                Calculator* curr = new Calculator();
                calculators.insert(name, QPair<QThread*, Calculator*>(currThread, curr));
                //curr->setLoadingSmooth(loadingSmooth);
                curr->moveToThread(currThread);

                QObject::connect(curr, SIGNAL(dead()), currThread, SLOT(quit()), Qt::QueuedConnection);
                QObject::connect(currThread, SIGNAL(finished()), curr, SLOT(deleteLater()), Qt::QueuedConnection);
                QObject::connect(curr, SIGNAL(dead()), currThread, SLOT(deleteLater()), Qt::QueuedConnection);

                //QObject::connect(this, &MainWindow::setLoader, curr, &Calculator::setLoader, Qt::QueuedConnection);
                QObject::connect(curr, &Calculator::dataPointers, this, [=](QVector<QPointF>* edge, QVector<QPointF>* preEdge, QStringList names){
                    qDebug() << this->metaObject()->className() << "::seriesPointers";
                    curr->mutex.lock();
                    for(int i = 0; i < names.size(); ++i){
                        QtCharts::QLineSeries* tmp = new QtCharts::QLineSeries(chart);
                        tmp->setName(names.at(i));
                        tmp->setUseOpenGL(true);
                        chart->addSeries(tmp);
                        tmp->attachAxis(axisX);
                        tmp->attachAxis(axisY);
                        if(i == 0){
                            tmp->append(edge->toList());
                        }else if(i == 1){
                            tmp->append(preEdge->toList());
                        }
                    }
                    curr->mutex.unlock();
                    qDebug() << this->metaObject()->className() << "::seriesPointers::exit";
                }, Qt::QueuedConnection);

                QObject::connect(curr, &Calculator::range, this, [=](QPointF xRange, QPointF yRange){
                    qDebug() << this->metaObject()->className() << "::range" << xRange << yRange;
                    axisX->setMin(qMin(axisX->min(), xRange.x()*1.01));
                    axisX->setMax(qMax(axisX->max(), xRange.y()*1.01));
                    axisY->setMin(qMin(axisY->min(), yRange.x()*1.01));
                    axisY->setMax(qMax(axisY->max(), yRange.y()*1.01));
                    qDebug() << this->metaObject()->className() << "::range::exit";
                }, Qt::QueuedConnection);


                curr->setLoader(dataDir, name);

                if(name.endsWith(".PAR")){
                    loadPar(name);
                }
            }
        }
        qDebug() << this->metaObject()->className() << "::selectionChanged::exit";
    });
    QObject::connect(ui->datBox, &QCheckBox::toggled, this, [=](bool state){
        qDebug() << this->metaObject()->className() << "::datBox::toggled" << state;
        if(!state && !ui->parBox->isChecked()){
            ui->datBox->setChecked(true);
        }else{
            buildFileTable(dataDir);
        }
        qDebug() << this->metaObject()->className() << "::datBox::toggled::exit";
    });
    QObject::connect(ui->parBox, &QCheckBox::toggled, this, [=](bool state){
        qDebug() << this->metaObject()->className() << "::parBox::toggled" << state;
        if(!state && !ui->datBox->isChecked()){
            ui->parBox->setChecked(true);
        }else{
            buildFileTable(dataDir);
        }
        qDebug() << this->metaObject()->className() << "::parBox::toggled::exit";
    });
    loadSettings();
    ui->element1Box->addItems(elements);
    ui->element2Box->addItems(elements);
    QObject::connect(ui->acceptButton, &QPushButton::pressed, this, [=]{
        qDebug() << this->metaObject()->className() << "::acceptButton::pressed";
        bool exists = QFile(dataDir + "/" + ui->filenameEdit->text() + ".PAR").exists();
        if((exists && QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Warning!", "Override .par file?",
                                                      QMessageBox::Yes|QMessageBox::No).exec()) || !exists){
            writeParam(ui->filenameEdit->text());
            loadPar("default");
        }
        qDebug() << this->metaObject()->className() << "::acceptButton::pressed::exit";
    });
    QObject::connect(ui->discardButton, &QPushButton::pressed, this, [=]{
        qDebug() << this->metaObject()->className() << "::discardButton::pressed";
        loadPar(loaded);
        qDebug() << this->metaObject()->className() << "::discardButton::pressed::exit";
    });
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

MainWindow::~MainWindow()
{
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    saveSettings();
    delete settings;
    delete model;
    //refresh?
    //kill all threads
    chart->removeAllSeries();
    delete chart;
    delete chartView;
    delete ui;
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}


void MainWindow::loadSettings(){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    settings->beginGroup("gui");
        resize(settings->value("windowSize", QSize(800, 600)).toSize());
        move(settings->value("windowPosition", QPoint(0, 0)).toPoint());
        settings->beginGroup("horizontalLayout");
            settings->beginGroup("fileTable");
                settings->beginWriteArray("columns");
                    for(int i = 0; i < ui->fileTable->columnCount(); ++i){
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
        int elementsLength = settings->beginReadArray("elements");
        for(int i = 0; i < elementsLength; i++){
            settings->setArrayIndex(i);
            elements.append(settings->value("element", "unknown").toString());
        }
        settings->endArray();
    settings->endGroup();
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void MainWindow::saveSettings(){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    settings->remove("");
    settings->beginGroup("gui");
        settings->setValue("windowPosition", this->pos());
        settings->setValue("windowSize", this->size());
        settings->beginGroup("horizontalLayout");
            settings->beginGroup("fileTable");
                settings->beginWriteArray("columns");
                    for(int i = 0; i < ui->fileTable->columnCount(); i++){
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
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void MainWindow::buildFileTable(QString d){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << d;
    Q_UNUSED(d);
    dats.clear();
    params.clear();
    /*
    calcThread->quit();
        calcThread->requestInterruption();
        if(calcThread->wait()){
        }else{
            log("Error: thread exit error");
    }
    delete calcThread;
    */
    calculators.clear();//memory leak
    ui->fileTable->clear();
    ui->dataDirLabel->setText(dataDir);
    QModelIndex childIndex;
    int tableRow = 0;
    for(int i = 0; i < model->rowCount(model->index(dataDir)); i++){
        childIndex = model->index(i, 0, model->index(dataDir));
        if(model->fileName(childIndex).endsWith(".csv", Qt::CaseInsensitive)){
            dats.append(model->fileName(childIndex));
            if(ui->datBox->isChecked()){
                ui->fileTable->insertRow(tableRow);
                ui->fileTable->setItem(tableRow, 0, new QTableWidgetItem(model->fileName(childIndex)));
                ui->fileTable->setItem(tableRow, 5, new QTableWidgetItem(model->fileInfo(childIndex).lastModified().
                                                                          toString("dd.MM.yyyy   HH:mm")));
                ui->fileTable->setItem(tableRow, ui->fileTable->columnCount() - 1, new QTableWidgetItem("0"));
                tableRow++;
            }
        }else if(model->fileName(childIndex).endsWith(".PAR", Qt::CaseInsensitive)){
            QSettings par(dataDir + "/" + model->fileName(childIndex), QSettings::IniFormat);
            QString tmp;
            if(ui->parBox->isChecked()){
                ui->fileTable->insertRow(tableRow);
                ui->fileTable->setItem(tableRow, 0, new QTableWidgetItem(model->fileName(childIndex)));
                ui->fileTable->setItem(tableRow, 5, new QTableWidgetItem(model->fileInfo(childIndex).lastModified().
                                                                          toString("dd.MM.yyyy   HH:mm")));
                ui->fileTable->setItem(tableRow, ui->fileTable->columnCount() - 1, new QTableWidgetItem("0"));
                par.beginGroup("common");
                    tmp = par.value("sampleName", "unknown").toString();
                    if(tmp == "unknown"){
                        tmp = par.value("sample", "unknown").toString();
                    }
                    ui->fileTable->setItem(tableRow, 1, new QTableWidgetItem(tmp));
                    ui->fileTable->setItem(tableRow, 3, new QTableWidgetItem(par.value("angle", "unknown").toString()));
                    ui->fileTable->setItem(tableRow, 4, new QTableWidgetItem(par.value("rating", "unknown").toString()));
                    ui->fileTable->setItem(tableRow, 6, new QTableWidgetItem(par.value("comment", "").toString()));
                par.endGroup();
                par.beginGroup("file1");
                    tmp = par.value("element", "").toString();
                par.endGroup();
                if(tmp.length() > 0 && tmp != "Undefined"){
                    tmp = tmp.left(2);
                }else{
                    par.beginGroup("file2");
                        tmp = par.value("element", "--").toString().left(2);
                    par.endGroup();
                }
                if(tmp == "Un"){
                    tmp = "--";
                }
                ui->fileTable->setItem(tableRow, 2, new QTableWidgetItem(tmp));
                tableRow++;
            }
            par.beginGroup("file1");
                tmp = par.value("filename", "none").toString();
                if(tmp != "none"){
                    params.insertMulti(tmp, model->fileName(childIndex));
                }
            par.endGroup();
            par.beginGroup("file2");
                tmp = par.value("filename", "none").toString();
                if(tmp != "none"){
                    params.insertMulti(tmp, model->fileName(childIndex));
                }
            par.endGroup();
        }else{
        }
    }

    ui->fileTable->sortByColumn(0, Qt::AscendingOrder);//add sorting

    /*
    QString entry;
    foreach (entry, dats) {
        bool flag = false;
        if(params.value(entry) found){
            QString element;
            QString rating;
            QSettings par(dataDir + "/" + params.value(entry), QSettings::IniFormat);
            par.beginGroup("common");
                table->setData(table->index(matchingIndex.row(), 1, QModelIndex()), par.value("sample", "unknown").toString(), Qt::EditRole);
                table->setData(table->index(matchingIndex.row(), 3, QModelIndex()), par.value("angle", "unknown").toString(), Qt::EditRole);
                table->setData(table->index(matchingIndex.row(), 6, QModelIndex()), par.value("comment", "unknown").toString(), Qt::EditRole);
            par.endGroup();
            par.beginGroup("file1");
                if(par.value("filename").toString() == entry){
                    rating = par.value("rating", "unknown").toString();
      }else{
                    flag = true;
                }
                element = par.value("element", "").toString();
            par.endGroup();
            if(element.length() > 0 && element != "Undefined"){
                element = element.left(2);
            }else{
                par.beginGroup("file2");
                    element = par.value("element", "--").toString().left(2);
                par.endGroup();
            }
            if(element == "Un"){
                element = "--";
            }
            if(flag){
                par.beginGroup("file2");
                    rating = par.value("rating", "unknown").toString();
                par.endGroup();
            }
            delete(par);
            table->setData(table->index(matchingIndex.row(), 2, QModelIndex()), element, Qt::EditRole);
            table->setData(table->index(matchingIndex.row(), 4, QModelIndex()), rating, Qt::EditRole);
        }
    }
    */

    QDir dir(dataDir);
    QStringList filters;
    filters << "*.csv";
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);
    ui->file1Box->clear();
    ui->file2Box->clear();
    foreach(QString file, dir.entryList()){
        ui->file1Box->addItem(file);
        ui->file2Box->addItem(file);
    }
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void MainWindow::load(QString p){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << p;
    model->setRootPath(p);
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void MainWindow::writeParam(QString name){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << name;
    QSettings par(dataDir + "/" + name + ".PAR", QSettings::IniFormat);
    par.beginGroup("common");
        par.setValue("sampleName", ui->sampleEdit->text());
        par.setValue("chi", ui->angleSpinBox->value());
        par.setValue("amplitude", ui->ampEdit->text());
        par.setValue("rating", ui->ratingEdit->text());
        par.setValue("comment", ui->commentEdit->toPlainText());
        par.setValue("shift", ui->verticalFitSpinBox->value());
        par.setValue("smooth", ui->smoothSpinBox->value());
        //patch for SMs programm
        par.setValue("corrDerivative", 0);
        par.setValue("area", 1);
        par.setValue("thickness", 1);
    par.endGroup();
    par.beginGroup("file1");
        par.setValue("filename", ui->file1Box->currentText());
        par.setValue("element", ui->element1Box->currentText());
        par.setValue("energy", ui->energy1Edit->text());
        par.setValue("rating", ui->rating1Edit->text());
        par.setValue("sensetivity", ui->sens1Box->value());
    par.endGroup();
    par.beginGroup("file2");
        par.setValue("filename", ui->file2Box->currentText());
        par.setValue("element", ui->element2Box->currentText());
        par.setValue("energy", ui->energy2Edit->text());
        par.setValue("rating", ui->rating2Edit->text());
        par.setValue("sensetivity", ui->sens2Box->value());
    par.endGroup();
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void MainWindow::loadPar(QString name){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << name;
    QSettings par(dataDir + "/" + name, QSettings::IniFormat);
    ui->filenameEdit->setText(name.chopped(4));
    par.beginGroup("common");
        ui->sampleEdit->setText(par.value("sampleName", "Unknown").toString());
        if(ui->sampleEdit->text() == "Unknown"){
            ui->sampleEdit->setText(par.value("sample", "Unknown").toString());
        }
        ui->angleSpinBox->setValue(par.value("chi", 0.0).toDouble());
        if(ui->angleSpinBox->value() == 0.0){
            ui->angleSpinBox->setValue(par.value("angle", 0.0).toDouble());
        }
        ui->ampEdit->setText(par.value("amplitude", "Unknown").toString());
        ui->ratingEdit->setText(par.value("rating", "Unknown").toString());
        ui->commentEdit->setText(par.value("comment", "Unknown").toString());
        ui->verticalFitSpinBox->setValue(par.value("shift", 0.0).toDouble());
        ui->smoothSpinBox->setValue(par.value("smooth", 1).toInt());
    par.endGroup();
    par.beginGroup("file1");
        int index = ui->file1Box->findText(par.value("filename", "Unknown").toString());
        if(index != -1){
            ui->file1Box->setCurrentIndex(index);
        }
        index = ui->element1Box->findText(par.value("element", "Unknown").toString());
        if(index != -1){
            ui->element1Box->setCurrentIndex(index);
        }
        ui->energy1Edit->setText(par.value("energy", "Unknown").toString());
        ui->rating1Edit->setText(par.value("rating", "Unknown").toString());
        ui->sens1Box->setValue(par.value("sensetivity", "1").toInt());
    par.endGroup();
    par.beginGroup("file2");
        index = ui->file2Box->findText(par.value("filename", "Unknown").toString());
        if(index != -1){
            ui->file2Box->setCurrentIndex(index);
        }
        index = ui->element2Box->findText(par.value("element", "Unknown").toString());
        if(index != -1){
            ui->element2Box->setCurrentIndex(index);
        }
        ui->energy2Edit->setText(par.value("energy", "Unknown").toString());
        ui->rating2Edit->setText(par.value("rating", "Unknown").toString());
        ui->sens2Box->setValue(par.value("sensetivity", "1").toInt());
    par.endGroup();
    loaded = name;
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}
