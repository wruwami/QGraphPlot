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

#include "QAutoScaler.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QLoggingCategory>

#include "../model/QAbstractSeriesModel.h"
#include "../series/QAbstractSeries.h"

namespace qgraphplot
{

namespace
{
Q_LOGGING_CATEGORY(lcAutoScaler, "qgraphplot.autoscaler")

//! Minimum half-expansion applied to a zero-width/height axis so that the
//! returned rect always has strictly positive dimensions (min < max).
constexpr double kDegenerateHalfSpan = 1.0;

//! Expands a single axis by @p halfSpan on each side of @p center, returning
//! {min, max}. The expansion is symmetric so the data stays centered.
void expandAxis(double center, double halfSpan, double& outMin, double& outMax) noexcept
{
    outMin = center - halfSpan;
    outMax = center + halfSpan;
}
}  // namespace

QRectF QAutoScaler::computePaddedBounds(const QList<QAbstractSeries*>& series, double paddingRatio)
{
    // Validate the padding ratio (AI.md §3.3 — never silently accept garbage).
    double ratio = paddingRatio;
    if (!std::isfinite(ratio) || ratio < 0.0) {
        qCWarning(lcAutoScaler) << "QAutoScaler: clamping invalid padding ratio to 0.0:"
                                << paddingRatio;
        ratio = 0.0;
    }

    // Union the bounds of every series whose model reports a usable rect.
    // An EMPTY model (pointCount()==0) returns a default-constructed QRectF
    // and is skipped. A NON-empty model is kept even if its bounds rect is
    // zero-area: a single point yields QRectF(x, y, 0, 0), and a constant-x
    // or constant-y series yields a zero-width / zero-height rect. Those
    // degenerate axes are expanded below, so the bounds' (x, y) origin is
    // still meaningful data we must union in.
    bool hasData = false;
    QRectF unioned;
    for (QAbstractSeries* s : series) {
        if (!s) {
            continue;
        }
        QAbstractSeriesModel* model = s->model();
        if (!model || model->pointCount() == 0) {
            continue;
        }
        const QRectF b = model->bounds();
        if (!hasData) {
            unioned = b;
            hasData = true;
        } else {
            unioned |= b;
        }
    }

    if (!hasData) {
        // No usable series: return a sane finite fallback so the view's
        // coordinate transform never sees a zero-area data rect.
        return QRectF(0.0, 0.0, 1.0, 1.0);
    }

    // Apply padding per axis. A zero-span axis (single point, or all points
    // share a coordinate) is expanded by a unit on each side regardless of
    // the ratio, since padding * 0 == 0.
    const double xSpan = unioned.width();
    const double ySpan = unioned.height();
    const double xHalf = (xSpan > 0.0) ? ratio * xSpan : kDegenerateHalfSpan;
    const double yHalf = (ySpan > 0.0) ? ratio * ySpan : kDegenerateHalfSpan;

    double xMin, xMax, yMin, yMax;
    if (xSpan > 0.0) {
        xMin = unioned.left() - xHalf;
        xMax = unioned.right() + xHalf;
    } else {
        // Single x coordinate: center on it.
        expandAxis(unioned.left(), xHalf, xMin, xMax);
    }
    if (ySpan > 0.0) {
        yMin = unioned.top() - yHalf;
        yMax = unioned.bottom() + yHalf;
    } else {
        expandAxis(unioned.top(), yHalf, yMin, yMax);
    }

    return QRectF(xMin, yMin, xMax - xMin, yMax - yMin);
}

}  // namespace qgraphplot
