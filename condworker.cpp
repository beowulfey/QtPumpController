#include "condworker.h"
#include "condinterface.h"
#include <QDebug>
#include <QApplication>

CondWorker::CondWorker(CondInterface* interface, QObject* parent)
    : QObject(parent), condInterface(interface) {

    connect(condInterface, &CondInterface::measurementReceived,
            this, &CondWorker::onResponseReceived);

}

void CondWorker::initialize() {
    timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(3000);

    connect(timeoutTimer, &QTimer::timeout, this, [this]() {
        qWarning() << "Command timed out!";
        processing = false;
        processNext();
    });
}

void CondWorker::enqueueCommand(const QString &command) {
    //qDebug() << "enqueueCommand running in thread:" << QThread::currentThread();
    //qDebug() << "condWorker lives in thread:" << this->thread();
    commandQueue.enqueue(command);
    if (commandQueue.length() > 1)
    {
        qWarning() << "CondWorker commands accumulating! Queue total: " << commandQueue.length();
    }
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
   // qDebug()<<"CondWorker returning command";
    emit condCommandReady(cmd);
    processing = true;
    timeoutTimer->start();
}

void CondWorker::onResponseReceived(CondReading response) {
    Q_UNUSED(response)
    timeoutTimer->stop();
    processing = false;
    processNext();
}
