#include "plotwidget.h"
#include "theming.h"

PlotWidget::PlotWidget(QWidget *parent)
    : QWidget(parent), _x(-100), yBot(0), yTop(100), runStart(0) {

    // Initialize the plot
    plot = new QCustomPlot(this);
    graph = plot->addGraph();

    QPen pen;
    pen.setWidth(2);
    pen.setColor(UiRed);
    graph->setPen(pen);

    plot->xAxis->setLabel("Time (min)");
    plot->yAxis->setLabel("Conc (mM)");

    plot->xAxis->setTickLabelFont(QFont("Arial", 12));
    plot->yAxis->setTickLabelFont(QFont("Arial", 12));


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

void PlotWidget::setYAxis(int pac, int pbc) {
    yBot = std::min(pac, pbc);
    yTop = std::max(pac, pbc);
    onChange();
}

void PlotWidget::setYlabel(QString label){
    plot->yAxis->setLabel(label);
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
    plot->clearItems();
    graph->setData(xData, yData);


    // Dynamic x/y range setup
    double xMin = 0.0, xMax = 10.0;
    //double yMin = 0.0, yMax = 1.0;

    if (!xData.isEmpty()) {
        auto xMinMax = std::minmax_element(xData.constBegin(), xData.constEnd());
        xMin = *xMinMax.first;
        xMax = *xMinMax.second;
    }

    // Add padding
    double xPadding = (xMax - xMin) * 0.05;
    double yPadding = (yTop - yBot) * 0.05;

    plot->xAxis->setRange(xMin - xPadding, xMax + xPadding);
    plot->yAxis->setRange(yBot - yPadding, yTop + yPadding);

    // Draw vertical line if needed

    if (_x >= 0) {
        QCPItemLine *vLine = new QCPItemLine(plot);
        QPen m_pen;
        m_pen.setWidth(2);
        m_pen.setColor(UiGreen);
        vLine->setPen(m_pen);
        vLine->start->setCoords(_x, yBot - yPadding);
        vLine->end->setCoords(_x, yTop + yPadding);

    }

    plot->replot();
}
