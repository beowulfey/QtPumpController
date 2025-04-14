#include "pumpinterface.h"
#include <QDebug>
#include <QTimer>


/* Interface for the two New Era NE-1002X pumps
 * Example usage:
 *  pumpInterface->broadcastCommand(PumpCommand::Start);
 *  pumpInterface->sendToPump("Pump A", PumpCommand::SetFlowRate, 1.25);
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
    connect(commandWorker, &PumpCommandWorker::pumpCommandReady,
            this, &PumpInterface::handlePumpCommand,
            Qt::QueuedConnection);  // <- critical!



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

void PumpInterface::handlePumpCommand(const QString& name, PumpCommand cmd, double value) {
    sendToPump(name, cmd, value);
}

bool PumpInterface::connectToPumps(const QString &portName, qint32 baudRate) {
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
    //emit dataReceived("Connecting to pumps! ");
    broadcastCommand(PumpCommand::GetVersion);
    return true;
}


void PumpInterface::broadcastCommand(PumpCommand cmd, double value) {
    for (const Pump &pump : pumps) {
        AddressedCommand command;
        command.name = pump.name;
        command.cmd = cmd;
        command.value = value;
        commandWorker->enqueueCommand(command);
    }
}

void PumpInterface::sendToPump(const QString &name, PumpCommand cmd, double value) {
    // Primary reference function -- used externally. Requires the name of the pump
    // (which is PumpA or PumpB)

    for (const Pump &pump : pumps) {
        if (pump.name == name) {
            sendCommand(pump.address, cmd, value);
            return;
        }
    }
    emit errorOccurred("Pump with name " + name + " not found.");
}


// Private functions

bool PumpInterface::sendCommand(int addr, PumpCommand cmd, double value) {
    // Internal function, not for public use.
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

QByteArray PumpInterface::buildCommand(PumpCommand cmd, double value) {
    // Base function. Look up the command in our enum, then convert to the
    // correct pump terminology
    QByteArray payload;

    switch (cmd) {
    case PumpCommand::Start:
        // RUN [phase] will start either the current phase or this number
        payload = QString("RUN%1").arg(value,0,'f', 0).toUtf8();
        break;
    case PumpCommand::Stop:
        payload = "STP";
        break;
    case PumpCommand::SetFlowRate:
        payload = QString("FUNRAT%1").arg(value, 0, 'f', 2).toUtf8();
        break;
    case PumpCommand::SetFlowDirection:
        // Any value given will set to withdraw
        payload = (value > 0) ? "DIRWDR" : "DIRINF";
        break;
    case PumpCommand::SetPhase:
        payload = QString("PHN%1").arg(value,0, 'f', 0).toUtf8();
        break;
    case PumpCommand::GetVersion:
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
