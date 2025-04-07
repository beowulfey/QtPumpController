#include "plotwidget.h"

PlotWidget::PlotWidget(QWidget *parent)
    : QWidget(parent), _x(0), ybot(0), ytop(10), runStart(0) {

    // Initialize the plot
    plot = new QCustomPlot(this);
    graph = plot->addGraph();

    graph->setPen(QPen(QColor(222,101, 94))); // Line color

    plot->xAxis->setLabel("Time (min)");
    plot->yAxis->setLabel("Conc (mM)");

    plot->xAxis->setTickLabelFont(QFont("Arial", 9));
    plot->yAxis->setTickLabelFont(QFont("Arial", 9));

    // Set layout
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(plot);
    setLayout(layout);

    onChange();
}

void PlotWidget::setX(double currX) {
    _x = currX;
    onChange();
}

double PlotWidget::x() const {
    return _x;
}

void PlotWidget::setYAxis(double bot, double top) {
    ybot = bot;
    ytop = top;
    onChange();
}

void PlotWidget::clearAxes() {
    plot->clearGraphs();
    graph = plot->addGraph();
    graph->setPen(QPen(QColor(222,101, 94)));
    plot->xAxis->setLabel("");
    plot->yAxis->setLabel("");
    plot->replot();
}

void PlotWidget::setStart(double time) {
    runStart = time;
}

double PlotWidget::getStart() const {
    return runStart;
}

void PlotWidget::setStop() {
    runStart = 0;
}

void PlotWidget::setData(QVector<double> xVals, QVector<double> yVals) {
    xData = xVals;
    yData = yVals;
    onChange();
}

void PlotWidget::appendData(double x, double y) {
    xData.append(x);
    yData.append(y);
    onChange();
}

void PlotWidget::onChange() {
    graph->setData(xData, yData);

    if (_x > 0) {
        QCPItemLine *vLine = new QCPItemLine(plot);
        vLine->start->setCoords(_x, ybot);
        vLine->end->setCoords(_x, ytop);
        vLine->setPen(QPen(QColor(222,101, 94)));
    }

    plot->xAxis->setRange(0, xData.isEmpty() ? 10 : xData.last());
    plot->yAxis->setRange(ybot, ytop);

    plot->replot();
}
