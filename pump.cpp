#include "pump.h"
#include <QDebug>

/*
 * Class for a "Pump" object that inherits from QObject. This handles the serial
 * interface with one of the two pumps in this design. It also performs all
 * the necessary conversions for converting desired flow rates from protocols, etc.
 */

Pump::Pump(QObject *parent)
    : QObject(parent), serial(new QSerialPort(this))
{
    connect(serial, &QSerialPort::readyRead, this, &Pump::handleReadyRead);
    connect(serial, &QSerialPort::errorOccurred, this, &Pump::handleError);
}

Pump::~Pump()
{
    if (serial->isOpen()) {
        serial->close();
    }
}

bool Pump::openPort(const QString &portName, qint32 baudRate)
{
    serial->setPortName(portName);
    serial->setBaudRate(baudRate);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (serial->open(QIODevice::ReadWrite)) {
        qDebug() << "Port opened:" << portName;
        return true;
    } else {
        emit errorOccurred(serial->errorString());
        return false;
    }
}

void Pump::closePort()
{
    if (serial->isOpen()) {
        serial->close();
        qDebug() << "Port closed.";
    }
}

bool Pump::sendCommand(const QByteArray &command)
{
    if (!serial->isOpen()) {
        emit errorOccurred("Serial port is not open.");
        return false;
    }

    QByteArray packet;
    packet.append(startByte);
    packet.append(command);
    packet.append(endByte);

    qint64 bytesWritten = serial->write(packet);
    if (bytesWritten == packet.size()) {
        return true;
    } else {
        emit errorOccurred("Failed to write to serial port.");
        return false;
    }
}

void Pump::handleReadyRead()
{
    buffer.append(serial->readAll());

    // Process complete packets if available
    while (true) {
        int startIndex = buffer.indexOf(startByte);
        int endIndex = buffer.indexOf(endByte, startIndex);

        if (startIndex != -1 && endIndex != -1 && endIndex > startIndex) {
            QByteArray packet = buffer.mid(startIndex + 1, endIndex - startIndex - 1);
            emit dataReceived(packet);
            buffer.remove(0, endIndex + 1);
        } else {
            break; // No complete packet
        }
    }
}

void Pump::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        emit errorOccurred(serial->errorString());
    }
}
