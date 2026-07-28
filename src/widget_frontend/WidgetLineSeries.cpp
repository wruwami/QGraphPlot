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

    // Map data points to pixel space and draw as a single polyline.
    const auto span = mdl->points(0, count - 1);
    QPolygonF poly;
    poly.reserve(static_cast<qsizetype>(span.size()));
    for (const QPointF& dp : span) {
        poly.append(transform.toPixel(dp));
    }
    painter->drawPolyline(poly);
}

}  // namespace qgraphplot
