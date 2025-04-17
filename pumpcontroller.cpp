#include <QtMath>
#include <QDebug>

#include "pumpcontroller.h"
#include "ui_pumpcontroller.h"
#include "comsdialog.h"
#include "theming.h"

PumpController::PumpController(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::PumpController),
    pumpComPort(""),
    condComPort(""),
    //offset(0.00),       //
    xPos(-1),            // hides the vLine off the protocol chart
    protocolChanged(1)
{
    // Create window
    ui->setupUi(this);
    this->setWindowTitle(QString("Pump Controller"));

    QFont monoFont;

#ifdef Q_OS_MAC
    monoFont = QFont("Monaco", 14);  // macOS default system font
#elif defined(Q_OS_WIN)
    monoFont = QFont("Courier New", 11);
#else
    monoFont = QFont("Courier New", 11);
#endif

    ui->console->setFont(monoFont);

    // Table and model setup
    tableModel = new TableModel(this);
    QHeaderView* vHeader=ui->tableSegments->verticalHeader();
    vHeader->setDefaultSectionSize(20); // 20 px height
    vHeader->setSectionResizeMode(QHeaderView::Fixed);
    QHeaderView* hHeader=ui->tableSegments->horizontalHeader();
    hHeader->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableSegments->setModel(tableModel);
    ui->tableSegments->resizeColumnsToContents();
    // Here we reimplement selection so clicking selects/deselects whole rows
    ui->tableSegments->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableSegments->setSelectionBehavior(QAbstractItemView::SelectRows);
    currProtocol = new Protocol(this);

    // Timer stuff
    runTimer = new QTimer(this);
    runTimer->setTimerType(Qt::PreciseTimer);
    runTimer->setSingleShot(1);
    intervalTimer = new QTimer(this);
    intervalTimer->setSingleShot(0);
    intervalTimer->start(currProtocol->dt()*1000); // This will most likely never change.

    //condTimer = new QTimer(this);
    //condTimer->setSingleShot(0);


    // Set default settings for everything
    ui->spinFlowRate->setValue(0.4);
    ui->spinPac->setValue(0);
    ui->spinPbc->setValue(125);

    // Disable everything before confirmation
    ui->spinStraightConc->setDisabled(1);
    ui->butStartPump->setDisabled(1);
    ui->butUpdatePump->setDisabled(1);
    ui->butStopPump->setDisabled(1);
    //ui->spinSegTime->setDisabled(1);
    //ui->spinStartConc->setDisabled(1);
    //ui->spinEndConc->setDisabled(1);
    //ui->butAddSegment->setDisabled(1);
    //ui->butClearSegments->setDisabled(1);
    //ui->tableSegments->setDisabled(1);
    ui->butStartProtocol->setDisabled(1);
    ui->butStopProtocol->setDisabled(1);
    ui->butSendProtocol->setDisabled(1);
    //ui->butDeleteSegment->setDisabled(1);
    ui->butSetCondMin->setDisabled(1);
    ui->butSetCondMax->setDisabled(1);
    ui->butResetCond->setDisabled(1);

    // Adjust the conductivity plot from default
    ui->condPlot->setYlabel("mS/cm");
    ui->label_cond_units->setText("mS/cm");
    ui->condPlot->setYAxis(0,15);



    // SIGNALS TO SLOTS
    // These are for UX, disabling/enabling to encourage order of operations. Can't start protocols until settings are confirmed.
    connect(ui->butClearConsole, &QPushButton::clicked, this, &PumpController::clearConsole);
    connect(ui->butSaveConsole, &QPushButton::clicked, this, &PumpController::saveConsole);

    connect(ui->butConfirmSettings, &QPushButton::clicked, this, &PumpController::confirmSettings);
    connect(ui->butSetComs, &QPushButton::clicked, this, &PumpController::openCOMsDialog);
    connect(ui->spinFlowRate, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &PumpController::settingsChanged);
    connect(ui->spinPac, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PumpController::settingsChanged);
    connect(ui->spinPbc, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PumpController::settingsChanged);

    connect(ui->butAddSegment, &QPushButton::clicked, this, &PumpController::addSegment);
    connect(ui->butDeleteSegment, &QPushButton::clicked, this, &PumpController::rmSegment);
    connect(ui->butClearSegments, &QPushButton::clicked, this, &PumpController::clearSegments);
    // this one allows for clicking and unclicking a row
    connect(ui->tableSegments, &QTableView::clicked, this, [=](const QModelIndex &index) {
        int row = index.row();
        QItemSelectionModel *selectionModel = ui->tableSegments->selectionModel();

        QModelIndex topLeft = ui->tableSegments->model()->index(row, 0);
        QModelIndex bottomRight = ui->tableSegments->model()->index(row, ui->tableSegments->model()->columnCount() - 1);
        QItemSelection selection(topLeft, bottomRight);

        if (selectionModel->isRowSelected(row, QModelIndex())) {
            selectionModel->select(selection, QItemSelectionModel::Deselect);
        } else {
            selectionModel->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    });

    connect(tableModel, &TableModel::segmentsChanged, this, &PumpController::updateProtocol);
    connect(ui->butStartProtocol, &QPushButton::clicked, this, &PumpController::startProtocol);
    connect(ui->butSendProtocol, &QPushButton::clicked, this, &PumpController::sendProtocol);
    connect(ui->butStopProtocol, &QPushButton::clicked, this, &PumpController::stopProtocol);

    connect(runTimer, &QTimer::timeout, this, &PumpController::stopProtocol);
    connect(intervalTimer, &QTimer::timeout, this, &PumpController::timerTick);


    // Run timers, for protocol stuff
    //connect(&this->int_timer, &QTimer::timeout, this, &PumpController::timer_tick);
    //connect(&this->cond_timer, &QTimer::timeout, this, &PumpController::cond_timer_tick);
    //connect(&this->run_timer, &QTimer::timeout, this, &PumpController::stop_record_cond);

    connect(this->ui->butStartPump, &QPushButton::clicked, this, &PumpController::startPumps);
    connect(this->ui->butStopPump, &QPushButton::clicked, this, &PumpController::stopPumps);
    connect(this->ui->butUpdatePump, &QPushButton::clicked, this, &PumpController::updatePumps);

    //connect(this->ui->but_start_protocol, &QPushButton::clicked, this, &PumpController::start_protocol);
    //connect(this->ui->but_stop_protocol, &QPushButton::clicked, this, &PumpController::stop_protocol);

    //connect(this->ui->but_update_protocol, &QPushButton::clicked, this, &PumpController::update_protocol);



    //connect(this->ui->but_set_cond_min, &QPushButton::clicked, this, &PumpController::set_cond_min);
    //connect(this->ui->but_set_cond_max, &QPushButton::clicked, this, &PumpController::set_cond_max);
    //connect(this->ui->but_reset_cond, &QPushButton::clicked, this, &PumpController::reset_cond);

    writeToConsole("Welcome to Pump Controller v. "+QString::number(VERSION_MAJOR)+"."+QString::number(VERSION_MINOR)+"."+QString::number(VERSION_BUILD)+"!");
}

PumpController::~PumpController()
{
    delete ui;
}

// SLOT FUNCTIONS

void PumpController::writeToConsole(const QString& text, const QColor& color)
{
    QColor useColor = color.isValid() ? color : ui->console->palette().color(QPalette::Text);

    // This one has a tab character but it's quite long
    //QString formattedText = "<code style=\"white-space:pre\">" + QDateTime::currentDateTime().toString("HH:mm:ss.ms")+"&#9;|"+ QString("<span style=\"white-space: pre-wrap; color: %1\">%2</span></code><br>")
    //.arg(useColor.name(), text);

    QString formattedText = "<code>"+QDateTime::currentDateTime().toString("HH:mm:ss")+" | "+ QString("<span style=\"white-space: pre-wrap; color: %1\">%2</span></code><br>")
        .arg(useColor.name(), text);

    QTextCursor cursor = ui->console->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->console->setTextCursor(cursor);
    ui->console->insertHtml(formattedText);
    cursor.movePosition(QTextCursor::PreviousCharacter);
    ui->console->setTextCursor(cursor);
}

void PumpController::clearConsole()
{
    QMessageBox msg(this);
    msg.setText("Are you sure you want to clear the console?");
    msg.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    int result = msg.exec();

    if (result == 1024)
    {
        ui->console->clear();
    }
    writeToConsole("Console cleared!", UiYellow);
}

void PumpController::saveConsole()
{
    QString saveFile = QFileDialog::getSaveFileName(this, tr("Save Console Text"),QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    QFile colorFile(saveFile+"_colors.md");
    QFile plainFile(saveFile+".txt");
    if (plainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&plainFile);
        out << ui->console->toPlainText();
        plainFile.close();
    } else {
        qWarning() << "Could not open file for writing:" << saveFile+".txt";
    }
    if (colorFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&colorFile);
        out << ui->console->toHtml();
        colorFile.close();
    } else {
        qWarning() << "Could not open file for writing:" << saveFile+"_color.md";
    }
    writeToConsole("Wrote plain text and color version of logs to "+saveFile, UiYellow);

}


void PumpController::openCOMsDialog()
{
    COMsDialog dialog(this);
    connect(&dialog, &COMsDialog::coms, this, &PumpController::setCOMs);
    dialog.exec();
}


void PumpController::setCOMs(const QString& cond, const QString& pump)
// Need to add timeout to this
{
    if (!pump.isEmpty()) {
        pumpComPort = pump;
        writeToConsole("PUMP PORT SELECTED: " + pumpComPort, UiGreen);
        initiatePumps();
        //commandWorker = new PumpCommandWorker(pumps, this);  // parent is QObject, and this is PumpInterface
        //commandWorker->start();
    } else {
        pumpComPort.clear();
        writeToConsole("No pump port selected!", UiRed);
    }

    if (!cond.isEmpty()) {
        condComPort = cond;
        writeToConsole("COND METER PORT SELECTED: " + condComPort, UiGreen);
        initiateCond();

    } else {
        condComPort.clear();
        writeToConsole("No cond meter port selected!", UiRed);
    }
    this->settingsChanged();
}

void PumpController::initiateCond()
{
    if (condInterface) {
        condInterface->shutdown();
        delete condInterface;
        condInterface = nullptr;
    }
    if (!condComPort.isEmpty())
    {
        qDebug() <<"Creating cond";
        condInterface = new CondInterface(this);
        condInterface->connectToMeter(condComPort);
        connect(condInterface, &CondInterface::measurementReceived,
                this, &PumpController::receiveCondMeasurement);
        connect(condInterface, &CondInterface::errorOccurred, this, &PumpController::receivePumpError);
    }

}

void PumpController::receiveCondMeasurement(CondReading reading)
{
    qDebug() << "Conductivity:" << reading.value << reading.units;
    double mSReading = reading.value;
    if (reading.units == "mS/cm")
    {
        ui->label_cond->setText(QString::number(mSReading, 'f', 2));
    } else if (reading.units == "uS/cm")
    {
        mSReading = reading.value / 1000;
        ui->label_cond->setText(QString::number(mSReading,'f',2));
    }

    if (!runTimer->isActive())
    {
        // Save the previous 120 measurements (aka 1 minute).
        if (condPreReadings.size() >= condPreSaveWindow) {
            condPreReadings.pop_front();  // Remove oldest
        }

        condPreReadings.push_back(mSReading);  // Add newest
        qDebug() << QTime::currentTime() << ": added to previous readings";
    } else {
        condReadings.append(mSReading);
        qDebug() << QTime::currentTime() << ": collecting protocol readings";
    }


}

void PumpController::initiatePumps()
// For this, Pump A is address 0, Pump B is address 1
// This is true in all cases.
{
    if (pumpInterface) {
        pumpInterface->shutdown();
        delete pumpInterface;
        pumpInterface = nullptr;
    }


    if (! pumpComPort.isEmpty())
    {
        pumpInterface = new PumpInterface(this);
        pumpInterface->connectToPumps(pumpComPort);
        connect(pumpInterface, &PumpInterface::errorOccurred, this, &PumpController::receivePumpError);
        connect(pumpInterface, &PumpInterface::dataReceived, this, &PumpController::receivePumpResponse);

    }
}

void PumpController::receivePumpError(const QString& err)
{
    writeToConsole(err,UiRed);
}

void PumpController::receivePumpResponse(const QString& msg)
{
    int source = msg.left(2).toInt();
    if (source == 0) {
        writeToConsole(QString("Pump A response: ")+ msg, UiYellow);
    } else if (source == 1) {
        writeToConsole(QString("Pump B response: ")+ msg, UiYellow);
        }
    else {
        writeToConsole(QString("Pump Msg: ")+msg);
    }


}

void PumpController::confirmSettings()
{
    writeToConsole("PUMP SETTINGS CONFIRMED: ", UiGreen);
    writeToConsole("Flow Rate (mL/min): " + QString::number(ui->spinFlowRate->value(), 'f', 2) +
                              " | Pump A (mM): " + QString::number(ui->spinPac->value(), 'f', 0) +
                              " | Pump B (mM): " + QString::number(ui->spinPbc->value(), 'f', 0), UiGreen);
    if (pumpComPort == "None" || condComPort == "None" ) {
        writeToConsole("Confirm ports for pumps and meter! At least one was not selected!", UiRed);
    }
    ui->butConfirmSettings->setStyleSheet("QPushButton { color: mediumseagreen;}");
    ui->butConfirmSettings->setText("Confirmed");


    ui->spinSegTime->setEnabled(1);
    ui->spinStartConc->setEnabled(1);
    ui->spinEndConc->setEnabled(1);
    //ui->butAddSegment->setEnabled(1);
    //ui->butClearSegments->setEnabled(1);
    //ui->butDeleteSegment->setEnabled(1);
    //ui->tableSegments->setEnabled(1);
    ui->butStartProtocol->setEnabled(1);
    //ui->butStopProtocol->setEnabled(1);
    ui->butConfirmSettings->setDisabled(1);
    ui->spinSegTime->setMaximum(120); // # 30 minutes maximum time; limit of pump pause?
    ui->protocolPlot->setYAxis(ui->spinPac->value(), ui->spinPbc->value());
    ui->spinStartConc->setMaximum(std::max(ui->spinPac->value(), ui->spinPbc->value()));
    ui->spinEndConc->setMaximum(std::max(ui->spinPac->value(), ui->spinPbc->value()));
    tableModel->updateSegments();
    if (!pumpComPort.isEmpty()) {
        ui->spinStraightConc->setEnabled(1);
        ui->butStartPump->setEnabled(1);
        ui->butUpdatePump->setEnabled(1);
        ui->butStopPump->setEnabled(1);
        ui->butSendProtocol->setEnabled(1);
    }
    //

}

void PumpController::settingsChanged()
{

    ui->butConfirmSettings->setEnabled(1);
    ui->butConfirmSettings->setStyleSheet(ui->butResetCond->styleSheet());
    ui->butConfirmSettings->setText("Confirm");
    ui->spinStraightConc->setDisabled(1);
    ui->butStartPump->setDisabled(1);
    ui->butUpdatePump->setDisabled(1);
    ui->butStopPump->setDisabled(1);
    ui->spinSegTime->setDisabled(1);
    ui->spinStartConc->setDisabled(1);
    ui->spinEndConc->setDisabled(1);
    //ui->butAddSegment->setDisabled(1);
    //ui->butClearSegments->setDisabled(1);

    //ui->tableSegments->setDisabled(1);
    ui->butStartProtocol->setDisabled(1);
    ui->butStopProtocol->setDisabled(1);
    ui->butSendProtocol->setDisabled(1);

}


// SEGMENT SLOTS

void PumpController::addSegment()
{
    if (ui->spinSegTime->value() > 0){
        int row = -1;
        QModelIndexList selected = ui->tableSegments->selectionModel()->selectedRows();
        if (!selected.isEmpty()) {
            row = selected.first().row()+1;

        }
        tableModel->addSegment(ui->spinSegTime->value(), ui->spinStartConc->value(), ui->spinEndConc->value(), row);
        ui->spinSegTime->setValue(0.00);
        ui->spinStartConc->setValue(0);
        ui->spinEndConc->setValue(0);
        ui->tableSegments->selectionModel()->clearSelection();
        //qDebug()<<tableModel->getSegments();

    } else {
        writeToConsole("You can't add a segment zero minutes long...", UiYellow);
    }

}

void PumpController::rmSegment()
{
    if (! ui->tableSegments->selectionModel()->selectedRows().isEmpty())
    {
        tableModel->removeSegment(ui->tableSegments->selectionModel()->selectedRows().first().row());
    } else {
        writeToConsole("Pick a segment to remove first!", UiYellow);
    }
}

void PumpController::clearSegments()
{
    if (tableModel->rowCount(QModelIndex())>0)
    {
        tableModel->clearSegments();

    } else {
        writeToConsole("Maybe add some segments first -- nothing to clear.", UiYellow);
    }

}

//void PumpController::updateProtocolButton()
//{

//}

void PumpController::updateProtocol()
// Not a button, but called automatically whenever the protocol changes.
{
    //if (tableModel->rowCount(QModelIndex())>0)
    //{
    currProtocol->generate(tableModel->getSegments());
    //qDebug()<<currProtocol->xvals();
    //qDebug()<<currProtocol->yvals();
    ui->protocolPlot->setData(currProtocol->xvals(),currProtocol->yvals());

    //} else {
        //writeToConsole("I'm afraid you can't update with an empty protocol.", UiYellow);
    //}

}

void PumpController::startPumps()
{
    pumpInterface->startPumps(1);
}
void PumpController::updatePumps()
{
    double conc = ui->spinStraightConc->value();
    QVector<QVector<double>> run = { QVector<double>{conc} };
    QVector<QVector<PumpPhase>> phases = generatePumpPhases(0, run);
    pumpInterface->setPhases(phases);
}

void PumpController::stopPumps()
{
    if (runTimer->isActive())
    {
        stopProtocol();
    } else {
    pumpInterface->stopPumps();
    }
}

// PROTOCOL SLOTS

void PumpController::startProtocol()
{
    startTime=QTime::currentTime();
    if (tableModel->rowCount(QModelIndex())>0)
    {
        writeToConsole("Current segments: ", UiBlue);
        for (const auto& seg : currProtocol->shareSegments()) {
            if (seg.size() != 3) continue;  // Expecting [duration, start, end]

            double duration = seg[0];
            double start = seg[1];
            double end = seg[2];
            writeToConsole(QString::number(duration, 'f', 2)+" min | "+QString::number(start)+" mM | " +QString::number(end)+" mM", UiBlue);
        }
        // timepoints in seconds. subtract one because we care about time intervals
        // (e.g. 15 seconds is 31 points, so subtract one to give 15 seconds.
        // I subtract a second time point too because I skip the first point to sync
        // with the interval timer.
        double totalTime = (currProtocol->xvals().count() - 1)*currProtocol->dt()*1000; //
        qDebug() << "Total Time: " << totalTime;

        // Use a lambda that captures `this`
        auto starter = new QObject(this); // use a temporary object for connection context

        connect(intervalTimer, &QTimer::timeout, starter, [this, starter, totalTime]() {
            // Disconnect this temporary connection
            disconnect(intervalTimer, nullptr, starter, nullptr);
            starter->deleteLater();



            // Only the first interval might be shorter, the rest will be syncd with intTimer
            runTimer->start(totalTime);
            //qDebug() << "Synchronized protocol start! runTimer remaining time now: " << runTimer->remainingTime();
            xPos = 0;
            ui->protocolPlot->setX(currProtocol->xvals().at(xPos));
            writeToConsole("Protocol started.", UiGreen);
        });
        ui->butStopProtocol->setEnabled(1);
        ui->butStartPump->setDisabled(1);
        ui->butUpdatePump->setDisabled(1);
        ui->butStopPump->setDisabled(1);
        ui->butStartProtocol->setText("Restart");
        ui->butAddSegment->setDisabled(1);
        ui->butDeleteSegment->setDisabled(1);
        ui->butClearSegments->setDisabled(1);
        ui->spinFlowRate->setDisabled(1);
        ui->spinPac->setDisabled(1);
        ui->spinPbc->setDisabled(1);
        ui->butSetComs->setDisabled(1);

        ui->butSendProtocol->setDisabled(1);

        if (!pumpComPort.isEmpty()) {
            pumpInterface->startPumps(2);
        }
        if (!condComPort.isEmpty()) {
            condReadings.clear();
            qDebug() << QTime::currentTime() << "Cleared previous readings";
            condInterface->getMeasurement();
        }

    } else {
        writeToConsole("Your protocol is empty! What are you running?", UiYellow);
    }
}

void PumpController::sendProtocol()
{
    // Protocol phases start at Phase 2, so set offset to 1 (to skip first phase).
    writeToConsole("Sent protocol to pumps", UiBlue);
    QVector<QVector<PumpPhase>> phases = generatePumpPhases(1, tableModel->getSegments());
    pumpInterface->setPhases(phases);
}


void PumpController::stopProtocol()
// This is called upon hitting Stop Protocol button
{
    if (runTimer->isActive())
    {
        writeToConsole("Protocol stopped", UiYellow);
        runTimer->stop();
    } else {
        writeToConsole("Protocol ended on its own", UiGreen);
    }
    xPos = -1; // just in case lets reset these
    ui->protocolPlot->setX(-1);
    ui->butStopProtocol->setDisabled(1);
    ui->butStartProtocol->setText("Start");
    ui->butAddSegment->setEnabled(1);
    ui->butClearSegments->setEnabled(1);
    ui->butDeleteSegment->setEnabled(1);
    ui->butSetComs->setEnabled(1);
    ui->spinFlowRate->setEnabled(1);
    ui->spinPac->setEnabled(1);
    ui->spinPbc->setEnabled(1);

    if (!pumpComPort.isEmpty() ) {
        ui->butStartPump->setEnabled(1);
        ui->butUpdatePump->setEnabled(1);
        ui->butStopPump->setEnabled(1);
        ui->butSendProtocol->setEnabled(1);
        pumpInterface->stopPumps();
    }


}

void PumpController::timerTick()
// At each time point, record a conductivity meter reading and update plot X position
{
    if (condInterface)
    {
        qDebug()<<"requesting Measurement";
        condInterface->getMeasurement();
    }
    if (runTimer->isActive())
    {
        int secsDiff = startTime.secsTo(QTime::currentTime());
        double elapsedMinutes = secsDiff / 60.0;
        qDebug() << "Elapsed:" << elapsedMinutes << "minutes";

        ui->protocolPlot->setX(elapsedMinutes);  // move marker to correct X pos based on time
    }
}

// UTILITY FUNCTIONS (Const)

QVector<double> PumpController::calculateFlowRates(double concentration) const {
    // returns flow rates as uL / min

    double pac = ui->spinPac->value();
    double pbc = ui->spinPbc->value();
    double totalFlowRate = ui->spinFlowRate->value();

    if (std::abs(pbc - pac) < 1e-6) {
        return {0.0, 0.0};
    }

    double bRate = ((concentration - pac) / (pbc - pac)) * totalFlowRate;
    double aRate = totalFlowRate - bRate;

    aRate = std::round(std::abs(aRate) * 1000.0); // / 1000.0;
    bRate = std::round(std::abs(bRate) * 1000.0); // / 1000.0;

    return {aRate, bRate};
}



QVector<QVector<PumpPhase>> PumpController::generatePumpPhases(const int startPhase, const QVector<QVector<double>>& segments)
{
    QVector<PumpPhase> phasesA;
    QVector<PumpPhase> phasesB;

    int phaseCounterA = startPhase + 1;     // startPhase is 0 for base run or 1 for program
    int phaseCounterB = startPhase + 1;

    if (segments[0].size() ==1 ) // non-protocol version
    {
        int conc = segments[0][0];
        QVector<double> rates = calculateFlowRates(conc);

        // Constant rate segment — RAT
        double aRate = rates[0];
        double bRate = rates[1];

        // Pump A
        if (aRate > 0) {
            PumpPhase phaseA;
            phaseA.phaseNumber = phaseCounterA;
            phaseA.function = "RAT";
            phaseA.rate = aRate;
            phaseA.volume = 0;
            phaseA.direction = "INF";
            phasesA.append(phaseA);
        } else {
            // Pause for pump A
            PumpPhase pauseA;
            pauseA.phaseNumber = phaseCounterA;
            pauseA.function = "STOP";
            phasesA.append(pauseA);
        }

        // Pump B
        if (bRate > 0) {
            PumpPhase phaseB;
            phaseB.phaseNumber = phaseCounterB;
            phaseB.function = "RAT";
            phaseB.rate = aRate;
            phaseB.volume = 0;
            phaseB.direction = "INF";
            phasesB.append(phaseB);
        } else {
            // Pause for pump B
            PumpPhase pauseB;
            pauseB.phaseNumber = phaseCounterB;
            pauseB.function = "STOP";
            phasesB.append(pauseB);
        }
    } else
    {
        for (const auto& row : segments)
        {
            if (row.size() < 3) continue;

            double timeMin = row[0];
            int startConc = row[1];
            int endConc = row[2];

            QVector<double> startRates = calculateFlowRates(startConc); // [a_rate, b_rate]
            QVector<double> endRates = calculateFlowRates(endConc);

            // Estimate how many new phases will be added for A and B
            int newPhasesA = 0, newPhasesB = 0;
            double totalTimeSec = timeMin * 60;

            if (startConc == endConc)
            {
                newPhasesA = (startRates[0] > 0) ? 1 : static_cast<int>(std::ceil(totalTimeSec / 99.0));
                newPhasesB = (startRates[1] > 0) ? 1 : static_cast<int>(std::ceil(totalTimeSec / 99.0));
            } else
            {
                newPhasesA = 2; // LIN start and end
                newPhasesB = 2;
            }

            if ((phaseCounterA + newPhasesA > 40) || (phaseCounterB + newPhasesB > 40))
            {
                writeToConsole("########################    WARNING!!!!   ########################", UiRed);
                writeToConsole("There are too many phases, please reduce!", UiRed);
                writeToConsole("Pump A: " + QString::number(phaseCounterA + newPhasesA) +"; Pump B: "+ QString::number(phaseCounterB + newPhasesB), UiRed);
                writeToConsole("The pumps DO NOT match your expected settings!!!", UiRed);
                break;
            }

            if (startConc == endConc)
            {
                // Constant rate segment — RAT
                double aRate = startRates[0];
                double bRate = startRates[1];

                double totalTimeSec = timeMin * 60;

                // Pump A
                if (aRate > 0)
                {
                    double volume = aRate * timeMin;
                    PumpPhase phaseA;
                    phaseA.phaseNumber = phaseCounterA;
                    phaseA.function = "RAT";
                    phaseA.rate = aRate;
                    phaseA.volume = volume;
                    phaseA.direction = "INF";
                    phasesA.append(phaseA);
                    phaseCounterA++;
                } else
                {
                    // Pause for pump A
                    int remaining = static_cast<int>(totalTimeSec);
                    while (remaining > 0)
                    {
                        int chunk = qMin(remaining, 99);
                        PumpPhase pause;
                        pause.phaseNumber = phaseCounterA;
                        pause.function = "PAUSE";
                        pause.time = QString::number(chunk);//QString("00:%1").arg(chunk, 2, 10, QLatin1Char('0'));
                        phasesA.append(pause);
                        remaining -= chunk;
                        phaseCounterA++;
                    }
                }

                // Pump B
                if (bRate > 0)
                {
                    double volume = bRate * timeMin;
                    PumpPhase phaseB;
                    phaseB.phaseNumber = phaseCounterB;
                    phaseB.function = "RAT";
                    phaseB.rate = bRate;
                    phaseB.volume = volume;
                    phaseB.direction = "INF";
                    phasesB.append(phaseB);
                    phaseCounterB++;
                } else
                {
                    // Pause for pump B
                    int remaining = static_cast<int>(totalTimeSec);
                    while (remaining > 0)
                    {
                        int chunk = qMin(remaining, 99);
                        PumpPhase pause;
                        pause.phaseNumber = phaseCounterB;
                        pause.function = "PAUSE";
                        pause.time = QString::number(chunk);//QString("00:%1").arg(chunk, 2, 10, QLatin1Char('0'));
                        phasesB.append(pause);
                        remaining -= chunk;
                        phaseCounterB++;
                    }
                }

            } else
            {
                // Linear ramp segment — LIN (two parts)
                int minutes = static_cast<int>(timeMin);
                int seconds = static_cast<int>((timeMin - minutes) * 60);
                int tenths = static_cast<int>(qRound(((timeMin - minutes) * 60 - seconds) * 10));

                // Pump A - Start
                PumpPhase phaseA_start;
                phaseA_start.phaseNumber = phaseCounterA;
                phaseA_start.function = "LIN";
                phaseA_start.rate = startRates[0];
                phaseA_start.time = QString("%1:%2").arg(0, 2, 10, QLatin1Char('0')).arg(minutes, 2, 10, QLatin1Char('0'));
                phaseA_start.direction = "INF";
                phasesA.append(phaseA_start);

                // Pump A - End
                PumpPhase phaseA_end;
                phaseA_end.phaseNumber = phaseCounterA + 1;
                phaseA_end.function = "LIN";
                phaseA_end.rate = endRates[0];
                phaseA_end.time = QString("%1:%2").arg(seconds, 2, 10, QLatin1Char('0')).arg(tenths, 2, 10, QLatin1Char('0'));
                phaseA_end.direction = "INF";
                phasesA.append(phaseA_end);

                phaseCounterA += 2;

                // Pump B - Start
                PumpPhase phaseB_start;
                phaseB_start.phaseNumber = phaseCounterB;
                phaseB_start.function = "LIN";
                phaseB_start.rate = startRates[1];
                phaseB_start.time = QString("%1:%2").arg(0, 2, 10, QLatin1Char('0')).arg(minutes, 2, 10, QLatin1Char('0'));
                phaseB_start.direction = "INF";
                phasesB.append(phaseB_start);

                // Pump B - End
                PumpPhase phaseB_end;
                phaseB_end.phaseNumber = phaseCounterB + 1;
                phaseB_end.function = "LIN";
                phaseB_end.rate = endRates[1];
                phaseB_end.time = QString("%1:%2").arg(seconds, 2, 10, QLatin1Char('0')).arg(tenths, 2, 10, QLatin1Char('0'));
                phaseB_end.direction = "INF";
                phasesB.append(phaseB_end);

                phaseCounterB += 2;
            }
            qDebug() << "Phases: " << phaseCounterA << phaseCounterB;
        }

        // Pump A - End
        PumpPhase phaseA_stop;
        phaseA_stop.phaseNumber = phaseCounterA;
        phaseA_stop.function = "STOP";
        phasesA.append(phaseA_stop);
        qDebug() << "Stops: " << phaseCounterA << phaseCounterB;

        // Pump B - Start
        PumpPhase phaseB_stop;
        phaseB_stop.phaseNumber = phaseCounterB;
        phaseB_stop.function = "STOP";
        phasesB.append(phaseB_stop);
    }

    return { phasesA, phasesB };
}


