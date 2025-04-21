#include "plotwidget.h"
#include "theming.h"

PlotWidget::PlotWidget(QWidget *parent)
    : QWidget(parent), _x(-100), yBot(0), yTop(100), runStart(0) {

    // Initialize the plot
    plot = new QCustomPlot(this);
    graph = plot->addGraph();
    graph->setLineStyle(QCPGraph::lsStepCenter);

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
    setYAxis(0,1);

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
    qDebug() << "updating Yaxes " << pac << " "<< pbc;
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
    // I was getting weird jagged lines when two consecutive X vals were the same
    // This inserts some fuzzy values into X so they aren't the same
    QVector<double> adjustedX;
    adjustedX.reserve(xVals.size());

    double lastX = -std::numeric_limits<double>::infinity();

    for (double x : xVals) {
        if (x <= lastX) {
            // Add a small random epsilon between 0 and 0.001
            double epsilon = QRandomGenerator::global()->bounded(0.001);
            x = lastX + epsilon;
        }
        adjustedX.append(x);
        lastX = x;
    }

    xData = adjustedX;
    yData = yVals;
    onChange();
}


QVector<QVector<double>> PlotWidget::getData()
{
    QVector<QVector<double>> result;
    result.append(xData);
    result.append(yData);
    return result;
}

void PlotWidget::appendData(double x, double y) {
    xData.append(x);
    yData.append(y);
    onChange();
}

void PlotWidget::onChange() {
    plot->clearItems();
    graph->setData(xData, yData);
    qDebug() << "Updating data for plot; min/maxY is " << yBot << yTop;


    // Dynamic x/y range setup
    double xMin = 0.0, xMax = 10.0;

    double yMin = yBot, yMax = yTop;
    if (yTop == yBot == 0)
    {
        if (!yData.isEmpty()) {
            auto yMinMax = std::minmax_element(yData.constBegin(), yData.constEnd());
            yMin = *yMinMax.first;
            yMax = *yMinMax.second;
        }
    }

    if (!xData.isEmpty()) {
        auto xMinMax = std::minmax_element(xData.constBegin(), xData.constEnd());
        xMin = *xMinMax.first;
        xMax = *xMinMax.second;
    }


    // Add padding
    double xPadding = (xMax - xMin) * 0.05;
    double yPadding = (yMax - yMin) * 0.05;

    plot->xAxis->setRange(xMin - xPadding, xMax + xPadding);
    plot->yAxis->setRange(yMin - yPadding, yMax + yPadding);

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
