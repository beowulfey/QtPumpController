#include "condmeter.h"
#include <QDebug>

CondMeter::CondMeter(const QString &portName, double minConc, double maxConc, QObject *parent)
    : QObject(parent), serial(new QSerialPort(this)), minRead(0.0), maxRead(0.0), minConc(minConc), maxConc(maxConc), units("Units")
{
    if (!portName.isEmpty()) {
        serial->setPortName(portName);
        serial->setBaudRate(QSerialPort::Baud9600);
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);

        if (serial->open(QIODevice::ReadWrite)) {
            setup();
        } else {
            emit errorOccurred("Failed to open port: " + serial->errorString());
        }
    }
}

CondMeter::~CondMeter()
{
    if (serial->isOpen()) {
        serial->close();
    }
}

void CondMeter::reset()
{
    minRead = 0.0;
    maxRead = 0.0;
}

void CondMeter::setMin(double value, const QString &units)
{
    if (maxRead != 0.0 && value >= maxRead) {
        emit errorOccurred("ERROR -- MIN POINT GREATER THAN MAX POINT!");
        return;
    }

    if (this->units.isEmpty()) {
        this->units = units;
    } else if (this->units != units) {
        emit errorOccurred("ERROR -- UNITS DON'T MATCH OTHER SETPOINT!");
        return;
    }

    minRead = value;
}

void CondMeter::setMax(double value, const QString &units)
{
    if (minRead != 0.0 && value <= minRead) {
        emit errorOccurred("ERROR -- MAX POINT LESS THAN MIN POINT!");
        return;
    }

    if (this->units.isEmpty()) {
        this->units = units;
    } else if (this->units != units) {
        emit errorOccurred("ERROR -- UNITS DON'T MATCH OTHER SETPOINT!");
        return;
    }

    maxRead = value;
}

double CondMeter::convert(double reading) const
{
    return minConc + (reading - minRead) * ((maxConc - minConc) / (maxRead - minRead));
}

void CondMeter::setup()
{
    if (serial->isOpen()) {
        serial->write(QString("SETRTC %1\r\n")
                          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-3"))
                          .toUtf8());
        serial->waitForBytesWritten(1000);
    }
}

std::pair<QDateTime, std::pair<QString, QString>> CondMeter::getMeasurement()
{
    if (!serial->isOpen()) {
        emit errorOccurred("Port is not open.");
        return {};
    }

    serial->write("GETMEAS\r");
    if (serial->waitForReadyRead(2000)) {
        QByteArray data = serial->readAll();
        QList<QByteArray> reply = data.split(',');

        if (reply.size() >= 11) {
            QDateTime time = QDateTime::fromString(reply[4] + " " + reply[5], "MM-dd-yyyy HH:mm:ss");
            QString value = reply[9];
            QString unit = reply[10];

            if (minRead != 0.0 && maxRead != 0.0) {
                double converted = convert(value.toDouble());
                value = QString::number(converted, 'f', 2);
                unit = "mM";
            }

            return {time, {value, unit}};
        }
    }

    emit errorOccurred("Failed to read measurement.");
    return {};
}

void CondMeter::read()
{
    auto result = getMeasurement();
    if (result.first.isValid()) {
        emit measurement(result.first, result.second.first, result.second.second);
    }
}
