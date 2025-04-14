#ifndef PUMPINTERFACE_H
#define PUMPINTERFACE_H


#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#include "pumpcommandworker.h"

#include "pumpcommands.h"

struct Pump {
    int address;
    QString name;
};

class PumpInterface : public QObject {
    Q_OBJECT

public:
    explicit PumpInterface(QObject *parent = nullptr);
    ~PumpInterface();

    bool connectToPumps(const QString &portName, qint32 baudRate = QSerialPort::Baud19200);           // initiates connections
    void broadcastCommand(PumpCommand cmd, QString value = 0);                                      // for basic stuff, like versions
    void sendToPump(const QString &name, PumpCommand cmd, QString value = 0);
    void shutdown();
    void setPhases(const QVector<QVector<PumpPhase>> &phases);

    bool startPumps(int phase);

public slots:
    void handlePumpCommand(const QString& name, PumpCommand cmd, QString value);


signals:
    void dataReceived(const QString &data);
    void errorOccurred(const QString &message);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QThread *workerThread;
    PumpCommandWorker *commandWorker;
    QByteArray serialBuffer;
    QSerialPort *serial;
    QVector<Pump> pumps;

    QByteArray buildCommand(PumpCommand cmd, QString value);
    bool sendCommand(int addr, PumpCommand cmd, QString value = 0);
};

#endif // PUMPINTERFACE_H
