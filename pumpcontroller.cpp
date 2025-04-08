#include "pumpcontroller.h"
#include "ui_pumpcontroller.h"
#include "comsdialog.h"

PumpController::PumpController(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::PumpController),
    pump_com_port("None"),
    cond_com_port("None")
{
    // Create window
    ui->setupUi(this);
    this->setWindowTitle(QString("Pump Controller"));



    // Set default settings for everything
    this->ui->spin_flow_rate->setValue(0.4);
    this->ui->spin_pac->setValue(0);
    this->ui->spin_pbc->setValue(100);

    // Disable everything before confirmation
    this->ui->spin_straight_conc->setDisabled(1);
    this->ui->but_start_pump->setDisabled(1);
    this->ui->but_update_pump->setDisabled(1);
    this->ui->but_stop_pump->setDisabled(1);
    this->ui->spin_seg_time->setDisabled(1);
    this->ui->spin_start_conc->setDisabled(1);
    this->ui->spin_end_conc->setDisabled(1);
    this->ui->but_add_segment->setDisabled(1);
    this->ui->but_clear_segments->setDisabled(1);
    this->ui->table_segments->setDisabled(1);
    this->ui->but_start_protocol->setDisabled(1);
    this->ui->but_stop_protocol->setDisabled(1);
    this->ui->but_update_protocol->setDisabled(1);
    this->ui->table_segments->resizeColumnsToContents();

    // SIGNALS TO SLOTS
    // These are for UX, disabling/enabling to encourage order of operations. Can't start protocols until settings are confirmed.
    connect(this->ui->but_confirm_settings, &QPushButton::clicked, this, &PumpController::confirmSettings);
    connect(this->ui->but_set_coms, &QPushButton::clicked, this, &PumpController::openCOMsDialog);
    connect(this->ui->spin_flow_rate, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &PumpController::settingsChanged);
    connect(this->ui->spin_pac, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PumpController::settingsChanged);
    connect(this->ui->spin_pbc, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PumpController::settingsChanged);
    connect(this->ui->but_confirm_settings, &QPushButton::clicked, this, &PumpController::confirmSettings);

    // Run timers, for protocol stuff
    //connect(&this->int_timer, &QTimer::timeout, this, &PumpController::timer_tick);
    //connect(&this->cond_timer, &QTimer::timeout, this, &PumpController::cond_timer_tick);
    //connect(&this->run_timer, &QTimer::timeout, this, &PumpController::stop_record_cond);

    //connect(this->ui->but_start_pump, &QPushButton::clicked, this, &PumpController::start_pump);
    //connect(this->ui->but_stop_pump, &QPushButton::clicked, this, &PumpController::stop_pump);
    //connect(this->ui->but_update_pump, &QPushButton::clicked, this, &PumpController::update_pump);
    //connect(this->ui->but_add_segment, &QPushButton::clicked, this, &PumpController::add_segment);
    //connect(this->ui->but_clear_segments, &QPushButton::clicked, this, &PumpController::clear_segments);
    //connect(this->ui->but_start_protocol, &QPushButton::clicked, this, &PumpController::start_protocol);
    //connect(this->ui->but_stop_protocol, &QPushButton::clicked, this, &PumpController::stop_protocol);

    //connect(this->ui->but_update_protocol, &QPushButton::clicked, this, &PumpController::update_protocol);


    //connect(this->ui->but_delete_segment, &QPushButton::clicked, this, &PumpController::rm_segment);
    //connect(this->ui->but_set_cond_min, &QPushButton::clicked, this, &PumpController::set_cond_min);
    //connect(this->ui->but_set_cond_max, &QPushButton::clicked, this, &PumpController::set_cond_max);
    //connect(this->ui->but_reset_cond, &QPushButton::clicked, this, &PumpController::reset_cond);

}

PumpController::~PumpController()
{
    delete ui;
}

// SLOT FUNCTIONS

void PumpController::writeToConsole(const QString& text, const QColor& color)
{
    QString formattedText = QDateTime::currentDateTime().toString("HH:mm:ss.ms")+" | "+ QString("<span style=\"white-space: pre-wrap; color: %1\">%2</span><br>")
    .arg(color.name(), text);

    QTextCursor cursor = this->ui->console->textCursor();
    cursor.movePosition(QTextCursor::End);
    this->ui->console->setTextCursor(cursor);
    this->ui->console->insertHtml(formattedText);
    cursor.movePosition(QTextCursor::PreviousCharacter);
    this->ui->console->setTextCursor(cursor);
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
        pump_com_port = pump;
        writeToConsole("PUMP PORT SELECTED: " + pump_com_port, QColorConstants::Green);
    } else {
        pump_com_port.clear();
        writeToConsole("No pump port selected!", QColorConstants::Red);
    }

    if (cond != "None") {
        cond_com_port = cond;
        writeToConsole("COND METER PORT SELECTED: " + cond_com_port, QColorConstants::Green);
    } else {
        cond_com_port.clear();
        writeToConsole("No cond meter port selected!", QColorConstants::Red);
    }
}


void PumpController::confirmSettings()
{
    //this->write_to_console(f"{datetime->strftime(datetime->now(), FMT)} PUMP SETTINGS CONFIRMED:", color=GREEN)
    //this->write_to_console(f"{datetime->strftime(datetime->now(), FMT)} Flow Rate: {this->ui->spin_flow_rate->value()}; PAC: {this->ui->spin_pac->value()}; PBC: {this->ui->spin_pbc->value()}", color=GREEN)
    this->settingsChanged();
    this->ui->but_confirm_settings->setStyleSheet("QPushButton { color: green;}");
    this->ui->but_confirm_settings->setText("Confirmed");
    this->ui->spin_straight_conc->setEnabled(1);
    this->ui->but_start_pump->setEnabled(1);
    this->ui->but_update_pump->setEnabled(1);
    this->ui->but_stop_pump->setEnabled(1);
    this->ui->spin_seg_time->setEnabled(1);
    this->ui->spin_start_conc->setEnabled(1);
    this->ui->spin_end_conc->setEnabled(1);
    this->ui->but_add_segment->setEnabled(1);
    this->ui->but_clear_segments->setEnabled(1);
    this->ui->table_segments->setEnabled(1);
    this->ui->but_start_protocol->setEnabled(1);
    this->ui->but_stop_protocol->setEnabled(1);
    this->ui->but_update_protocol->setEnabled(1);
    this->ui->but_confirm_settings->setDisabled(1);
    this->ui->spin_seg_time->setMaximum(30); // # 30 minutes maximum time; limit of pump pause?
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
    //this>port = None
    //this.pumps = None
    //this->cond_timer->stop();
    //this->record_cond=0;
    this->ui->but_confirm_settings->setEnabled(1);
    this->ui->but_confirm_settings->setStyleSheet(this->ui->but_reset_cond->styleSheet());
    this->ui->but_confirm_settings->setText("Confirm");
    this->ui->spin_straight_conc->setDisabled(1);
    this->ui->but_start_pump->setDisabled(1);
    this->ui->but_update_pump->setDisabled(1);
    this->ui->but_stop_pump->setDisabled(1);
    this->ui->spin_seg_time->setDisabled(1);
    this->ui->spin_start_conc->setDisabled(1);
    this->ui->spin_end_conc->setDisabled(1);
    this->ui->but_add_segment->setDisabled(1);
    this->ui->but_clear_segments->setDisabled(1);
    this->ui->table_segments->setDisabled(1);
    this->ui->but_start_protocol->setDisabled(1);
    this->ui->but_stop_protocol->setDisabled(1);
    this->ui->but_update_protocol->setDisabled(1);

}

