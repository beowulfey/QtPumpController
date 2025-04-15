#include "condinterface.h"
#include <QStringList>

CondWorker::CondWorker(QObject* parent) : QObject(parent) {}

void CondWorker::setPort(QSerialPort* port) {
    serialPort = port;
}

void CondWorker::requestMeasurement() {
    if (!serialPort || !serialPort->isOpen()) {
        emit error("Serial port not open");
        return;
    }

    serialPort->write("GETMEAS\r"); // send command
    if (!serialPort->waitForBytesWritten(100)) {
        emit error("Write timeout");
        return;
    }

    if (!serialPort->waitForReadyRead(100)) {
        emit error("Read timeout");
        return;
    }

    QByteArray responseData;
    while (serialPort->waitForReadyRead(50))
        responseData.append(serialPort->readAll());

    QStringList fields = QString(responseData).split(',');

    if (fields.size() >= 11) {
        CondReading reading;
        reading.value = fields[10].toDouble();  // 0.00
        reading.units = fields[11].trimmed();   // uS/cm
        emit finished(reading);
    } else {
        emit error("Unexpected response format");
    }
}

CondInterface::CondInterface(QObject* parent) : QObject(parent) {
    worker = new CondWorker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &CondInterface::goMeasure, worker, &CondWorker::requestMeasurement);
    connect(worker, &CondWorker::finished, this, &CondInterface::measurementReceived);
    connect(worker, &CondWorker::error, this, &CondInterface::errorOccurred);

    workerThread.start();
}

CondInterface::~CondInterface() {
    workerThread.quit();
    workerThread.wait();
}

void CondInterface::setSerialPort(QSerialPort* port) {
    if (worker) {
        worker->setPort(port);
    }
}

void CondInterface::getMeasurement() {
    emit goMeasure(); // signals the worker
}
