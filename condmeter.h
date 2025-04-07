#ifndef CONDMETER_H
#define CONDMETER_H

#include <QObject>
#include <QSerialPort>
#include <QDateTime>

class CondMeter : public QObject
{
    Q_OBJECT

public:
    explicit CondMeter(const QString &portName = "", double minConc = 0.0, double maxConc = 100.0, QObject *parent = nullptr);
    ~CondMeter();

    void reset();
    void setMin(double value, const QString &units);
    void setMax(double value, const QString &units);
    void read();

signals:
    void measurement(const QDateTime &time, const QString &value, const QString &units);
    void errorOccurred(const QString &error);

private:
    void setup();
    std::pair<QDateTime, std::pair<QString, QString>> getMeasurement();
    double convert(double reading) const;

    QSerialPort *serial;
    double minRead;
    double maxRead;
    double minConc;
    double maxConc;
    QString units;
};

#endif // CONDMETER_H
