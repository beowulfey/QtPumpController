#ifndef PUMP_H
#define PUMP_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

class Pump : public QObject
{
    Q_OBJECT

public:
    explicit Pump(QObject *parent = nullptr);
    ~Pump();

    bool openPort(const QString &portName, qint32 baudRate = QSerialPort::Baud9600);
    void closePort();


    enum class Command {
        Start,
        Stop,
        SetFlowRate,
        SetDirection,
        GetStatus

    };
    Q_ENUM(Command)  // Enables use in signals/slots and metaobject system
    bool sendCommand(Command cmd, double value = 0.0);


signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *serial;
    const char startByte = 0x02;
    const char endByte = 0x03;
    QByteArray buffer;
    QByteArray buildCommand(Command cmd, double value);
};

#endif // PUMP_H
