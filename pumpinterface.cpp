#include "pumpinterface.h"
#include <QDebug>

PumpInterface::PumpInterface(QObject *parent) : QObject(parent), serial(new QSerialPort(this)) {
    connect(serial, &QSerialPort::readyRead, this, &PumpInterface::handleReadyRead);
    connect(serial, &QSerialPort::errorOccurred, this, &PumpInterface::handleError);
    // lamda for testing
    //connect(serial, &QSerialPort::readyRead, this, []() {
    //    qDebug() << "readyRead() fired!";
    //});
}

PumpInterface::~PumpInterface() {
    closePort();
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
    serial->setDataTerminalReady(true);
    serial->setRequestToSend(true);

    qDebug() << "Port open yet?" << serial->isOpen();

    if (!serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred("Failed to open port: " + serial->errorString());
        return false;
    }

    qDebug() << serial->portName();
    qDebug() << "Port open now?" << serial->isOpen();
    bool first = sendCommand(PumpB, BasicCommand::GetVersion);
    return true;
}

void PumpInterface::closePort() {
    if (serial->isOpen()) {
        serial->close();
    }
}

bool PumpInterface::sendCommand(int addr, BasicCommand cmd, double value) {
    if (!serial->isOpen()) {
        emit errorOccurred("Serial port not open.");
        return false;
    }

    QByteArray packet;
    packet.append(addr);     // raw address byte
    packet.append(buildCommand(cmd, value));    // ASCII + '\r'

    qDebug() << "Sending:" << packet;

    qint64 bytesWritten = serial->write(packet);
    qDebug() << "return" << bytesWritten;
    return bytesWritten == packet.size();
}

QByteArray PumpInterface::buildCommand(BasicCommand cmd, double value) {
    QByteArray payload;

    switch (cmd) {
    case BasicCommand::Start:
        payload = QString("RUN%1").arg(value, 0, 'f', 2).toUtf8();
        break;
    case BasicCommand::Stop:
        payload = "STP";
        break;
    case BasicCommand::SetFlowRate:
        payload = QString("RAT%1").arg(value, 0, 'f', 2).toUtf8();
        break;
    case BasicCommand::SetDirection:
        payload = (value > 0) ? "DIR:INF" : "DIR:WDR";
        break;
    case BasicCommand::GetStatus:
        payload = "STATUS";
        break;
    case BasicCommand::GetVersion:
        payload = "VER";
        break;
    }

    payload.append('\r'); // Required terminator
    return payload;

}

void PumpInterface::handleReadyRead() {
    QByteArray data = serial->readAll();
    qDebug() <<" Data detected" <<data;
    emit dataReceived(data);
}

void PumpInterface::handleError(QSerialPort::SerialPortError error) {
    if (error != QSerialPort::NoError) {
        emit errorOccurred(serial->errorString());
    }
}
