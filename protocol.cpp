#include "protocol.h"


Protocol::Protocol(QObject* parent)
    : QObject(parent), timeStep(0.5) {
    generate({});
}

void Protocol::setDt(double dt) {
    timeStep = dt;
    generate(segments);
}

double Protocol::dt() const {
    // interval in seconds, default in constructor is 0.5 seconds
    return timeStep;
}

const QVector<double>& Protocol::xvals() const {
    return xValues;
}

void Protocol::setXvals(const QVector<double>& x) {
    xValues = x;
}

const QVector<double>& Protocol::yvals() const {
    return yValues;
}

void Protocol::setYvals(const QVector<double>& y) {
    yValues = y;
}

const QVector<QVector<double>> Protocol::shareSegments()
{
    return segments;
}

void Protocol::generate(const QVector<QVector<double>>& segs) {
    if (!segs.isEmpty()) {
        segments = segs;
        QVector<double> xtot;
        QVector<double> ytot;
        double totalTime = 0.0;

        for (const auto& seg : segs) {
            if (seg.size() != 3) continue;  // Expecting [duration, start, end]

            double duration = seg[0];
            double start = seg[1];
            double end = seg[2];

            QVector<double> x = { totalTime, totalTime + duration };
            QVector<double> y = { start, end };

            int steps = static_cast<int>((duration * 60.0) / timeStep);
            QVector<double> xExpanded(steps + 1);
            for (int i = 0; i <= steps; ++i) {
                xExpanded[i] = x[0] + i * (x[1] - x[0]) / steps;
            }

            QVector<double> yExpanded(steps + 1);
            for (int i = 0; i <= steps; ++i) {
                double t = xExpanded[i];
                yExpanded[i] = start + ((t - x[0]) / (x[1] - x[0])) * (end - start);
            }

            xtot.append(xExpanded);
            ytot.append(yExpanded);

            totalTime += duration;
        }

        xValues = xtot;
        yValues = ytot;
    }
}

void Protocol::clear() {
    xValues.clear();
    yValues.clear();
    segments.clear();
}
