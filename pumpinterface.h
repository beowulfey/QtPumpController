#ifndef PUMPINTERFACE_H
#define PUMPINTERFACE_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

class PumpInterface : public QObject
{
    Q_OBJECT

public:
    explicit PumpInterface(QObject *parent = nullptr);
    ~PumpInterface();

    bool openPort(const QString &portName, qint32 baudRate = QSerialPort::Baud19200);
    void closePort();

    enum class BasicCommand {
        Start,
        Stop,
        SetFlowRate,      // Sets infusion rate
        SetVolume,
        SetDirection, // INF or WDR
        GetStatus,
        GetVersion
    };

    Q_ENUM(BasicCommand)  // Allows use with signals/slots and Qt Designer

    bool sendCommand(int addr, BasicCommand cmd, double value = 0.0);

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    int PumpA = 0;
    int PumpB = 1;
    QSerialPort *serial;
    const char startByte = 0x02;
    const char endByte = 0x03;

    QByteArray buildCommand(BasicCommand cmd, double value);
};

#endif // PUMPINTERFACE_H
