#ifndef CONDINTERFACE_H
#define CONDINTERFACE_H

#include <QObject>
#include <QSerialPort>
#include <QThread>

struct CondReading {
    double value = 0.0;
    QString units;
};

class CondWorker : public QObject {
    Q_OBJECT
public:
    explicit CondWorker(QObject* parent = nullptr);
    void setPort(QSerialPort* port);

public slots:
    void requestMeasurement();

signals:
    void finished(CondReading reading);
    void error(QString message);

private:
    QSerialPort* serialPort = nullptr;
};

class CondInterface : public QObject {
    Q_OBJECT
public:
    explicit CondInterface(QObject* parent = nullptr);
    ~CondInterface();

    void setSerialPort(QSerialPort* port);
    void getMeasurement();

signals:
    void goMeasure();
    void measurementReceived(CondReading reading);
    void errorOccurred(QString message);

private:
    QThread workerThread;
    CondWorker* worker = nullptr;
};

#endif // CONDINTERFACE_H
