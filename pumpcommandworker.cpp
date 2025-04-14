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
        // emit message that queue is complete?
        processing = false;
        return;
    }

    AddressedCommand cmd = commandQueue.dequeue();
    emit pumpCommandReady(cmd.name, cmd.cmd, cmd.value);
    processing = true;
}

void PumpCommandWorker::onResponseReceived(const QString& response) {
    Q_UNUSED(response)
    processing = false;
    processNext();
}
