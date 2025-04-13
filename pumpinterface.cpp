#include "pumpinterface.h"
#include <QDebug>
#include <QTimer>


/* Interface for the two New Era NE-1002X pumps
 * Example usage:
 *  pumpInterface->broadcastCommand(BasicCommand::Start);
 *  pumpInterface->sendToPump("Pump A", BasicCommand::SetFlowRate, 1.25);
 */





PumpInterface::PumpInterface(QObject *parent)
    : QObject(parent), serial(new QSerialPort(this)) {

    connect(serial, &QSerialPort::readyRead, this, &PumpInterface::handleReadyRead);
    connect(serial, &QSerialPort::errorOccurred, this, &PumpInterface::handleError);

    // Initialize pumps
    pumps = {
        {0, "PumpA"},
        {1, "PumpB"}
    };
    workerThread = new QThread;
    commandWorker = new PumpCommandWorker(this);
    commandWorker->moveToThread(workerThread);
    workerThread->start();

}

void PumpInterface::shutdown() {
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


PumpInterface::~PumpInterface() {
    shutdown();
}

bool PumpInterface::openPort(const QString &portName, qint32 baudRate) {
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

    qDebug() << "Port opened successfully:" << serial->portName();

    // Send initial commands (like GetVersion)
    //broadcastCommand(BasicCommand::GetVersion);
    return true;
}

void PumpInterface::closePort() {
    if (serial->isOpen()) {
        serial->close();
    }
}

/* Removed this "send to both" option -- too fast, and can't really use in threads
 * void PumpInterface::broadcastCommand(BasicCommand cmd, double value) {
    for (const Pump &pump : pumps) {
        sendCommand(pump.address, cmd, value);
        //QThread::msleep(50);
    }
}*/

void PumpInterface::sendToPump(const QString &name, BasicCommand cmd, double value) {
    for (const Pump &pump : pumps) {
        if (pump.name == name) {
            sendCommand(pump.address, cmd, value);
            return;
        }
    }
    emit errorOccurred("Pump with name " + name + " not found.");
}

bool PumpInterface::sendCommand(int addr, BasicCommand cmd, double value) {
    // Write the packet to the address of the pump
    if (!serial->isOpen()) {
        emit errorOccurred("Serial port not open.");
        return false;
    }

    QByteArray command = buildCommand(cmd, value);
    QByteArray packet;
    packet.append(static_cast<char>('0' + addr));
    packet.append(command);

    qint64 bytesWritten = serial->write(packet);
    qDebug() << "Sending to pump" << addr << ":" << packet << packet.toHex(' ');
    return bytesWritten == packet.size();
}

QByteArray PumpInterface::buildCommand(BasicCommand cmd, double value) {
    QByteArray payload;

    switch (cmd) {
    case BasicCommand::Start:
        payload = "RUN";
        break;
    case BasicCommand::Stop:
        payload = "STP";
        break;
    case BasicCommand::SetFlowRate:
        payload = QString("RAT%1").arg(value, 0, 'f', 2).toUtf8();
        break;
    case BasicCommand::SetDirection:
        payload = (value > 0) ? "DIR INF" : "DIR WDR";
        break;
    case BasicCommand::GetStatus:
        payload = "STATUS";
        break;
    case BasicCommand::GetVersion:
        payload = "VER";
        break;
    }

    payload.append('\r'); // Required carriage return
    return payload;
}

void PumpInterface::handleReadyRead() {
    serialBuffer.append(serial->readAll());

    while (true) {
        int startIndex = serialBuffer.indexOf(static_cast<char>(0x02));
        int endIndex = serialBuffer.indexOf(static_cast<char>(0x03), startIndex + 1);

        if (startIndex != -1 && endIndex != -1 && endIndex > startIndex) {
            // We found a complete frame
            QByteArray payload = serialBuffer.mid(startIndex + 1, endIndex - startIndex - 1);
            QString readable = QString::fromLatin1(payload);
            qDebug() << "Parsed payload:" << readable;
            emit dataReceived(readable);

            // Remove processed data from buffer
            serialBuffer.remove(0, endIndex + 1);
        } else {
            // Either no full frame yet, or partial data still arriving
            break;
        }
    }

    // Optionally print buffer contents during debugging
   // qDebug() << "Current buffer:" << serialBuffer.toHex(' ');
}


void PumpInterface::handleError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError)
        return;
    emit errorOccurred("Serial error: " + serial->errorString());
}
