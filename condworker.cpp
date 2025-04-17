#include "condworker.h"
#include "condinterface.h"
#include <QDebug>
#include <QApplication>

CondWorker::CondWorker(CondInterface* interface, QObject* parent)
    : QObject(parent), condInterface(interface) {
    connect(condInterface, &CondInterface::measurementReceived,
            this, &CondWorker::onResponseReceived);

}

void CondWorker::enqueueCommand(const QString &command) {
    //qDebug() << "enqueueCommand running in thread:" << QThread::currentThread();
    //qDebug() << "condWorker lives in thread:" << this->thread();
    //qDebug() << "Queued command: " << command;
    commandQueue.enqueue(command);
    if (!processing) {
        processNext();
    }
}

void CondWorker::processNext() {
   // qDebug() << "Processing next!";
    if (commandQueue.isEmpty()) {
      //  qDebug() << "nothing to process";
        processing = false;
        return;
    }
    QString cmd = commandQueue.dequeue();
    //qDebug()<<"Emitting command";
    emit condCommandReady(cmd);
    processing = true;
}

void CondWorker::onResponseReceived(CondReading response) {
    Q_UNUSED(response)
    processing = false;
    processNext();
}
