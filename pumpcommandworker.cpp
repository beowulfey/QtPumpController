// pumpcommandworker.cpp
#include "pumpcommandworker.h"
#include <QDebug>

PumpCommandWorker::PumpCommandWorker(PumpInterface* interface, QObject* parent)
    : QObject(parent), pumpInterface(interface) {
    connect(pumpInterface, &PumpInterface::dataReceived,
            this, &PumpCommandWorker::onResponseReceived);
}

void PumpCommandWorker::enqueueCommand(const PumpCommand& command) {
    commandQueue.enqueue(command);
    if (!processing) {
        processNext();
    }
}

void PumpCommandWorker::processNext() {
    if (commandQueue.isEmpty()) {
        processing = false;
        return;
    }

    PumpCommand cmd = commandQueue.dequeue();
    qDebug() << "Sending command to pump" << cmd.address;
    pumpInterface->sendCommand(cmd.address, cmd.cmd, cmd.value);
    processing = true;
}

void PumpCommandWorker::onResponseReceived(const QString& response) {
    Q_UNUSED(response)
    processing = false;
    processNext();
}
