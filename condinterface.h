#ifndef CONDINTERFACE_H
#define CONDINTERFACE_H

#include "condworker.h"
#include <QObject>
#include <QSerialPort>
#include <QThread>

// This particular conductivity meter (Thermo Orion Lab Star EC112) is not great and has very minimal USB connectivity.
// I can basically only call GETMEAS, which gets a measurement, so that's what I'll be programming!
// Originally, I had the software set the current time on connect; may add that back.

// Simple struct to hold a conductivity measurement



class CondInterface : public QObject {
    Q_OBJECT
public:
    explicit CondInterface(QObject* parent = nullptr);
    ~CondInterface();

    bool connectToMeter(const QString &portName, qint32 baudRate = QSerialPort::Baud9600);
    void getMeasurement();
    void shutdown();

public slots:
    void handleCommand(const QString &cmd);


signals:
    void messageReceived(const QString &data);
    void measurementReceived(CondReading reading);
    void errorOccurred(const QString &message);
    void sendCommand(const QString& cmd);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    bool sendToMeter(const QString &cmd);
    QThread *workerThread;
    CondWorker *condWorker;
    QSerialPort *serial;
    QByteArray serialBuffer;

};

#endif // CONDINTERFACE_H
