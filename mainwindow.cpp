#include "mainwindow.h"
#include "ui_mainwindow.h"


/*
 *To-do:
 *  base
 *      create common grid instead of finding zero

 *      separate loading and drawing functions
 *      calculate at loading
 *      move loading to thread
 *      check loading
 *          !!!! change vectors to lists !!!!
 *          change doubles to floats where possible, data wouldnt be lost due to low precision???wtf i meant
 *  save/load


 *  params dialog

 *      fill fields when file selected/changed
 *      param file in correlation with SM`s programm
 *      save dialog size
 *  multiple loop comparaison
 *      moovable legend
 *      mouse position
 *          subclass smth in order to modify mousemoveevent
 *      loading progress

 *  smoothing count spinBox with only odd numbers//forgot why.
 *  export dialog
 *      export in correlation with SM`s programm
 *      export name including more data
 *  load data from param file to dialog
 *  calculate coercitivity, amplitude and offset
 *  errors dialog

 *  smooth
 *  mb same sample filter?
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
    series = new QtCharts::QLineSeries;
    axisX = new QtCharts::QValueAxis;
    axisX->setMinorGridLineVisible(true);
    axisY = new QtCharts::QValueAxis();
    chartView = new QtCharts::QChartView(chart);
    ui->chartWidget->setLayout(new QGridLayout(ui->chartWidget));
    ui->chartWidget->layout()->addWidget(chartView);
    chartView->show();
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    axisX->setTickCount(11);
    axisX->setMinorTickCount(5);
    axisX->setTitleText("Magnetic flux density [mT]");
    axisX->setMax(300);
    axisX->setMin(-300);
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
    series->blockSignals(true);
    chart->addSeries(series);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    ui->fileTable->setModel(table);
    ui->fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->fileTable->horizontalHeader()->setStretchLastSection(true);
    ui->fileTable->setColumnHidden(6, true); //hide comment
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
    });
    QObject::connect(ui->exportButton, &QPushButton::pressed, this, [=]{
        qDebug() << this->metaObject()->className() << "::exportButton::pressed";
        QString output = QFileDialog::getSaveFileName(this, tr("Select export path"), dataDir, QString(), nullptr,
                                                      QFileDialog::DontConfirmOverwrite);
        if(output.length() != 0){
            QFile file(output + ".txt");
            file.open(QIODevice::WriteOnly);
            QTextStream stream(&file);
            foreach (QPointF point, series->points()) {
                stream << point.x() << "    " << point.y() << "\n";
            }
            file.close();
        }
    });
    QObject::connect(model, &QFileSystemModel::directoryLoaded, this, &MainWindow::buildFileTable);
    QObject::connect(refresh, &QFileSystemWatcher::directoryChanged, this, &MainWindow::load);
    QObject::connect(ui->fileTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]{
        qDebug() << this->metaObject()->className() << "::selectionChanged";
        foreach (QModelIndex ind, ui->fileTable->selectionModel()->selectedRows()) { //overkill, 0 or 1 element only
            QString name = table->data(table->index(ind.row(), 0, QModelIndex()), Qt::DisplayRole).toString();
            if(name.endsWith(".PAR")){
                loadPar(name);
            }
            FileLoader loader(dataDir, name);
            QPair<int, QVector<QPair<double, QPair<double, double>>>> signal = loader.getSignal();
            if(signal.first == -1){
                //sens dialog
                //signal.first = 50;
                //ask sensetivity for single files
            }
            QPair<int, QVector<QPair<double, QPair<double, double>>>> bgData = loader.getBackground();
            QPair<double, QPair<double, double>> point;
            QList<QPointF> dots;
            if(bgData.second.length() > 0){
                if(bgData.first == -1){
                    //sens dialog
                    //bgData.first = 10;
                }
                int ind = 0;
                foreach (point, bgData.second) {
                    dots.append(QPointF(point.first, (signal.second.at(ind).second.first*signal.first/point.second.second)/
                                        (point.second.first*bgData.first/point.second.second)));
                    ind++;
                }
            }else{
                foreach (point, signal.second) {
                    dots.append(QPointF(point.first, point.second.first*signal.first/point.second.second));
                }
            }
            int quater = dots.length()/4;
            double h = 0.5*(dots.at(quater).y() + dots.at(quater*3).y());
            int i = 0;
            foreach (QPointF dot, dots) {
                dots.replace(i, QPointF(dot.x(), dot.y()/h));
                i++;
            }
            h = 0.5*(dots.at(quater).y() + dots.at(quater*3).y());
            qreal yMax = std::numeric_limits<double>::min();
            series->blockSignals(true);
            series->clear();
            foreach (QPointF dot, dots) {
                qreal y = dot.y() - h;
                series->append(dot.x(), y);
                if(qAbs(y) > yMax){
                    yMax = qAbs(y);
                }
            }
            //axisX->setMax(xMax);
            //axisX->setMin(xMin);
            axisY->setMax(yMax);
            axisY->setMin(-yMax);
            series->blockSignals(false);
            series->setName(name);
            series->pointsReplaced();
        }
    });
    QObject::connect(ui->datBox, &QCheckBox::toggled, this, [=](bool state){
        if(!state && !ui->parBox->isChecked()){
            ui->datBox->setChecked(true);
        }else{
            buildFileTable(dataDir);
        }
    });
    QObject::connect(ui->parBox, &QCheckBox::toggled, this, [=](bool state){
        if(!state && !ui->datBox->isChecked()){
            ui->parBox->setChecked(true);
        }else{
            buildFileTable(dataDir);
        }
    });
    loadSettings();
    ui->element1Box->addItems(elements);
    ui->element2Box->addItems(elements);
    QObject::connect(ui->acceptButton, &QPushButton::pressed, this, [=]{
        bool exists = QFile(dataDir + "/" + ui->filenameEdit->text() + ".PAR").exists();
        if((exists && QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Warning!", "Override .par file?",
                                                      QMessageBox::Yes|QMessageBox::No).exec()) || !exists){
            writeParam(ui->filenameEdit->text());
            loadPar("default");
        }
    });
    QObject::connect(ui->discardButton, &QPushButton::pressed, this, [=]{
        loadPar(loaded);
    });
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete settings;
    delete model;
    chart->removeAllSeries();
    delete chart;
    delete chartView;
    delete ui;
}


void MainWindow::loadSettings(){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    settings->beginGroup("gui");
        resize(settings->value("windowSize", QSize(800, 600)).toSize());
        move(settings->value("windowPosition", QPoint(0, 0)).toPoint());
        settings->beginGroup("horizontalLayout");
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
        int elementsLength = settings->beginReadArray("elements");
        for(int i = 0; i < elementsLength; i++){
            settings->setArrayIndex(i);
            elements.append(settings->value("element", "unknown").toString());
        }
        settings->endArray();
    settings->endGroup();
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
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    Q_UNUSED(d);
    dats.clear();
    params.clear();
    table->removeAll();
    ui->dataDirLabel->setText(dataDir);
    QModelIndex childIndex;
    int tableRow = 0;
    for(int i = 0; i < model->rowCount(model->index(dataDir)); i++){
        childIndex = model->index(i, 0, model->index(dataDir));
        if(model->fileName(childIndex).endsWith(".csv", Qt::CaseInsensitive)){
            dats.append(model->fileName(childIndex));
            if(ui->datBox->isChecked()){
                table->insertRow(tableRow, QModelIndex());
                table->setData(table->index(tableRow, 0, QModelIndex()), model->fileName(childIndex), Qt::EditRole);
                table->setData(table->index(tableRow, 5, QModelIndex()),
                               model->fileInfo(childIndex).lastModified().toString("dd.MM.yyyy   HH:mm"), Qt::EditRole);
                tableRow++;
            }
        }else if(model->fileName(childIndex).endsWith(".PAR", Qt::CaseInsensitive)){
            QSettings *par = new QSettings(dataDir + "/" + model->fileName(childIndex), QSettings::IniFormat);
            QString tmp;
            if(ui->parBox->isChecked()){
                table->insertRow(tableRow, QModelIndex());
                table->setData(table->index(tableRow, 0, QModelIndex()), model->fileName(childIndex), Qt::EditRole);
                table->setData(table->index(tableRow, 5, QModelIndex()),
                               model->fileInfo(childIndex).lastModified().toString("dd.MM.yyyy   HH:mm"), Qt::EditRole);
                par->beginGroup("common");
                    table->setData(table->index(tableRow, 1, QModelIndex()), par->value("sample", "unknown").toString(), Qt::EditRole);
                    table->setData(table->index(tableRow, 4, QModelIndex()), par->value("rating", "unknown").toString(), Qt::EditRole);
                    table->setData(table->index(tableRow, 3, QModelIndex()), par->value("angle", "unknown").toString(), Qt::EditRole);
                    table->setData(table->index(tableRow, 6, QModelIndex()), par->value("comment", "unknown").toString(), Qt::EditRole);
                par->endGroup();
                par->beginGroup("file1");
                    tmp = par->value("element", "").toString();
                par->endGroup();
                if(tmp.length() > 0 && tmp != "Undefined"){
                    tmp = tmp.left(2);
                }else{
                    par->beginGroup("file2");
                        tmp = par->value("element", "--").toString().left(2);
                    par->endGroup();
                }
                if(tmp == "Un"){
                    tmp = "--";
                }
                table->setData(table->index(tableRow, 2, QModelIndex()), tmp, Qt::EditRole);
                tableRow++;
            }
            par->beginGroup("file1");
                tmp = par->value("filename", "none").toString();
                if(tmp != "none"){
                    params.insertMulti(tmp, model->fileName(childIndex));
                }
            par->endGroup();
            par->beginGroup("file2");
                tmp = par->value("filename", "none").toString();
                if(tmp != "none"){
                    params.insertMulti(tmp, model->fileName(childIndex));
                }
            par->endGroup();
            delete par;

        }else{
            //wtf, do nothing
        }
    }
    ui->fileTable->sortByColumn(0, Qt::AscendingOrder);
    proxy.setSourceModel(table);
    proxy.setFilterKeyColumn(0);
    QString entry;
    foreach (entry, dats) {
        bool flag = false;
        proxy.setFilterFixedString(entry);
        QModelIndex matchingIndex = proxy.mapToSource(proxy.index(0,0));
        if(matchingIndex.isValid()){
            QString element;
            QString rating;
            QSettings *par = new QSettings(dataDir + "/" + params.value(entry), QSettings::IniFormat);
            par->beginGroup("common");
                table->setData(table->index(matchingIndex.row(), 1, QModelIndex()), par->value("sample", "unknown").toString(), Qt::EditRole);
                table->setData(table->index(matchingIndex.row(), 3, QModelIndex()), par->value("angle", "unknown").toString(), Qt::EditRole);
                table->setData(table->index(matchingIndex.row(), 6, QModelIndex()), par->value("comment", "unknown").toString(), Qt::EditRole);
            par->endGroup();
            par->beginGroup("file1");
                if(par->value("filename").toString() == entry){
                    rating = par->value("rating", "unknown").toString();
                }else{
                    flag = true;
                }
                element = par->value("element", "").toString();
            par->endGroup();
            if(element.length() > 0 && element != "Undefined"){
                element = element.left(2);
            }else{
                par->beginGroup("file2");
                    element = par->value("element", "--").toString().left(2);
                par->endGroup();
            }
            if(element == "Un"){
                element = "--";
            }
            if(flag){
                par->beginGroup("file2");
                    rating = par->value("rating", "unknown").toString();
                par->endGroup();
            }
            delete(par);
            table->setData(table->index(matchingIndex.row(), 2, QModelIndex()), element, Qt::EditRole);
            table->setData(table->index(matchingIndex.row(), 4, QModelIndex()), rating, Qt::EditRole);
        }
    }
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
}

void MainWindow::load(QString p){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    model->setRootPath(p);
}

void MainWindow::writeParam(QString name){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    QSettings *par = new QSettings(dataDir + "/" + name + ".PAR", QSettings::IniFormat);
    par->beginGroup("common");
        par->setValue("sample", ui->sampleEdit->text());
        par->setValue("angle", ui->angleEdit->text());
        par->setValue("amplitude", ui->ampEdit->text());
        par->setValue("rating", ui->ratingEdit->text());
        par->setValue("comment", ui->commentEdit->toPlainText());
    par->endGroup();
    par->beginGroup("file1");
        par->setValue("filename", ui->file1Box->currentText());
        par->setValue("element", ui->element1Box->currentText());
        par->setValue("energy", ui->energy1Edit->text());
        par->setValue("rating", ui->rating1Edit->text());
        par->setValue("sensetivity", ui->sens1Box->value());
    par->endGroup();
    par->beginGroup("file2");
        par->setValue("filename", ui->file2Box->currentText());
        par->setValue("element", ui->element2Box->currentText());
        par->setValue("energy", ui->energy2Edit->text());
        par->setValue("rating", ui->rating2Edit->text());
        par->setValue("sensetivity", ui->sens2Box->value());
    par->endGroup();
    delete par;
}

void MainWindow::loadPar(QString name){
    QSettings par(dataDir + "/" + name, QSettings::IniFormat);
    ui->filenameEdit->setText(name.chopped(4));
    par.beginGroup("common");
        ui->sampleEdit->setText(par.value("sample", "Unknown").toString());
        ui->angleEdit->setText(par.value("angle", "Unknown").toString());
        ui->ampEdit->setText(par.value("amplitude", "Unknown").toString());
        ui->ratingEdit->setText(par.value("rating", "Unknown").toString());
        ui->commentEdit->setText(par.value("comment", "Unknown").toString());
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
        ui->sens1Box->setValue(par.value("sensetivity", "Unknown").toDouble());
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
        ui->sens2Box->setValue(par.value("sensetivity", "Unknown").toDouble());
    par.endGroup();
    loaded = name;
}
