#include "pumpcontroller.h"
#include "ui_pumpcontroller.h"
#include "comsdialog.h"
#include "theming.h"

PumpController::PumpController(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::PumpController),
    pumpComPort("None"),
    condComPort("None"),
    offset(0.00)
{
    // Create window
    ui->setupUi(this);
    this->setWindowTitle(QString("Pump Controller"));

    // Table model setup
    tableModel = new TableModel(this);
    QHeaderView* vHeader=ui->tableSegments->verticalHeader();
    vHeader->setDefaultSectionSize(20); // 20 px height
    vHeader->setSectionResizeMode(QHeaderView::Fixed);
    QHeaderView* hHeader=ui->tableSegments->horizontalHeader();
    hHeader->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableSegments->setModel(tableModel);
    // We reimplment selection so clicking selects/deselects whole rows
    ui->tableSegments->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableSegments->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Timer stuff
    runTimer = new QTimer(this);
    runTimer->setSingleShot(1);

    intervalTimer = new QTimer(this);
    intervalTimer->setSingleShot(0);

    condTimer = new QTimer(this);
    condTimer->setSingleShot(0);

    currProtocol = new Protocol(this);


    // Set default settings for everything
    ui->spinFlowRate->setValue(0.4);
    ui->spinPac->setValue(0);
    ui->spinPbc->setValue(125);

    // Disable everything before confirmation
    ui->spinStraightConc->setDisabled(1);
    ui->butStartPump->setDisabled(1);
    ui->butUpdatePump->setDisabled(1);
    ui->butStopPump->setDisabled(1);
    ui->spinSegTime->setDisabled(1);
    ui->spinStartConc->setDisabled(1);
    ui->spinEndConc->setDisabled(1);
    ui->butAddSegment->setDisabled(1);
    ui->butClearSegments->setDisabled(1);
    ui->tableSegments->setDisabled(1);
    ui->butStartProtocol->setDisabled(1);
    ui->butStopProtocol->setDisabled(1);
    ui->butUpdateProtocol->setDisabled(1);
    ui->tableSegments->resizeColumnsToContents();

    // SIGNALS TO SLOTS
    // These are for UX, disabling/enabling to encourage order of operations. Can't start protocols until settings are confirmed.
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
    QString formattedText = "<code>" + QDateTime::currentDateTime().toString("HH:mm:ss.ms")+" | "+ QString("<span style=\"white-space: pre-wrap; color: %1\">%2</span></code><br>")
    .arg(color.name(), text);

    QTextCursor cursor = ui->console->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->console->setTextCursor(cursor);
    ui->console->insertHtml(formattedText);
    cursor.movePosition(QTextCursor::PreviousCharacter);
    ui->console->setTextCursor(cursor);
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
    ui->spinStraightConc->setEnabled(1);
    ui->butStartPump->setEnabled(1);
    ui->butUpdatePump->setEnabled(1);
    ui->butStopPump->setEnabled(1);
    ui->spinSegTime->setEnabled(1);
    ui->spinStartConc->setEnabled(1);
    ui->spinEndConc->setEnabled(1);
    ui->butAddSegment->setEnabled(1);
    ui->butClearSegments->setEnabled(1);
    ui->tableSegments->setEnabled(1);
    ui->butStartProtocol->setEnabled(1);
    ui->butStopProtocol->setEnabled(1);
    ui->butUpdateProtocol->setEnabled(1);
    ui->butConfirmSettings->setDisabled(1);
    ui->spinSegTime->setMaximum(30); // # 30 minutes maximum time; limit of pump pause?
    ui->protocolPlot->setYAxis(ui->spinPac->value(), ui->spinPbc->value());
    ui->spinStartConc->setMaximum(std::max(ui->spinPac->value(), ui->spinPbc->value()));
    ui->spinEndConc->setMaximum(std::max(ui->spinPac->value(), ui->spinPbc->value()));

    // Maybe add logic to update segments if segments are changed!


    //this->ui->spin_start_conc->setMaximum(max(this->ui->spin_pac->value(), this->ui->spin_pbc->value()));
    //this->ui->spin_end_conc->setMaximum(max(this->ui->spin_pac->value(), this->ui->spin_pbc->value()));
    //this->protocol->set_dt(1) // # Locked to 1s for conductivity meter;
    //this->ui->spin_straight_conc->setMaximum(max(this->ui->spin_pac->value(), this->ui->spin_pbc->value()));

    //offset = max(this->ui->spin_pac->value(), this->ui->spin_pbc->value()) * 0.10;
    //this->ui->widget_plots->set_yax(min(this->ui->spin_pac->value(), this->ui->spin_pbc->value())-offset,max(this->ui->spin_pac->value(), this->ui->spin_pbc->value())+offset)
    //this->ui->widget_plot_cond->clear_axes()
    //this->cond_timer.start(1000)
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
    ui->butAddSegment->setDisabled(1);
    ui->butClearSegments->setDisabled(1);
    ui->tableSegments->setDisabled(1);
    ui->butStartProtocol->setDisabled(1);
    ui->butStopProtocol->setDisabled(1);
    ui->butUpdateProtocol->setDisabled(1);

    //this>port = None
    //this.pumps = None
    //this->cond_timer->stop();
    //this->record_cond=0;

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
        qDebug()<<tableModel->getSegments();

    } else {
        writeToConsole("You can't add a segment zero minutes long...", UiYellow);
    }

}

void PumpController::rmSegment()
{
    if (! ui->tableSegments->selectionModel()->selectedRows().isEmpty())
    {
        tableModel->removeSegment(ui->tableSegments->selectionModel()->selectedRows().first().row());
    }
}

void PumpController::clearSegments()
{
    tableModel->clearSegments();
}

void PumpController::updateProtocol()
{
    currProtocol->generate(tableModel->getSegments());
    ui->protocolPlot->setData(currProtocol->xvals(),currProtocol->yvals());

}
