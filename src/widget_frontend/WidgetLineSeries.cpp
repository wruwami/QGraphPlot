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

//! @file WidgetLineSeries.cpp

#include "WidgetLineSeries.h"

#include <algorithm>

#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPolygonF>

#include "model/QAbstractSeriesModel.h"
#include "series/QAbstractSeries.h"

namespace qgraphplot
{

WidgetLineSeries::WidgetLineSeries(QObject* parent) : QLineSeries(parent) {}

void WidgetLineSeries::paint(QPainter* painter, const QCoordinateTransform& transform) const
{
    paintSeries(painter, transform, this);
}

void WidgetLineSeries::paintSeries(QPainter* painter,
                                   const QCoordinateTransform& transform,
                                   const QAbstractSeries* series)
{
    if (!painter || !series) {
        return;
    }
    if (!series->isVisible()) {
        return;
    }
    auto* mdl = series->model();
    if (!mdl) {
        return;
    }
    const qsizetype count = mdl->pointCount();
    if (count == 0) {
        return;
    }

    // Visible-range culling (issue #90): skip the entire series when its
    // bounding box lies wholly outside the viewport's X range.  bounds() is
    // O(1) (cached by the model), so this is a near-free fast path for
    // off-screen series in dashboards with many channels.
    const QRectF db = transform.dataBounds();
    const QRectF mb = mdl->bounds();
    if (mb.right() < db.left() || mb.left() > db.right()) {
        return;
    }

    // Fold opacity into the color's alpha channel — same contract as the QML
    // scene-graph renderer (issue #71, AI.md §3.1 parity).
    QColor c = series->color();
    c.setAlphaF(c.alphaF() * series->opacity());

    QPen pen(c);
    pen.setWidthF(series->lineWidth());
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    const QList<qreal>& dash = series->dashPattern();
    if (!dash.isEmpty()) {
        pen.setStyle(Qt::CustomDashLine);
        pen.setDashPattern(dash);
    }
    painter->setPen(pen);

    // Retrieve the full point span, then narrow it to the visible X window.
    const auto span = mdl->points(0, count - 1);
    const QPointF* itFirst = span.data();
    const QPointF* itLast = span.data() + span.size();

    // Sub-span binary search (issue #90): for X-monotone-ascending data (the
    // standard layout for time series), find the visible subrange in O(log N)
    // and extend one point beyond each edge so that line segments crossing
    // the viewport boundary are drawn correctly.
    // Monotonicity guard: span[0] must hold the model's x-minimum and
    // span[N-1] must hold the model's x-maximum (both O(1) from the cached
    // mb already computed above).  This rules out non-monotone datasets where
    // the endpoint heuristic span[0].x() <= span[N-1].x() would pass but the
    // interior is not sorted, which would cause binary search to omit segments.
    if (count >= 2 && span[0].x() == mb.left() && span[count - 1].x() == mb.right()) {
        const double xMin = db.left();
        const double xMax = db.right();
        auto lo = std::lower_bound(
            itFirst, itLast, xMin, [](const QPointF& p, double x) { return p.x() < x; });
        auto hi = std::upper_bound(
            itFirst, itLast, xMax, [](double x, const QPointF& p) { return x < p.x(); });
        if (lo != itFirst) {
            --lo;
        }
        if (hi != itLast) {
            ++hi;
        }
        itFirst = lo;
        itLast = hi;
    }

    // TODO(issue #90 / #68): pixel-column min/max decimation.
    // When (itLast - itFirst) >> transform.pixelRect().width(), reduce vertex
    // count via one-pass min/max decimation: for each pixel column (mapping
    // back to an X data interval), emit (xLeft, yMin) and (xLeft, yMax).
    // This preserves the waveform envelope at O(n), matching QCustomPlot's
    // adaptive sampling and the standard instrument/oscilloscope technique.

    QPolygonF poly;
    poly.reserve(static_cast<qsizetype>(itLast - itFirst));
    for (const QPointF* it = itFirst; it != itLast; ++it) {
        poly.append(transform.toPixel(*it));
    }
    if (!poly.isEmpty()) {
        painter->drawPolyline(poly);
    }
}

}  // namespace qgraphplot
