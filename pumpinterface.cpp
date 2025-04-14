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

void PumpInterface::handlePumpCommand(const QString& name, PumpCommand cmd, QString value) {
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

    //qDebug() << "Port opened successfully:" << serial->portName();

    // Send initial commands (like GetVersion)
    //emit dataReceived("Connecting to pumps! ");
    broadcastCommand(PumpCommand::GetVersion);
    broadcastCommand(PumpCommand::SetVolUnits); // hardcoded to uL
    return true;
}


void PumpInterface::broadcastCommand(PumpCommand cmd, QString value) {
    for (const Pump &pump : pumps) {
        AddressedCommand command;
        command.name = pump.name;
        command.cmd = cmd;
        command.value = value;
        commandWorker->enqueueCommand(command);
    }
}

void PumpInterface::sendToPump(const QString &name, PumpCommand cmd, QString value) {
    // Primary reference function -- used externally. Requires the name of the pump
    // (which is PumpA or PumpB)

    for (const Pump &pump : pumps) {
        if (pump.name == name) {
            //qDebug() << "sendToPump: #"<<pump.address;
            sendCommand(pump.address, cmd, value);
            return;
        }
    }
    emit errorOccurred("Pump with name " + name + " not found.");
}

void PumpInterface::setPhases(const QVector<QVector<PumpPhase>> &phases)
{
    QVector<PumpPhase> aPhases = phases.at(0);
    QVector<PumpPhase> bPhases = phases.at(1);

    QVector<AddressedCommand> cmdList;
    foreach (PumpPhase phase, aPhases)
    {
        AddressedCommand phaseNumber;
        phaseNumber.name = "PumpA";
        phaseNumber.cmd = PumpCommand::SetPhase;
        phaseNumber.value = QString::number(phase.phaseNumber);
        commandWorker->enqueueCommand(phaseNumber);

        if (phase.function == "RAT")
        {
            AddressedCommand phaseFunc;
            phaseFunc.name = "PumpA";
            phaseFunc.cmd = PumpCommand::RateFunction;

            AddressedCommand phaseRate;
            phaseRate.name = "PumpA";
            phaseRate.cmd = PumpCommand::SetFlowRate;
            phaseRate.value = QString::number(phase.rate);

            AddressedCommand phaseVol;
            phaseVol.name = "PumpA";
            phaseVol.cmd = PumpCommand::SetVolume;
            phaseVol.value = QString::number(phase.volume);

            AddressedCommand phaseDir;
            phaseDir.name = "PumpA";
            phaseDir.cmd = PumpCommand::SetFlowDirection;

            commandWorker->enqueueCommand(phaseFunc);
            commandWorker->enqueueCommand(phaseRate);
            commandWorker->enqueueCommand(phaseVol);
            commandWorker->enqueueCommand(phaseDir);
        }
        else if (phase.function == "LIN")
        {
       //     qDebug() << "RATE: " << phase.rate;
       //     qDebug() << "TIME: " << phase.time;
        }
        else if (phase.function == "PAUSE")
        {
            AddressedCommand phaseFunc;
            phaseFunc.name = "PumpA";
            phaseFunc.cmd = PumpCommand::PauseFunction;

            AddressedCommand phaseTime;
            phaseTime.name = "PumpA";
            phaseTime.cmd = PumpCommand::SetPause;
            phaseTime.value = phase.time;

            commandWorker->enqueueCommand(phaseFunc);
            commandWorker->enqueueCommand(phaseTime);
        }
        else if (phase.function == "STOP"){
            AddressedCommand stopPhase;
            stopPhase.name = "PumpA";
            stopPhase.cmd = PumpCommand::StopFunction;
            commandWorker->enqueueCommand(stopPhase);
        }
    }
    //qDebug() << "PUMP B CMDS!!!!";
    foreach (PumpPhase phase, bPhases)
    {
        AddressedCommand phaseNumber;
        phaseNumber.name = "PumpB";
        phaseNumber.cmd = PumpCommand::SetPhase;
        phaseNumber.value = QString::number(phase.phaseNumber);
        commandWorker->enqueueCommand(phaseNumber);

        if (phase.function == "RAT")
        {
            AddressedCommand phaseFunc;
            phaseFunc.name = "PumpB";
            phaseFunc.cmd = PumpCommand::RateFunction;

            AddressedCommand phaseRate;
            phaseRate.name = "PumpB";
            phaseRate.cmd = PumpCommand::SetFlowRate;
            phaseRate.value = QString::number(phase.rate);

            AddressedCommand phaseVol;
            phaseVol.name = "PumpB";
            phaseVol.cmd = PumpCommand::SetVolume;
            phaseVol.value = QString::number(phase.volume);

            AddressedCommand phaseDir;
            phaseDir.name = "PumpB";
            phaseDir.cmd = PumpCommand::SetFlowDirection;

            commandWorker->enqueueCommand(phaseFunc);
            commandWorker->enqueueCommand(phaseRate);
            commandWorker->enqueueCommand(phaseVol);
            commandWorker->enqueueCommand(phaseDir);
        }
        else if (phase.function == "LIN")
        {
          //  qDebug() << "RATE: " << phase.rate;
          //  qDebug() << "TIME: " << phase.time;
        }
        else if (phase.function == "PAUSE")
        {
            AddressedCommand phaseFunc;
            phaseFunc.name = "PumpB";
            phaseFunc.cmd = PumpCommand::PauseFunction;

            AddressedCommand phaseTime;
            phaseTime.name = "PumpB";
            phaseTime.cmd = PumpCommand::SetPause;
            phaseTime.value = phase.time;

            commandWorker->enqueueCommand(phaseFunc);
            commandWorker->enqueueCommand(phaseTime);
          //  qDebug() << "TIME: " << phase.time;
        }
        else if (phase.function == "STOP"){
            AddressedCommand stopPhase;
            stopPhase.name = "PumpB";
            stopPhase.cmd = PumpCommand::StopFunction;
            commandWorker->enqueueCommand(stopPhase);
        }
    }
}

bool PumpInterface::startPumps(int phase)
{
    if (!serial->isOpen()) {
        emit errorOccurred("Serial port not open.");
        return false;
    }
    QByteArray command = QString("RUN%1").arg(phase).toUtf8();
    QByteArray packet;
    packet.append(static_cast<char>('0')+0);
    packet.append(command);
    packet.append(static_cast<char>('*'));
    packet.append(static_cast<char>('0')+1);
    packet.append(command);
    packet.append(static_cast<char>('*'));
    packet.append('\r');


    qint64 bytesWritten = serial->write(packet);
    qDebug() << "Sending to pump: " << packet;
    return bytesWritten == packet.size();
}

// Private functions

bool PumpInterface::sendCommand(int addr, PumpCommand cmd, QString value) {
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
    qDebug() << "Sending to pump" << addr << ":" << packet;
    return bytesWritten == packet.size();
}

QByteArray PumpInterface::buildCommand(PumpCommand cmd, QString value) {
    // Base function. Look up the command in our enum, then convert to the
    // correct pump terminology
    QByteArray payload;

    switch (cmd) {
    case PumpCommand::Start:
        // RUN [phase] will start either the current phase or this number
        payload = QString("RUN%1").arg(value.toFloat(),0,'f', 0).toUtf8();
        break;
    case PumpCommand::Stop:
        payload = "STP";
        break;
    case PumpCommand::RateFunction:
        payload = QString("FUNRAT").toUtf8();
        break;
    case PumpCommand::SetFlowRate:
        payload = QString("RAT%1UM").arg(value.toFloat(), 0, 'f', 1).toUtf8();
        break;
    case PumpCommand::SetVolume:
        payload = QString("VOL%1").arg(value.toFloat(),0,'f',0).toUtf8();
        break;
    case PumpCommand::SetVolUnits:
        payload = "VOLUL";
        break;
    case PumpCommand::SetFlowDirection:
        // Any value given will set to withdraw
        payload = (value.toInt() > 0) ? "DIRWDR" : "DIRINF";
        break;
    case PumpCommand::SetPhase:
        payload = QString("PHN%1").arg(value.toInt()).toUtf8();
        break;
    case PumpCommand::RampFunction:
        payload = "FUNLIN";
        break;
    case PumpCommand::StopFunction:
        payload = "FUNSTP";
        break;
    case PumpCommand::PauseFunction:
        payload = "FUNPAS";
        break;
    case PumpCommand::SetPause:
        payload = QString("PAS%1)").arg(value.toInt()).toUtf8();
        break;
    case PumpCommand::SetRampTime:
        // will be in format (00:00), for HH:MM or SS:Tenths depending on phase
        payload = QString("TIM%1").arg(value).toUtf8();
        break;
    case PumpCommand::GetVersion:
        payload = "VER";
        break;
    }

    payload.append('\r'); // Required carriage return
    //qDebug() << "buildCommand built as: " << payload;
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
            qDebug() << "Parsed response:" << readable;
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
