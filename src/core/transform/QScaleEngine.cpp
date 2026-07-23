// QGraphPlot — High-performance Qt chart library
//
// Copyright 2026 QGraphPlot Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "QScaleEngine.h"

#include <QtCore/QLoggingCategory>
#include <cmath>
#include <algorithm>

namespace qgraphplot {

namespace {
Q_LOGGING_CATEGORY(lcScaleEngine, "qgraphplot.scaleengine")

//! Helper function to compute "nice" numbers (Paul Heckbert algorithm).
double niceNum(double x, bool round) {
    if (x <= 0.0) {
        return 0.0;
    }
    const int expv = static_cast<int>(std::floor(std::log10(x)));
    const double f = x / std::pow(10.0, expv);
    double nf;
    if (round) {
        if (f < 1.5) nf = 1.0;
        else if (f < 3.0) nf = 2.0;
        else if (f < 7.0) nf = 5.0;
        else nf = 10.0;
    } else {
        if (f <= 1.0) nf = 1.0;
        else if (f <= 2.0) nf = 2.0;
        else if (f <= 5.0) nf = 5.0;
        else nf = 10.0;
    }
    return nf * std::pow(10.0, expv);
}
} // namespace

std::vector<QScaleEngine::TickInfo> QScaleEngine::calculateTicks(
    double min, double max, int targetTickCount, bool isLog)
{
    std::vector<TickInfo> ticks;

    // Validate and clamp input parameters (AI.md §3.3)
    if (targetTickCount <= 1) {
        targetTickCount = 2;
    }
    if (min > max) {
        std::swap(min, max);
    }
    if (std::isnan(min) || std::isnan(max) || std::isinf(min) || std::isinf(max)) {
        return ticks;
    }

    if (isLog) {
        if (min <= 0.0 || max <= 0.0) {
            qCWarning(lcScaleEngine) << "QScaleEngine: Log scale requires strictly positive range. Falling back to linear.";
            isLog = false;
        }
    }

    if (std::abs(max - min) < 1e-12) {
        ticks.push_back({min, formatLabel(min, 1.0)});
        return ticks;
    }

    if (isLog) {
        const double logMin = std::log10(min);
        const double logMax = std::log10(max);
        const double logRange = logMax - logMin;

        if (logRange >= 1.0) {
            // Span is at least one decade; ticks at powers of 10.
            const int startDecade = static_cast<int>(std::floor(logMin));
            const int endDecade = static_cast<int>(std::ceil(logMax));
            int step = 1;
            const int decadeCount = endDecade - startDecade;
            if (decadeCount > targetTickCount) {
                step = static_cast<int>(std::ceil(static_cast<double>(decadeCount) / targetTickCount));
            }

            for (int d = startDecade; d <= endDecade; d += step) {
                const double val = std::pow(10.0, d);
                if (val >= min && val <= max) {
                    ticks.push_back({val, formatLabel(val, val)});
                }
            }
        } else {
            // Span is less than one decade; treat log range linearly and project.
            const double rawStep = logRange / (targetTickCount - 1);
            const double step = niceNum(rawStep, true);
            if (step > 0.0) {
                const double tickStart = std::ceil(logMin / step) * step;
                const double tickEnd = std::floor(logMax / step) * step;

                for (double val = tickStart; val <= tickEnd + 0.5 * step; val += step) {
                    const double dataVal = std::pow(10.0, val);
                    if (dataVal >= min && dataVal <= max) {
                        ticks.push_back({dataVal, formatLabel(dataVal, dataVal - std::pow(10.0, val - step))});
                    }
                }
            }
        }
    } else {
        // Heckbert linear algorithm
        const double range = niceNum(max - min, false);
        const double step = niceNum(range / (targetTickCount - 1), true);

        if (step > 0.0) {
            const double graphMin = std::floor(min / step) * step;
            const double graphMax = std::ceil(max / step) * step;

            for (double val = graphMin; val <= graphMax + 0.5 * step; val += step) {
                if (val >= min && val <= max) {
                    ticks.push_back({val, formatLabel(val, step)});
                }
            }
        }
    }

    return ticks;
}

QString QScaleEngine::formatLabel(double value, double step) {
    if (std::abs(value) < 1e-15) {
        value = 0.0;
    }

    if (step <= 0.0) {
        return QString::number(value);
    }

    if (step >= 1.0) {
        if (std::abs(value) >= 1e6 || (std::abs(value) > 0.0 && std::abs(value) <= 1e-4)) {
            return QString::number(value, 'e', 2);
        }
        return QString::number(value, 'f', 0);
    }

    int precision = static_cast<int>(std::max(0.0, -std::floor(std::log10(step))));
    if (precision > 12) {
        precision = 12;
    }
    return QString::number(value, 'f', precision);
}

} // namespace qgraphplot
