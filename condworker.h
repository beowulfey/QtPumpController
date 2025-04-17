#ifndef CONDWORKER_H
#define CONDWORKER_H

#include <QObject>
#include <QQueue>
#include <QTime>

// Queues the commands used by CondInterface for sending to meter.
// Used exact same layout as PumpCommandWorker

class CondInterface;

struct CondReading {
    double value = 0.0;
    QString units;
    QTime timestamp;

    CondReading() = default;

    CondReading(double v, const QString& u, const QTime& t = QTime::currentTime())
        : value(v), units(u), timestamp(t) {}
};

class CondWorker : public QObject
{
    Q_OBJECT
public:
    explicit CondWorker(CondInterface* interface, QObject* parent = nullptr);

signals:
    void condCommandReady(QString cmd);


public slots:
    void enqueueCommand(const QString& cmd);

private slots:
    void onResponseReceived(CondReading response);

private:
    void processNext();

    QQueue<QString> commandQueue;
    CondInterface* condInterface;
    bool processing = false;
};

#endif // CONDWORKER_H
