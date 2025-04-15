#include "comsdialog.h"
#include <QSerialPortInfo>

COMsDialog::COMsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    combo_com_pump->addItem("None");
    combo_com_cond->addItem("None");
    //combo_com_pump->addItem("TEST");
    //combo_com_cond->addItem("TEST");

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        QString port = info.portName();
        combo_com_pump->addItem(port);
        combo_com_cond->addItem(port);
    }

    connect(combo_com_cond, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &COMsDialog::updatePump);
    connect(combo_com_pump, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &COMsDialog::updateCond);
}

void COMsDialog::updateCond()
{
    if (combo_com_pump->currentIndex() == combo_com_cond->currentIndex() && combo_com_pump->currentIndex() > 0) {
        combo_com_cond->setCurrentIndex(combo_com_cond->currentIndex() - 1);
    }
}

void COMsDialog::updatePump()
{
    if (combo_com_pump->currentIndex() == combo_com_cond->currentIndex() && combo_com_pump->currentIndex() > 0) {
        combo_com_pump->setCurrentIndex(combo_com_pump->currentIndex() - 1);
    }
}

void COMsDialog::accept()
{
    emit coms(combo_com_cond->currentText(), combo_com_pump->currentText());
    QDialog::accept();
}
