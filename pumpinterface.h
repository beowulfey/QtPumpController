#ifndef PUMPINTERFACE_H
#define PUMPINTERFACE_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

enum class BasicCommand {
    Start,
    Stop,
    SetFlowRate,
    SetDirection,
    GetStatus,
    GetVersion
};

struct Pump {
    int address;
    QString name;
};

class PumpInterface : public QObject {
    Q_OBJECT

public:
    explicit PumpInterface(QObject *parent = nullptr);
    ~PumpInterface();

    bool openPort(const QString &portName, qint32 baudRate = QSerialPort::Baud19200);
    void closePort();

    void broadcastCommand(BasicCommand cmd, double value = 0.0);
    void sendToPump(const QString &name, BasicCommand cmd, double value = 0.0);

signals:
    void dataReceived(const QString &data);
    void errorOccurred(const QString &message);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QByteArray serialBuffer;
    QSerialPort *serial;
    QVector<Pump> pumps;

    QByteArray buildCommand(BasicCommand cmd, double value);
    bool sendCommand(int addr, BasicCommand cmd, double value);
};

#endif // PUMPINTERFACE_H
