#include "utils.h"
#include <limits>
#include <algorithm>

namespace utils {

std::pair<double, double> findReasonableMinMax(
    const QVector<double>& data,
    double minAcceptable,
    double maxAcceptable)
{
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();

    for (double val : data) {
        if (val >= minAcceptable && val <= maxAcceptable) {
            min = std::min(min, val);
            max = std::max(max, val);
        }
    }

    if (min > max) return {minAcceptable, maxAcceptable};
    return {min, max};
}

}
