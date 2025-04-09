#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QVector>
#include "libs/qcustomplot/qcustomplot.h"

class PlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void setX(double currX);
    double x() const;

    void setYAxis(double bot, double top);

    void clearAxes();

    void setStart(double time);
    double getStart() const;
    void setStop();
    void setData(QVector<double> xVals, QVector<double> yVals);
    void appendData(double x, double y);
    void onChange();

private:
    QCustomPlot *plot;
    QCPGraph *graph;
    double _x;
    double ybot, ytop;
    double runStart;
    QVector<double> xData, yData;
};

#endif // PLOTWIDGET_H
