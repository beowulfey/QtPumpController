#include "pumpcontroller.h"
#include "ui_pumpcontroller.h"
#include "comsdialog.h"
#include "theming.h"

PumpController::PumpController(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::PumpController),
    pumpComPort("None"),
    condComPort("None"),
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
    ui->condPlot->setYlabel("Units");



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

    //connect(this->ui->but_start_pump, &QPushButton::clicked, this, &PumpController::start_pump);
    //connect(this->ui->but_stop_pump, &QPushButton::clicked, this, &PumpController::stop_pump);
    //connect(this->ui->but_update_pump, &QPushButton::clicked, this, &PumpController::update_pump);

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
{
    if (pump != "None") {
        pumpComPort = pump;
        writeToConsole("PUMP PORT SELECTED: " + pumpComPort, UiGreen);
        initiatePumps();
    } else {
        pumpComPort.clear();
        writeToConsole("No pump port selected!", UiRed);
    }

    if (cond != "None") {
        condComPort = cond;
        writeToConsole("COND METER PORT SELECTED: " + condComPort, UiGreen);
    } else {
        condComPort.clear();
        writeToConsole("No cond meter port selected!", UiRed);
    }
    this->settingsChanged();
}

void PumpController::initiatePumps()
// For this, Pump A is address 0, Pump B is address 1
// This is true in all cases.
{
    if (! pumpComPort.isEmpty())
    {
        pumps = new PumpInterface(this);
        pumps->openPort(pumpComPort);
        connect(pumps, &PumpInterface::errorOccurred, this, &PumpController::receivePumpError);
        connect(pumps, &PumpInterface::dataReceived, this, &PumpController::receivePumpError);
    }
}

void PumpController::receivePumpError(const QString& err)
{
    writeToConsole(err,UiRed);
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
    ui->spinSegTime->setMaximum(30); // # 30 minutes maximum time; limit of pump pause?
    ui->protocolPlot->setYAxis(ui->spinPac->value(), ui->spinPbc->value());
    ui->spinStartConc->setMaximum(std::max(ui->spinPac->value(), ui->spinPbc->value()));
    ui->spinEndConc->setMaximum(std::max(ui->spinPac->value(), ui->spinPbc->value()));
    tableModel->updateSegments();

    if (pumpComPort != "None" ) {
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
    if (tableModel->rowCount(QModelIndex())>0)
    {
        currProtocol->generate(tableModel->getSegments());
        ui->protocolPlot->setData(currProtocol->xvals(),currProtocol->yvals());

    } else {
        //writeToConsole("I'm afraid you can't update with an empty protocol.", UiYellow);
    }

}

// PROTOCOL SLOTS

void PumpController::startProtocol()
{
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
            qDebug() << "Synchronized protocol start! runTimer remaining time now: " << runTimer->remainingTime();
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

    } else {
        writeToConsole("Your protocol is empty! What are you running?", UiYellow);
    }
}

void PumpController::sendProtocol()
{}


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

    if (pumpComPort != "None" ) {
        ui->butStartPump->setEnabled(1);
        ui->butUpdatePump->setEnabled(1);
        ui->butStopPump->setEnabled(1);
        ui->butSendProtocol->setEnabled(1);
    }


}

void PumpController::timerTick()
// At each time point, record a conductivity meter reading and update plot X position
{
    // Example for 15 seconds. xvals() is 31 points. First timerTick is xvals[0], second is xvals[1]
    // This would equate to 14500 ms left.
    if (runTimer->isActive())
    {

        if (xPos <= currProtocol->xvals().length()){
            xPos += 1;
            ui->protocolPlot->setX(currProtocol->xvals().at(xPos));
            //writeToConsole("Test "+QString::number(xPos));
        }

        //double totalTime = ((currProtocol->xvals().count() - 1)*currProtocol->dt())*1000; //
        //double timeLeft = runTimer->remainingTime();
        //int currPos = std::round(totalTime - timeLeft);
        //qDebug() << totalTime << timeLeft << currPos <<xPos << currProtocol->xvals().length(); //<< currProtocol->xvals().length() - currPos;

    }
}
