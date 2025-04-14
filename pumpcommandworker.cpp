// pumpcommandworker.cpp
#include "pumpcommandworker.h"
#include "pumpinterface.h"
#include <QDebug>

PumpCommandWorker::PumpCommandWorker(PumpInterface* interface, QObject* parent)
    : QObject(parent), pumpInterface(interface) {
    connect(pumpInterface, &PumpInterface::dataReceived,
            this, &PumpCommandWorker::onResponseReceived);
}

void PumpCommandWorker::enqueueCommand(const AddressedCommand& command) {
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

    AddressedCommand cmd = commandQueue.dequeue();
    qDebug() << "Worker sending command to " << cmd.name;      // << cmd.cmd <<cmd.value;
    //pumpInterface->sendToPump(cmd.name, cmd.cmd, cmd.value);
    emit pumpCommandReady(cmd.name, cmd.cmd, cmd.value);
    processing = true;
}

void PumpCommandWorker::onResponseReceived(const QString& response) {
    Q_UNUSED(response)
    processing = false;
    processNext();
}
