#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QObject>

class Protocol : public QObject {
    Q_OBJECT

public:
    explicit Protocol(QObject* parent = nullptr);

    void setDt(double dt);
    double dt() const;

    const QVector<double>& xvals() const;
    void setXvals(const QVector<double>& x);

    const QVector<double>& yvals() const;
    void setYvals(const QVector<double>& y);

    void generate(const QVector<QVector<double>>& segs);
    void clear();
    const QVector<QVector<double>> shareSegments();

private:
    double timeStep;
    QVector<double> xValues;
    QVector<double> yValues;
    QVector<QVector<double>> segments;
};

#endif // PROTOCOL_H
