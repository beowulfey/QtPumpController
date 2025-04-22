#include "condinterface.h"
#include <QDebug>
#include <QDateTime>

CondInterface::CondInterface(QObject* parent) : QObject(parent), serial(new QSerialPort(this)) {
    qDebug() << "Creating CondInterface";
    connect(serial, &QSerialPort::readyRead, this, &CondInterface::handleReadyRead);
    connect(serial, &QSerialPort::errorOccurred, this, &CondInterface::handleError);

    workerThread = new QThread;
    condWorker = new CondWorker(this, nullptr);
    condWorker->moveToThread(workerThread);
    workerThread->start();

    connect(this, &CondInterface::sendCommand, condWorker, &CondWorker::enqueueCommand, Qt::QueuedConnection);
    connect(condWorker, &CondWorker::condCommandReady, this, &CondInterface::handleCommand, Qt::QueuedConnection);

}

void CondInterface::shutdown() {
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
        delete workerThread;
        workerThread = nullptr;
    }
    // this is equivalent to closePort()
    if (serial->isOpen()) {
        serial->close();
    }
}

CondInterface::~CondInterface() {
    shutdown();
}

void CondInterface::handleCommand(const QString& cmd)
{
    sendToMeter(cmd);
}


bool CondInterface::connectToMeter(const QString &portName, qint32 baudRate) {
    if (serial->isOpen()) {
        serial->close();
    }

    serial->setPortName(portName);
    serial->setBaudRate(baudRate);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred("Failed to open port: " + serial->errorString());
        return false;
    }

    serial->setDataTerminalReady(true);
    serial->setRequestToSend(true);

    //qDebug() << "Meter port opened successfully:" << serial->portName();

    // Every time we connect, just confirm the datetime is set.
    //QDateTime now = QDateTime::currentDateTime();
    //QString command = QString("SETRTC %1-%2-%3-%4-%5-%6-3\r")
    //                      .arg(now.date().year(), 4, 10, QChar('0'))
    //                      .arg(now.date().month(), 2, 10, QChar('0'))
    //                      .arg(now.date().day(), 2, 10, QChar('0'))
    //                      .arg(now.time().hour(), 2, 10, QChar('0'))
    //                      .arg(now.time().minute(), 2, 10, QChar('0'))
    //                      .arg(now.time().second(), 2, 10, QChar('0'));
    //condWorker->enqueueCommand(command);
    return true;
}

void CondInterface::getMeasurement()
{
    //qDebug() << "CONDINTERFACE: Emitting measurement request";
    QString cmd = "GETMEAS\r";
    emit sendCommand(cmd);
}

bool CondInterface::sendToMeter(const QString &cmd)
{
    //qDebug() << "CondInterface sending to serial port";
    QByteArray packet = cmd.toUtf8();
    qint64 bytesWritten = serial->write(packet);
    return bytesWritten == packet.size();

}

void CondInterface::handleReadyRead() {
    serialBuffer.append(serial->readAll());

    while (true) {
        int endIndex = serialBuffer.indexOf('>');

        if (endIndex == -1)
            break; // No full response yet

        QByteArray frame = serialBuffer.left(endIndex + 1);  // Include '>'
        serialBuffer.remove(0, endIndex + 1);                // Remove from buffer

        QString response = QString::fromLatin1(frame).trimmed();
        //qDebug() << "Parsed response:" << response;

        if (response.contains("RTC updated")) {
            emit messageReceived(response);
        }
        else if (response.contains("Conductivity")) {
            //qDebug()<<"Measurement detected";
            QStringList fields = response.split(',');

            if (fields.size() >= 12) {
                CondReading reading;
                reading.value = fields[9].toDouble();         // e.g., "0.00"
                reading.units = fields[10].trimmed();          // e.g., "uS/cm"

                //qDebug() << "Conductivity reading:" << reading.value << reading.units;
                emit measurementReceived(reading);
            } else {
                emit errorOccurred("Malformed GETMEAS response");
            }
        }
        else {
            emit messageReceived("Unknown response: " + response);
        }
    }
}

void CondInterface::handleError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError)
        return;
    emit errorOccurred("Serial error: " + serial->errorString());
}
