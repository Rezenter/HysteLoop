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
    ui->fileTable->setColumnCount(labels.size());
    ui->fileTable->setHorizontalHeaderLabels(labels);
    ui->fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->fileTable->horizontalHeader()->setStretchLastSection(true);
    ui->signalType->setId(ui->rawSignalButton, 0);
    ui->signalType->setId(ui->zeroSignalButton, 1);
    ui->signalType->setId(ui->normSignalButton, 2);
    ui->fileGroup->setId(ui->edgeButton, 0);
    ui->fileGroup->setId(ui->preEdgeButton, 1);
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
            /*
            foreach (QPointF point, dynamic_cast<QtCharts::QLineSeries *>(chart->series().at(0))->points()) {
                stream << i++ << "\t" << 300.0 << "\t" <<point.x()*10 << "\t" << point.y() << "\r\n";
            }
            */
            file.close();
        }
        qDebug() << this->metaObject()->className() << "::exportButton::pressed::exit";
    });
    QObject::connect(model, &QFileSystemModel::directoryLoaded, this, &MainWindow::buildFileTable);
    QObject::connect(refresh, &QFileSystemWatcher::directoryChanged, this, &MainWindow::load);
    QObject::connect(ui->verticalFit1SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](qreal val){
        qDebug() << this->metaObject()->className() << "::verticalFitSpinBox::valueChanged" << val;
        Calc* curr = current();
        if(curr != nullptr){
            curr->calc->setVerticalOffset(0, val);
            curr->verticalFit[0] = val;
        }else{
            qDebug() << "wtf";
        }
        qDebug() << this->metaObject()->className() << "::verticalFitSpinBox::valueChanged::exit";
    });
    QObject::connect(ui->verticalFit2SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](qreal val){
        qDebug() << this->metaObject()->className() << "::verticalFitSpinBox::valueChanged" << val;
        Calc* curr = current();
        if(curr != nullptr){
            curr->calc->setVerticalOffset(1, val);
            curr->verticalFit[1] = val;
        }else{
            qDebug() << "wtf";
        }
        qDebug() << this->metaObject()->className() << "::verticalFitSpinBox::valueChanged::exit";
    });
    QObject::connect(ui->smoothDoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](qreal val){
        qDebug() << this->metaObject()->className() << "::smoothDoubleSpinBox::valueChanged" << val;
        Calc* curr = current();
        if(curr != nullptr){
            curr->calc->setGrain(val);
            curr->grain = val;
        }else{
            qDebug() << "wtf";
        }
        qDebug() << this->metaObject()->className() << "::smoothDoubleSpinBox::valueChanged::exit";
    });
    QObject::connect(ui->loadingSmoothSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val){
        qDebug() << this->metaObject()->className() << "::smoothDoubleSpinBox::valueChanged" << val;
        Calc* curr = current();
        if(curr != nullptr){
            curr->calc->setLoadingSmooth(val);
            curr->loadingSmooth = val;
        }else{
            qDebug() << "wtf";
        }
        qDebug() << this->metaObject()->className() << "::smoothDoubleSpinBox::valueChanged::exit";
    });
    QObject::connect(ui->signalType, QOverload<int, bool>::of(&QButtonGroup::buttonToggled), [=](int id, bool state){
        qDebug() << this->metaObject()->className() << "::signalType::buttonToggled" << id << state;
        if(state){
            Calc* curr = current();
            if(curr != nullptr){
                curr->calc->update(id, curr->files);
                curr->signal = id;
            }else{
                qDebug() << "wtf";
            }
        }
    });
    QObject::connect(ui->edgeButton, &QRadioButton::toggled, this, [=](bool state){
        qDebug() << this->metaObject()->className() << "::edgeButton::buttonToggled" << state;
        if(!state && !ui->preEdgeButton->isChecked()){
            ui->edgeButton->setChecked(true);
        }else{
            Calc* curr = current();
            qDebug() << curr->series[1]->isVisible();
            if(curr != nullptr){
                if(state){
                    if(ui->preEdgeButton->isChecked()){
                        curr->files = 2;
                    }else{
                        curr->files = 0;
                    }
                }else{
                    curr->files = 1;
                    qDebug() << curr->series[1]->isVisible();
                    curr->series[0]->setVisible(false);
                    qDebug() << curr->series[1]->isVisible();
                }
                qDebug() << curr->series[1]->isVisible();
                curr->calc->update(curr->signal, curr->files);
                qDebug() << curr->series[1]->isVisible();
            }else{
                qDebug() << "wtf";
            }
        }
        qDebug() << this->metaObject()->className() << "::edgeButton::buttonToggled::exit";
    });
    QObject::connect(ui->preEdgeButton, &QRadioButton::toggled, this, [=](bool state){
        qDebug() << this->metaObject()->className() << "::preEdgeButton::buttonToggled" << state;
        if(!state && !ui->edgeButton->isChecked()){
            ui->preEdgeButton->setChecked(true);
        }else{
            Calc* curr = current();
            if(curr != nullptr){
                if(state){
                    if(ui->edgeButton->isChecked()){
                        curr->files = 2;
                    }else{
                        curr->files = 1;
                    }
                }else{
                    curr->files = 0;
                    curr->series[1]->setVisible(false);
                }
                curr->calc->update(curr->signal, curr->files);
            }else{
                qDebug() << "wtf";
            }
        }
        qDebug() << this->metaObject()->className() << "::preEdgeButton::buttonToggled::exit";
    });
    QObject::connect(ui->fileTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]{
        qDebug() << this->metaObject()->className() << "::selectionChanged";
        foreach(Calc* calc, calculators){
            calc->series[0]->setVisible(false);
            calc->series[1]->setVisible(false);
        }
        QStringList names;
        if(ui->fileTable->selectionModel()->selectedRows().size() == 0){
            //make gui inactive
        }else{
            //activate gui
            foreach (QModelIndex ind, ui->fileTable->selectionModel()->selectedRows()) {
                QString name = ui->fileTable->item(ind.row(), 0)->text();
                names.append(name);
                if(!calculators.contains(name)){
                    Calc* calc = new Calc;
                    calculators.insert(name, calc);
                    calc->calc = new Calculator();
                    calc->thread = new QThread();
                    calc->calc->setLoadingSmooth(loadingSmooth);
                    calc->calc->moveToThread(calc->thread);
                    calc->name = name;
                    calc->xRange = QPointF(-1.0, 1.0);
                    calc->yRange = QPointF(-1.0, 1.0);
                    for(int i = 0; i < 2; ++i){
                        calc->series[i] = new QtCharts::QLineSeries(chart);
                        calc->series[i]->setUseOpenGL(true);
                        chart->addSeries(calc->series[i]);
                        calc->series[i]->attachAxis(axisX);
                        calc->series[i]->attachAxis(axisY);
                    }
                    QObject::connect(calc->calc, SIGNAL(dead()), calc->thread, SLOT(quit()), Qt::QueuedConnection);
                    QObject::connect(calc->thread, SIGNAL(finished()), calc->calc, SLOT(deleteLater()), Qt::QueuedConnection);
                    QObject::connect(calc->calc, SIGNAL(dead()), calc->thread, SLOT(deleteLater()), Qt::QueuedConnection);

                    QObject::connect(calc->calc, &Calculator::dataPointers, this,
                                     [=](QVector<QPointF>* first, QVector<QPointF>* second, QStringList names){
                        qDebug() << this->metaObject()->className() << "::dataPointers";
                        calc->calc->mutex.lock();
                        for(int i = 0; i < names.size(); ++i){
                            calc->series[i]->setName(names.at(i));
                            if(i == 0){
                                calc->data[i] = first;
                            }else if(i == 1){
                                calc->data[i] = second;
                            }
                        }
                        calc->calc->mutex.unlock();
                        calc->calc->update(0); //must be a function call, not a signal
                        qDebug() << this->metaObject()->className() << "::dataPointers::exit";
                    }, Qt::QueuedConnection);
                    QObject::connect(calc->calc, &Calculator::range, this, [=](QPointF xRange, QPointF yRange){
                        qDebug() << this->metaObject()->className() << "::range" << xRange << yRange;
                        calc->xRange = xRange;
                        calc->yRange = yRange;
                        resizeChart();
                        qDebug() << this->metaObject()->className() << "::range::exit";
                    }, Qt::QueuedConnection);
                    QObject::connect(calc->calc, &Calculator::ready, this, [=](int file){
                        qDebug() << this->metaObject()->className() << "::ready" << file;
                        {
                            const QSignalBlocker block(calc->series[file]);
                            Q_UNUSED(block);
                            calc->series[file]->clear();
                            calc->series[file]->append(calc->data[file]->toList());
                        }
                        calc->series[file]->pointsReplaced();
                        calc->series[file]->setVisible(true);
                        qDebug() << this->metaObject()->className() << "::ready::exit";
                    });
                    calc->calc->setLoader(dataDir, name);
                    if(name.endsWith(".PAR")){
                        loadPar(name);
                    }
                }else{
                    calculators.value(name)->calc->update();//to lazy to create a mapper
                }

            }
            resizeChart();
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
    //clear calculators
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
        loadingSmooth = settings->value("loadingSmooth", 5).toInt();
        grain = settings->value("grain", 0.5).toDouble();
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
        settings->setValue("loadingSmooth", loadingSmooth);
        settings->setValue("grain", grain);
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
        //par.setValue("shift", ui->verticalFitSpinBox->value());
        //par.setValue("smooth", ui->smoothSpinBox->value());
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
        //ui->verticalFitSpinBox->setValue(par.value("shift", 0.0).toDouble());
        //ui->smoothSpinBox->setValue(par.value("smooth", 1).toInt());
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

void MainWindow::resizeChart(){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    QPointF xRange = QPointF(0.0, 0.0);
    QPointF yRange = QPointF(0.0, 0.0);
    foreach(Calc* calc, calculators){
        if(calc->series[0]->isVisible()){
            xRange = QPointF(qMin(xRange.x(), calc->xRange.x()), qMax(xRange.y(), calc->xRange.y()));
            yRange = QPointF(qMin(yRange.x(), calc->yRange.x()), qMax(yRange.y(), calc->yRange.y()));
        }
    }
    axisX->setRange(xRange.x(), xRange.y());
    axisY->setRange(yRange.x(), yRange.y());
    xZeroSer->clear();
    xZeroSer->append(xRange.x(), 0.0);
    xZeroSer->append(xRange.y(), 0.0);
    yZeroSer->clear();
    yZeroSer->append(0.0, yRange.x());
    yZeroSer->append(0.0, yRange.y());
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}


MainWindow::Calc *MainWindow::current(){
    if(ui->fileTable->selectionModel()->selectedRows().size() > 0){
        QString name = ui->fileTable->item(ui->fileTable->selectionModel()->selectedRows().at(0).row(), 0)->text();
        if(calculators.contains(name)){
            return calculators.value(name);
        }else{
            qDebug() << "wtf, gui error 2";
        }
    }else{
        qDebug() << "wtf, gui error 1";
    }
    return nullptr;
}
