#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QElapsedTimer>

/*
 *To-do:
 *
 * filter:
 *      Savitzky–Golay filter
 * store fitParams in .PAR
 *      store splits
 * add tooltips
 *
 * default export name//meh
 * highlight default values in param panel
 * mouse position => coordinates on the grid
 * calculate coercitivity, amplitude and offset
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
    splitSeries = new QtCharts::QLineSeries();
    chart->addSeries(splitSeries);
    splitSeries->attachAxis(axisX);
    splitSeries->attachAxis(axisY);
    splitSeries->setName("split");
    splitSeries->setVisible(false);
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
    ui->fileTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->fileTable->horizontalHeader()->setStretchLastSection(true);
    ui->signalType->setId(ui->rawSignalButton, 0);
    ui->signalType->setId(ui->zeroSignalButton, 1);
    ui->signalType->setId(ui->normSignalButton, 2);
    ui->fileGroup->setId(ui->edgeButton, 0);
    ui->fileGroup->setId(ui->preEdgeButton, 1);
    ui->fileGroup->setId(ui->bothButton, 2);
    QObject::connect(ui->fileTable->horizontalHeader(), &QHeaderView::sectionClicked, [=](int index){
        qDebug() << this->metaObject()->className() <<  ":: headerClicked::clicked" << index;
        if(sortingIndex == index){
            ascending = !ascending;
            if(ascending){
                ui->fileTable->sortByColumn(index, Qt::AscendingOrder);
            }else{
                ui->fileTable->sortByColumn(index, Qt::DescendingOrder);
            }
        }else{
            sortingIndex = index;
            ascending = true;
            ui->fileTable->sortByColumn(index, Qt::AscendingOrder);
        }
        qDebug() << this->metaObject()->className() <<  ":: headerClicked::clicked :: exit";
    });
    QObject::connect(ui->folderButton, &QPushButton::pressed, [=]{
        qDebug() << this->metaObject()->className() <<  ":: folderButton::pressed";
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select data directory"), dataDir, QFileDialog::DontUseNativeDialog);
        if(dir.length() != 0){
            dataDir = dir;
            load(dataDir);
            buildFileTable(dataDir);
        }
        qDebug() << this->metaObject()->className() <<  ":: folderButton::pressed :: exit";
    });
    QObject::connect(ui->exportButton, &QPushButton::pressed, [=]{
        qDebug() << this->metaObject()->className() << ":: exportButton::pressed";
        if(ui->fileTable->selectionModel()->selectedRows().size() > 0){
            QString output = QFileDialog::getExistingDirectory(this, tr("Select export path"), dataDir, QFileDialog::DontConfirmOverwrite);
            if(output.length() != 0){
                foreach (Calc* curr, calculators) {
                    if(curr->series[0]->isVisible() || curr->series[1]->isVisible()){
                        ui->progressBar->setValue(0);
                        QDir().mkdir(output + "/" + curr->name);
                        QObject::connect(this, &MainWindow::exportSeries, curr->calc, QOverload<QString>::of(&Calculator::exportSeries)
                                         , Qt::QueuedConnection);
                        QObject::connect(curr->calc, &Calculator::exportProgress, this->ui->progressBar,
                                         &QProgressBar::setValue, Qt::QueuedConnection);
                        emit exportSeries(output + "/" + curr->name);
                        QObject::disconnect(this, &MainWindow::exportSeries, 0, 0);
                    }
                }
            }
        }
        qDebug() << this->metaObject()->className() << ":: exportButton::pressed :: exit";
    });
    QObject::connect(model, &QFileSystemModel::directoryLoaded, this, &MainWindow::buildFileTable);
    QObject::connect(refresh, &QFileSystemWatcher::directoryChanged, this, &MainWindow::load);
    QObject::connect(ui->splinePowerSpinBox1, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val){
        qDebug() << this->metaObject()->className() << ":: splinePowerSpinBox1 :: valueChanged" << val;
        if(ui->splineLengthSpinBox1->value() <= val){
            ui->splinePowerSpinBox1->setValue(ui->splineLengthSpinBox1->value() - 1);
        }else{
            Calc* curr = current();
            if(curr != nullptr){
                curr->power[0] = val;
                QObject::connect(this, &MainWindow::setSplines, curr->calc, &Calculator::setSplines, Qt::QueuedConnection);
                emit setSplines(0, val, curr->frame[0]);
                QObject::disconnect(this, &MainWindow::setSplines, 0, 0);
            }else{
                qDebug() << "wtf";
            }
        }
        qDebug() << this->metaObject()->className() << ":: splinePowerSpinBox1 :: valueChanged" << val;
    });
    QObject::connect(ui->splinePowerSpinBox2, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val){
        qDebug() << this->metaObject()->className() << ":: splinePowerSpinBox2 :: valueChanged" << val;
        if(ui->splineLengthSpinBox2->value() <= val){
            ui->splinePowerSpinBox2->setValue(ui->splineLengthSpinBox2->value() - 1);
        }else{
            Calc* curr = current();
            if(curr != nullptr){
                curr->power[1] = val;
                QObject::connect(this, &MainWindow::setSplines, curr->calc, &Calculator::setSplines, Qt::QueuedConnection);
                emit setSplines(1, val, curr->frame[1]);
                QObject::disconnect(this, &MainWindow::setSplines, 0, 0);
            }else{
                qDebug() << "wtf";
            }
        }
        qDebug() << this->metaObject()->className() << ":: splinePowerSpinBox2 :: valueChanged" << val;
    });
    QObject::connect(ui->splineLengthSpinBox1, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val){
        qDebug() << this->metaObject()->className() << ":: splineLengthSpinBox1 :: valueChanged" << val;
        if(val % 2 == 0){
            ui->splineLengthSpinBox1->setValue(val + 1);
        }else{
            if(ui->splinePowerSpinBox1->value() >= val){
                ui->splineLengthSpinBox1->setValue(ui->splinePowerSpinBox1->value() + 1);
            }else{
                Calc* curr = current();
                if(curr != nullptr){
                    curr->frame[0] = val;
                    QObject::connect(this, &MainWindow::setSplines, curr->calc, &Calculator::setSplines, Qt::QueuedConnection);
                    emit setSplines(0, curr->power[0], val);
                    QObject::disconnect(this, &MainWindow::setSplines, 0, 0);
                }else{
                    qDebug() << "wtf";
                }
            }
        }
        qDebug() << this->metaObject()->className() << ":: splineLengthSpinBox1 :: valueChanged" << val;
    });
    QObject::connect(ui->splineLengthSpinBox2, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val){
        qDebug() << this->metaObject()->className() << ":: splineLengthSpinBox2 :: valueChanged" << val;
        if(val % 2 == 0){
            ui->splineLengthSpinBox2->setValue(val + 1);
        }else{
            if(ui->splinePowerSpinBox2->value() >= val){
                ui->splineLengthSpinBox2->setValue(ui->splinePowerSpinBox2->value() + 1);
            }else{
                Calc* curr = current();
                if(curr != nullptr){
                    curr->frame[1] = val;
                    QObject::connect(this, &MainWindow::setSplines, curr->calc, &Calculator::setSplines, Qt::QueuedConnection);
                    emit setSplines(1, curr->power[1], val);
                    QObject::disconnect(this, &MainWindow::setSplines, 0, 0);
                }else{
                    qDebug() << "wtf";
                }
            }
        }
        qDebug() << this->metaObject()->className() << ":: splineLengthSpinBox2 :: valueChanged" << val;
    });
    QObject::connect(ui->verticalFit1SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](qreal val){
        qDebug() << this->metaObject()->className() << ":: verticalFitSpinBox :: valueChanged" << val;
        Calc* curr = current();
        if(curr != nullptr){
            curr->verticalFit[0] = val;
            QObject::connect(this, &MainWindow::setVerticalOffset, curr->calc, &Calculator::setVerticalOffset, Qt::QueuedConnection);
            emit setVerticalOffset(0, val);
            QObject::disconnect(this, &MainWindow::setSplines, 0, 0);
        }else{
            qDebug() << "wtf";
        }
        qDebug() << this->metaObject()->className() << ":: verticalFitSpinBox :: valueChanged :: exit";
    });
    QObject::connect(ui->verticalFit2SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](qreal val){
        qDebug() << this->metaObject()->className() << ":: verticalFitSpinBox :: valueChanged" << val;
        Calc* curr = current();
        if(curr != nullptr){
            curr->verticalFit[1] = val;
            QObject::connect(this, &MainWindow::setVerticalOffset, curr->calc, &Calculator::setVerticalOffset, Qt::QueuedConnection);
            emit setVerticalOffset(1, val);
            QObject::disconnect(this, &MainWindow::setSplines, 0, 0);
        }else{
            qDebug() << "wtf";
        }
        qDebug() << this->metaObject()->className() << ":: verticalFitSpinBox :: valueChanged::exit";
    });
    QObject::connect(ui->smoothDoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](qreal val){
        qDebug() << this->metaObject()->className() << ":: smoothDoubleSpinBox :: valueChanged" << val;
        Calc* curr = current();
        if(curr != nullptr){
            curr->grain = val;
            QObject::connect(this, &MainWindow::setGrain, curr->calc, &Calculator::setGrain, Qt::QueuedConnection);
            emit setGrain(curr->grain);
            QObject::disconnect(this, &MainWindow::setGrain, 0, 0);
        }else{
            qDebug() << "wtf";
        }
        qDebug() << this->metaObject()->className() << ":: smoothDoubleSpinBox :: valueChanged::exit";
    });
    QObject::connect(ui->loadingSmoothSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val){
        qDebug() << this->metaObject()->className() << ":: smoothDoubleSpinBox :: valueChanged" << val;
        Calc* curr = current();
        if(curr != nullptr){
            curr->loadingSmooth = val;
            QObject::connect(this, &MainWindow::setLoadingSmooth, curr->calc, &Calculator::setLoadingSmooth, Qt::QueuedConnection);
            emit setLoadingSmooth(curr->loadingSmooth);
            QObject::disconnect(this, &MainWindow::setLoadingSmooth, 0, 0);
        }else{
            qDebug() << "wtf";
        }
        qDebug() << this->metaObject()->className() << ":: smoothDoubleSpinBox :: valueChanged::exit";
    });
    QObject::connect(ui->signalType, QOverload<int, bool>::of(&QButtonGroup::buttonToggled), [=](int id, bool state){
        qDebug() << this->metaObject()->className() << ":: signalType :: buttonToggled" << id << state;
        if(state){
            Calc* curr = current();
            if(curr != nullptr){
                curr->signal = id;
                emitUpdate(curr);
                if(id == 0){
                    ui->splitGroupBox->setEnabled(!ui->edgeButton->isChecked() || !ui->preEdgeButton->isChecked());
                }else{
                    ui->splitGroupBox->setEnabled(false);
                }
                if(id == 2){
                    curr->series[1]->setVisible(false);
                }
            }else{
                qDebug() << "wtf";
            }
            splitSeries->setVisible(false);
            ui->splitComboBox->setCurrentIndex(0);
            ui->zeroCheckBox->setEnabled(id == 2);
            ui->der1DoubleSpinBox->setEnabled(id == 0 && ui->edgeButton->isChecked());
            ui->der2DoubleSpinBox->setEnabled(id == 0 && ui->preEdgeButton->isChecked());
            ui->verticalFit1SpinBox->setEnabled(id == 0 && ui->edgeButton->isChecked());
            ui->verticalFit2SpinBox->setEnabled(id == 0 && ui->preEdgeButton->isChecked());
            ui->splineLengthSpinBox1->setEnabled(id == 0 && ui->edgeButton->isChecked());
            ui->splineLengthSpinBox2->setEnabled(id == 0 && ui->preEdgeButton->isChecked());
            ui->splinePowerSpinBox1->setEnabled(id == 0 && ui->edgeButton->isChecked());
            ui->splinePowerSpinBox2->setEnabled(id == 0 && ui->preEdgeButton->isChecked());
            ui->edgeButton->setEnabled(id != 2);
            ui->preEdgeButton->setEnabled(id != 2 && curr->name.endsWith(".par", Qt::CaseInsensitive));
            ui->bothButton->setEnabled(id != 2 && curr->name.endsWith(".par", Qt::CaseInsensitive));
        }
        qDebug() << this->metaObject()->className() << ":: signalType :: buttonToggled exit";
    });
    QObject::connect(ui->fileGroup, QOverload<int, bool>::of(&QButtonGroup::buttonToggled), [=](int id, bool state){
        qDebug() << this->metaObject()->className() << ":: fileGroup :: buttonToggled" << id << state;
        Calc* curr = current();
        bool checked = state && ui->rawSignalButton->isChecked();
        switch (id) {
        case 0:
            ui->verticalFit1SpinBox->setEnabled(checked);
            ui->splineLengthSpinBox1->setEnabled(checked);
            ui->splinePowerSpinBox1->setEnabled(checked);
            ui->der1DoubleSpinBox->setEnabled(checked);
            updateSplits(0, -1);
            break;
        case 1:
            ui->verticalFit2SpinBox->setEnabled(checked);
            ui->splineLengthSpinBox2->setEnabled(checked);
            ui->splinePowerSpinBox2->setEnabled(checked);
            ui->der2DoubleSpinBox->setEnabled(checked);
            updateSplits(1, -1);
            break;
        case 2:
            //
            break;
        default:
            qDebug() << "wtf?";
            break;
        }
        if(curr != nullptr && state){
            curr->files = id;
            curr->series[0]->setVisible(id != 1);
            curr->series[1]->setVisible(id != 0);
            emitUpdate(curr);
        }
        ui->splitGroupBox->setEnabled(curr->files != 2 && ui->rawSignalButton->isChecked());
        qDebug() << this->metaObject()->className() << ":: fileGroup :: buttonToggled exit";
    });
    QObject::connect(ui->zeroCheckBox, &QCheckBox::toggled, [=](bool state){
        qDebug() << this->metaObject()->className() << ":: zeroCheckBox :: toggled" << state;
        Calc* curr = current();
        if(curr->zeroNorm != state){
            curr->zeroNorm = state;
            emitUpdate(curr);
        }
        qDebug() << this->metaObject()->className() << ":: zeroCheckBox :: toggled exit";
    });
    QObject::connect(ui->der1DoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double val){
        qDebug() << this->metaObject()->className() << ":: der1SpinBox :: valueChanged" << val;
        Calc* curr = current();
        if(curr->criticalDer[0] != val){
            curr->criticalDer[0] = val;
            QObject::connect(this, &MainWindow::setCriticalDer, curr->calc, &Calculator::setCriticalDer, Qt::QueuedConnection);
            emit setCriticalDer(0, val);
            QObject::disconnect(this, &MainWindow::setCriticalDer, 0, 0);
        }
        qDebug() << this->metaObject()->className() << ":: der1SpinBox :: valueChanged exit";
    });
    QObject::connect(ui->der2DoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double val){
        qDebug() << this->metaObject()->className() << ":: der2SpinBox :: valueChanged" << val;
        Calc* curr = current();
        if(curr->criticalDer[1] != val){
            curr->criticalDer[1] = val;
            QObject::connect(this, &MainWindow::setCriticalDer, curr->calc, &Calculator::setCriticalDer, Qt::QueuedConnection);
            emit setCriticalDer(1, val);
            QObject::disconnect(this, &MainWindow::setCriticalDer, 0, 0);
        }
        qDebug() << this->metaObject()->className() << ":: der2SpinBox :: valueChanged exit";
    });
    QObject::connect(ui->splitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){
        qDebug() << this->metaObject()->className() << ":: splitComboBox :: indexChanged" << index;
        splitSeries->setVisible(false);
        splitSeries->clear();
        if(index != -1 && ui->splitComboBox->currentText().size() > 0){//overkill?
            Calc* curr = current();
            int file = 0;
            if(ui->preEdgeButton->isChecked()){
                file = 1;
            }
            int rise = 1;
            if(ui->splitComboBox->currentData().toInt() < 0){
                rise = 0;
            }
            int index = qAbs(ui->splitComboBox->currentData().toInt()) - 1;
            ui->splitLeftSpinBox->setValue(curr->calc->splits[file][rise]->at(index)->l);
            ui->splitRightSpinBox->setValue(curr->calc->splits[file][rise]->at(index)->r);
            ui->splitShiftDoubleSpinBox->setValue(curr->calc->splits[file][rise]->at(index)->shift);
            if(curr->calc->splits[file][rise]->at(index)->original->size() > 0){
                for(int i = 0; i < curr->calc->splits[file][rise]->at(index)->original->size(); ++i){
                    splitSeries->append(curr->calc->splits[file][rise]->at(index)->original->at(i).x(),
                                        curr->calc->splits[file][rise]->at(index)->original->at(i).y() - curr->calc->offsets[file]);
                }
                splitSeries->setVisible(true);
                emitUpdate(curr);
            }
        }
        qDebug() << this->metaObject()->className() << ":: splitComboBox :: indexChanged::exit";
    });
    QObject::connect(ui->splitLeftSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val){
        qDebug() << this->metaObject()->className() << ":: splitLeftSpinBox :: valueChanged" << val;
        if(ui->splitComboBox->currentIndex() != -1 && ui->splitComboBox->currentText().size() > 0){
            Calc* curr = current();
            int file = 0;
            if(ui->preEdgeButton->isChecked()){
                file = 1;
            }
            int rise = 1;
            if(ui->splitComboBox->currentData().toInt() < 0){
                rise = 0;
            }
            int index = qAbs(ui->splitComboBox->currentData().toInt()) - 1;
            if(curr->calc->splits[file][rise]->at(index)->l != val){
                curr->calc->mutex->lock();
                curr->calc->splits[file][rise]->at(index)->l = val;
                curr->calc->mutex->unlock();
                QObject::connect(this, &MainWindow::updateSplit, curr->calc, &Calculator::updateSplit, Qt::QueuedConnection);
                emit updateSplit(file, rise, index);
                QObject::disconnect(this, &MainWindow::updateSplit, 0, 0);
            }
        }
        qDebug() << this->metaObject()->className() << ":: splitLeftSpinBox :: valueChanged::exit";
    });
    QObject::connect(ui->splitRightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val){
        qDebug() << this->metaObject()->className() << ":: splitRightSpinBox :: valueChanged" << val;
        if(ui->splitComboBox->currentIndex() != -1 && ui->splitComboBox->currentText().size() > 0){
            Calc* curr = current();
            int file = 0;
            if(ui->preEdgeButton->isChecked()){
                file = 1;
            }
            int rise = 1;
            if(ui->splitComboBox->currentData().toInt() < 0){
                rise = 0;
            }
            int index = qAbs(ui->splitComboBox->currentData().toInt()) - 1;
            if(curr->calc->splits[file][rise]->at(index)->r != val){
                curr->calc->mutex->lock();
                curr->calc->splits[file][rise]->at(index)->r = val;
                curr->calc->mutex->unlock();
                QObject::connect(this, &MainWindow::updateSplit, curr->calc, &Calculator::updateSplit, Qt::QueuedConnection);
                emit updateSplit(file, rise, index);
                QObject::disconnect(this, &MainWindow::updateSplit, 0, 0);
            }
        }
        qDebug() << this->metaObject()->className() << ":: splitRightSpinBox :: valueChanged::exit";
    });
    QObject::connect(ui->splitShiftDoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double val){
        qDebug() << this->metaObject()->className() << ":: splitShiftDoubleSpinBox :: valueChanged" << val;
        if(ui->splitComboBox->currentIndex() != -1 && ui->splitComboBox->currentText().size() > 0){
            Calc* curr = current();
            int file = 0;
            if(ui->preEdgeButton->isChecked()){
                file = 1;
            }
            int rise = 1;
            if(ui->splitComboBox->currentData().toInt() < 0){
                rise = 0;
            }
            int index = qAbs(ui->splitComboBox->currentData().toInt()) - 1;
            if(curr->calc->splits[file][rise]->at(index)->shift != val){
                curr->calc->mutex->lock();
                curr->calc->splits[file][rise]->at(index)->shift = val;
                curr->calc->mutex->unlock();
                QObject::connect(this, &MainWindow::updateSplit, curr->calc, &Calculator::updateSplit, Qt::QueuedConnection);
                emit updateSplit(file, rise, index);
                QObject::disconnect(this, &MainWindow::updateSplit, 0, 0);
            }
        }
        qDebug() << this->metaObject()->className() << ":: splitShiftDoubleSpinBox :: valueChanged::exit";
    });
    QObject::connect(ui->fileTable->selectionModel(), &QItemSelectionModel::selectionChanged, [=]{
        qDebug() << this->metaObject()->className() << ":: selectionChanged";
        if(!fileTableBusy){
            foreach(Calc* calc, calculators){
                calc->series[0]->setVisible(false);
                calc->series[1]->setVisible(false);
            }
            if(ui->fileTable->selectionModel()->selectedRows().size() == 0){
                ui->splitGroupBox->setEnabled(false);
                ui->rawSignalButton->setEnabled(false);
                ui->zeroSignalButton->setEnabled(false);
                ui->normSignalButton->setEnabled(false);
                ui->zeroCheckBox->setEnabled(false);
                ui->der1DoubleSpinBox->setEnabled(false);
                ui->der2DoubleSpinBox->setEnabled(false);
                ui->verticalFit1SpinBox->setEnabled(false);
                ui->verticalFit2SpinBox->setEnabled(false);
                ui->splineLengthSpinBox1->setEnabled(false);
                ui->splineLengthSpinBox2->setEnabled(false);
                ui->splinePowerSpinBox1->setEnabled(false);
                ui->splinePowerSpinBox2->setEnabled(false);
                ui->smoothDoubleSpinBox->setEnabled(false);
                ui->loadingSmoothSpinBox->setEnabled(false);
            }else{
                ui->splitGroupBox->setEnabled(true);
                ui->rawSignalButton->setEnabled(true);
                ui->zeroSignalButton->setEnabled(true);
                ui->der1DoubleSpinBox->setEnabled(ui->edgeButton->isChecked());
                ui->der2DoubleSpinBox->setEnabled(ui->preEdgeButton->isChecked());
                ui->verticalFit1SpinBox->setEnabled(true);
                ui->splineLengthSpinBox1->setEnabled(true);
                ui->splinePowerSpinBox1->setEnabled(true);
                ui->smoothDoubleSpinBox->setEnabled(true);
                ui->loadingSmoothSpinBox->setEnabled(true);
                Calc* calc;
                foreach (QModelIndex ind, ui->fileTable->selectionModel()->selectedRows()) {
                    ui->splitComboBox->clear();
                    QString name = ui->fileTable->item(ind.row(), 0)->text();
                    if(!calculators.contains(name)){
                        calc = new Calc;
                        calculators.insert(name, calc);
                        calc->calc = new Calculator(name);
                        calc->thread = new QThread();
                        calc->calc->setLoadingSmooth(loadingSmooth);
                        calc->loadingSmooth = loadingSmooth;
                        calc->calc->moveToThread(calc->thread);
                        calc->name = name;
                        calc->xRange = QPointF(-1.0, 1.0);
                        calc->yRange = QPointF(-1.0, 1.0);
                        calc->files = 0;
                        calc->zeroNorm = false;
                        calc->criticalDer = criticalDer;
                        calc->grain = grain;
                        calc->signal = 0;
                        calc->verticalFit[0] = verticalFit[0];
                        calc->verticalFit[1] = verticalFit[1];
                        for(int i = 0; i < 2; ++i){
                            calc->series[i] = new QtCharts::QLineSeries(chart);
                            calc->series[i]->setUseOpenGL(true);
                            chart->addSeries(calc->series[i]);
                            calc->series[i]->attachAxis(axisX);
                            calc->series[i]->attachAxis(axisY);
                            calc->series[i]->setVisible(false);
                        }
                        QObject::connect(calc->calc, SIGNAL(dead()), calc->thread, SLOT(quit()), Qt::QueuedConnection);
                        QObject::connect(calc->thread, SIGNAL(finished()), calc->calc, SLOT(deleteLater()), Qt::QueuedConnection);
                        QObject::connect(calc->calc, SIGNAL(dead()), calc->thread, SLOT(deleteLater()), Qt::QueuedConnection);
                        calc->thread->start();
                        QObject::connect(calc->calc, &Calculator::dataPointers, this,
                                         [=](QVector<QPointF>* first, QVector<QPointF>* second, QStringList names){
                            qDebug() << this->metaObject()->className() << ":: dataPointers";
                            calc->calc->mutex->lock();
                            for(int i = 0; i < names.size(); ++i){
                                calc->series[i]->setName(names.at(i));
                                if(i == 0){
                                    calc->data[i] = first;
                                }else if(i == 1){
                                    calc->data[i] = second;
                                }
                            }
                            calc->calc->mutex->unlock();
                            emitUpdate(calc);
                            qDebug() << this->metaObject()->className() << ":: dataPointers::exit";
                        }, Qt::QueuedConnection);
                        QObject::connect(calc->calc, &Calculator::range, this, [=](QPointF xRange, QPointF yRange){
                            qDebug() << this->metaObject()->className() << ":: range" << xRange << yRange;
                            calc->xRange = xRange;
                            calc->yRange = yRange;
                            resizeChart();
                            qDebug() << this->metaObject()->className() << ":: range::exit";
                        }, Qt::QueuedConnection);
                        QObject::connect(calc->calc, &Calculator::ready, this, [=](int file){
                            qDebug() << this->metaObject()->className() << ":: ready" << file;
                            {
                                const QSignalBlocker block(calc->series[file]);
                                Q_UNUSED(block);
                                calc->series[file]->clear();
                                calc->series[file]->append(calc->data[file]->toList());
                            }
                            calc->series[file]->pointsReplaced();
                            calc->series[file]->setVisible(true);
                            resizeChart();
                            qDebug() << this->metaObject()->className() << ":: ready::exit";
                        }, Qt::QueuedConnection);
                        QObject::connect(calc->calc, &Calculator::updateSplits, this, [=](const int file, const int index){
                            qDebug() << this->metaObject()->className() << ":: updateSplitsLambda" << file << index;
                            if(file == 0 || ui->preEdgeButton->isChecked()){
                                updateSplits(file, index);
                            }
                            qDebug() << this->metaObject()->className() << ":: updateSplitsLambda::exit";
                        }, Qt::QueuedConnection);

                        calc->calc->setLoader(dataDir, name);
                        if(name.endsWith(".PAR")){
                            loadPar(name);
                        }
                    }else{
                        emitUpdate(calculators.value(name));
                    }
                }
                calc = current();
                if(calc != nullptr){
                    bool isPar = calc->name.endsWith(".par", Qt::CaseInsensitive);
                    ui->normSignalButton->setEnabled(isPar);
                    ui->edgeButton->setEnabled(isPar);
                    ui->preEdgeButton->setEnabled(isPar);
                    ui->bothButton->setEnabled(isPar);
                    ui->loadingSmoothSpinBox->setValue(calc->loadingSmooth);
                    ui->smoothDoubleSpinBox->setValue(calc->grain);
                    ui->der1DoubleSpinBox->setValue(calc->criticalDer[0]);
                    switch (calc->signal) {
                    case 0:
                        ui->rawSignalButton->setChecked(true);
                        break;
                    case 1:
                        ui->zeroSignalButton->setChecked(true);
                        break;
                    case 2:
                        ui->normSignalButton->setChecked(true);
                        break;
                    default:
                        ui->zeroSignalButton->setChecked(true);
                        ui->rawSignalButton->setChecked(true);
                        qDebug() << "signal = wtf?";
                        break;
                    }
                    switch (calc->files) {
                    case 0:
                        ui->edgeButton->setChecked(true);
                        updateSplits(0, -1);
                        break;
                    case 1:
                        ui->preEdgeButton->setChecked(true);
                        updateSplits(1, -1);
                        break;
                    case 2:
                        ui->bothButton->setChecked(true);
                        break;
                    default:
                        ui->edgeButton->setChecked(true);
                        qDebug() << "files = wtf?";
                        break;
                    }
                    ui->zeroCheckBox->setEnabled(isPar && ui->normSignalButton->isChecked());
                    ui->zeroCheckBox->setChecked(calc->zeroNorm);
                    ui->verticalFit1SpinBox->setValue(calc->verticalFit[0]);
                    ui->splineLengthSpinBox1->setValue(calc->frame[0]);
                    ui->splinePowerSpinBox1->setValue(calc->power[0]);
                    if(isPar){
                        ui->verticalFit2SpinBox->setValue(calc->verticalFit[1]);
                        ui->splineLengthSpinBox2->setValue(calc->frame[1]);
                        ui->splinePowerSpinBox2->setValue(calc->power[1]);
                        ui->der2DoubleSpinBox->setValue(calc->criticalDer[1]);
                    }
                }
                resizeChart();
            }
        }
        qDebug() << this->metaObject()->className() << ":: selectionChanged::exit";
    });
    QObject::connect(ui->datBox, &QCheckBox::toggled, [=](bool state){
        qDebug() << this->metaObject()->className() << ":: datBox :: toggled" << state;
        if(!state && !ui->parBox->isChecked()){
            ui->datBox->setChecked(true);
        }else{
            buildFileTable(dataDir);
        }
        qDebug() << this->metaObject()->className() << ":: datBox :: toggled::exit";
    });
    QObject::connect(ui->parBox, &QCheckBox::toggled, [=](bool state){
        qDebug() << this->metaObject()->className() << ":: parBox :: toggled" << state;
        if(!state && !ui->datBox->isChecked()){
            ui->parBox->setChecked(true);
        }else{
            buildFileTable(dataDir);
        }
        qDebug() << this->metaObject()->className() << ":: parBox :: toggled::exit";
    });
    loadSettings();
    ui->element1Box->addItems(elements);
    ui->element2Box->addItems(elements);
    QObject::connect(ui->acceptButton, &QPushButton::pressed, [=]{
        qDebug() << this->metaObject()->className() << ":: acceptButton::pressed";
        bool exists = QFile(dataDir + "/" + ui->filenameEdit->text() + ".PAR").exists();
        if((exists && QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Warning!", "Override .par file?",
                                                      QMessageBox::Yes|QMessageBox::No).exec()) || !exists){
            writeParam(ui->filenameEdit->text());
            loadPar("default");
        }
        qDebug() << this->metaObject()->className() << ":: acceptButton::pressed::exit";
    });
    QObject::connect(ui->discardButton, &QPushButton::pressed, [=]{
        qDebug() << this->metaObject()->className() << ":: discardButton::pressed";
        loadPar(loaded);
        qDebug() << this->metaObject()->className() << ":: discardButton::pressed::exit";
    });
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

MainWindow::~MainWindow()
{
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__;
    saveSettings();
    delete settings;
    delete model;
    delete refresh;
    dats.clear();
    params.clear();
    foreach (Calc* curr, calculators) {
        for(int file = 0; file < 2; ++file){
            chart->removeSeries(curr->series[file]);
            delete curr->series[file];
        }
        delete curr->calc;
        curr->thread->quit();
        curr->thread->requestInterruption();
        if(!curr->thread->wait()){
            qDebug() << "thread exit error";
        }
        delete curr->thread;
        delete curr;
    }
    calculators.clear();
    while(ui->fileTable->rowCount() != 0){
        for(int i = 0; i < ui->fileTable->columnCount(); ++i){
            if(ui->fileTable->item(0, i) != nullptr){
                delete (ui->fileTable->item(0, i));
            }
        }
        ui->fileTable->removeRow(0);
    }
    chart->removeAllSeries();
    delete chart;
    delete axisX;
    delete axisY;
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
        criticalDer[0] = settings->value("criticalDer0", 0.015).toDouble();
        criticalDer[1] = settings->value("criticalDer1", 0.015).toDouble();
        verticalFit[0] = settings->value("verticalFit0", 0.0).toDouble();
        verticalFit[1] = settings->value("verticalFit1", 0.0).toDouble();
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
        settings->setValue("criticalDer0", criticalDer[0]);
        settings->setValue("criticalDer1", criticalDer[1]);
        settings->setValue("verticalFit0", verticalFit[0]);
        settings->setValue("verticalFit1", verticalFit[1]);
    settings->endGroup();
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void MainWindow::buildFileTable(QString d){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << d;
    Q_UNUSED(d);
    fileTableBusy = true;
    dats.clear();
    params.clear();
    foreach (Calc* curr, calculators) {
        for(int file = 0; file < 2; ++file){
            chart->removeSeries(curr->series[file]);
            delete curr->series[file];
        }
        delete curr->calc;
        curr->thread->quit();
        curr->thread->requestInterruption();
        if(!curr->thread->wait()){
            qDebug() << "thread exit error";
        }
        delete curr->thread;
        delete curr;
    }
    calculators.clear();
    while(ui->fileTable->rowCount() != 0){
        for(int i = 0; i < ui->fileTable->columnCount(); ++i){
            if(ui->fileTable->item(0, i) != nullptr){
                delete (ui->fileTable->item(0, i));
            }
        }
        ui->fileTable->removeRow(0);
    }
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
                ui->fileTable->item(tableRow, 0)->setFlags(ui->fileTable->item(tableRow, 0)->flags() &  ~Qt::ItemIsEditable);
                ui->fileTable->setItem(tableRow, 5, new QTableWidgetItem(model->fileInfo(childIndex).lastModified().
                                                                          toString("dd.MM.yyyy   HH:mm")));
                ui->fileTable->item(tableRow, 5)->setFlags(ui->fileTable->item(tableRow, 5)->flags() &  ~Qt::ItemIsEditable);
                tableRow++;
            }
        }else if(model->fileName(childIndex).endsWith(".PAR", Qt::CaseInsensitive)){
            QSettings par(dataDir + "/" + model->fileName(childIndex), QSettings::IniFormat);
            QString tmp;
            if(ui->parBox->isChecked()){
                ui->fileTable->insertRow(tableRow);
                ui->fileTable->setItem(tableRow, 0, new QTableWidgetItem(model->fileName(childIndex)));
                ui->fileTable->item(tableRow, 0)->setFlags(ui->fileTable->item(tableRow, 0)->flags() &  ~Qt::ItemIsEditable);
                ui->fileTable->setItem(tableRow, 5, new QTableWidgetItem(model->fileInfo(childIndex).lastModified().
                                                                          toString("dd.MM.yyyy   HH:mm")));
                ui->fileTable->item(tableRow, 5)->setFlags(ui->fileTable->item(tableRow, 5)->flags() &  ~Qt::ItemIsEditable);
                par.beginGroup("common");
                    tmp = par.value("sampleName", "unknown").toString();
                    if(tmp == "unknown"){
                        tmp = par.value("sample", "unknown").toString();
                    }
                    ui->fileTable->setItem(tableRow, 1, new QTableWidgetItem(tmp));
                    ui->fileTable->item(tableRow, 1)->setFlags(ui->fileTable->item(tableRow, 1)->flags() &  ~Qt::ItemIsEditable);
                    ui->fileTable->setItem(tableRow, 3, new QTableWidgetItem(par.value("angle", "unknown").toString()));
                    ui->fileTable->item(tableRow, 3)->setFlags(ui->fileTable->item(tableRow, 3)->flags() &  ~Qt::ItemIsEditable);
                    ui->fileTable->setItem(tableRow, 4, new QTableWidgetItem(par.value("rating", "unknown").toString()));
                    ui->fileTable->item(tableRow, 4)->setFlags(ui->fileTable->item(tableRow, 4)->flags() &  ~Qt::ItemIsEditable);
                    ui->fileTable->setItem(tableRow, 6, new QTableWidgetItem(par.value("comment", "").toString()));
                    ui->fileTable->item(tableRow, 6)->setFlags(ui->fileTable->item(tableRow, 6)->flags() &  ~Qt::ItemIsEditable);
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
                ui->fileTable->item(tableRow, 2)->setFlags(ui->fileTable->item(tableRow, 2)->flags() &  ~Qt::ItemIsEditable);
                tableRow++;
            }
            par.beginGroup("file1");
                tmp = par.value("filename", "none").toString();
                if(tmp != "none"){
                    params.insert(tmp, model->fileName(childIndex));
                }
            par.endGroup();
            par.beginGroup("file2");
                tmp = par.value("filename", "none").toString();
                if(tmp != "none"){
                    params.insert(tmp, model->fileName(childIndex));
                }
            par.endGroup();
        }else{
        }
    }
    ui->fileTable->sortByColumn(0, Qt::AscendingOrder);
    sortingIndex = 0;
    ascending = true;
    QString entry;
    foreach (entry, dats) {
        bool flag = false;
        if(params.contains(entry)){
            int row = 0;
            for(int i = 0; i < ui->fileTable->rowCount(); ++i){
                if(ui->fileTable->item(i, 0)->text() == entry){
                    row = i;
                }
            }
            QString element;
            QString rating;
            QSettings par(dataDir + "/" + params.value(entry), QSettings::IniFormat);
            par.beginGroup("common");
                ui->fileTable->setItem(row, 1, new QTableWidgetItem(par.value("sample", "-").toString()));
                ui->fileTable->item(row, 1)->setFlags(ui->fileTable->item(row, 1)->flags() &  ~Qt::ItemIsEditable);
                ui->fileTable->setItem(row, 3, new QTableWidgetItem(par.value("angle", "-").toString()));
                ui->fileTable->item(row, 3)->setFlags(ui->fileTable->item(row, 3)->flags() &  ~Qt::ItemIsEditable);
                ui->fileTable->setItem(row, 6, new QTableWidgetItem(par.value("comment", "-").toString()));
                ui->fileTable->item(row, 6)->setFlags(ui->fileTable->item(row, 6)->flags() &  ~Qt::ItemIsEditable);
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
            ui->fileTable->setItem(row, 2, new QTableWidgetItem(element));
            ui->fileTable->item(row, 2)->setFlags(ui->fileTable->item(row, 2)->flags() &  ~Qt::ItemIsEditable);
            ui->fileTable->setItem(row, 4, new QTableWidgetItem(rating));
            ui->fileTable->item(row, 4)->setFlags(ui->fileTable->item(row, 4)->flags() &  ~Qt::ItemIsEditable);
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
    fileTableBusy = false;
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
        //----------------------------------------------------------------------------------------------
    Calc* curr = current();
    par.beginGroup("common");
        par.setValue("sample", ui->sampleEdit->text());
        par.setValue("angle", ui->angleSpinBox->value());
        par.setValue("amplitude", ui->ampEdit->text());
        par.setValue("rating", ui->ratingEdit->text());
        par.setValue("comment", ui->commentEdit->toPlainText());
        par.setValue("loadingSmooth", curr->loadingSmooth);
        par.setValue("grain", curr->grain);
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

        par.setValue("verticalFit", curr->verticalFit[0]);
        par.setValue("filterFrame", curr->frame[0]);
        par.setValue("filterPower", curr->power[0]);
        par.setValue("derivative", curr->criticalDer[0]);
        par.beginGroup("splits");

        par.endGroup();
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
        if(calc->series[0]->isVisible() || calc->series[1]->isVisible()){
            xRange = QPointF(qMin(xRange.x(), calc->xRange.x()), qMax(xRange.y(), calc->xRange.y()));
            yRange = QPointF(qMin(yRange.x(), calc->yRange.x()), qMax(yRange.y(), calc->yRange.y()));
        }
    }
    axisX->setRange(xRange.x()*1.01, xRange.y()*1.01);
    axisY->setRange(yRange.x()*1.01, yRange.y()*1.01);
    xZeroSer->clear();
    xZeroSer->append(axisX->min(), 0.0);
    xZeroSer->append(axisX->max(), 0.0);
    yZeroSer->clear();
    yZeroSer->append(0.0, axisY->min());
    yZeroSer->append(0.0, axisX->max());
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}


MainWindow::Calc* MainWindow::current(){
    if(ui->fileTable->selectionModel()->selectedRows().size() > 0){
        int maxRow = ui->fileTable->selectionModel()->selectedRows().at(0).row();
        foreach (QModelIndex curr, ui->fileTable->selectionModel()->selectedRows()) {
            if(curr.row() > maxRow){
                maxRow = curr.row();
            }
        }
        QString name = ui->fileTable->item(maxRow, 0)->text();
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

void MainWindow::emitUpdate(const Calc* curr){
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << curr->name;
    QObject::connect(this, &MainWindow::update, curr->calc, &Calculator::update, Qt::QueuedConnection);
    emit update(curr->signal, curr->files, curr->zeroNorm);
    QObject::disconnect(this, &MainWindow::update, 0, 0);
    qDebug() << this->metaObject()->className() <<  "::" << __FUNCTION__ << "exit";
}

void::MainWindow::updateSplits(const int file, const int index){
    qDebug() << this->metaObject()->className() << ":: updateSplits" << file << index;
    if(index == -1){
        ui->splitComboBox->clear();
        ui->splitComboBox->addItem("", 0);
        Calc* curr = current();
        for(int rise = 0; rise < 2; ++rise){
            QString prefix = "fall ";
            int sign = -1;
            if(rise == 1){
                prefix = "rise ";
                sign = 1;
            }
            for(int i = 0; i < curr->calc->splits[file][rise]->size(); ++i){
                ui->splitComboBox->addItem(prefix + QString::number(curr->calc->splits[file][rise]->at(i)->x), sign*(i + 1));
            }
        }
    }else{
        ui->splitComboBox->currentIndexChanged(ui->splitComboBox->currentIndex());
    }
    qDebug() << this->metaObject()->className() << ":: updateSplits::exit";
}
