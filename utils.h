#ifndef UTILS_H
#define UTILS_H

#include <QVector>
#include <utility>  // for std::pair

namespace utils {

std::pair<double, double> findReasonableMinMax(
    const QVector<double>& data,
    double minAcceptable,
    double maxAcceptable);

}

#endif // UTILS_H
