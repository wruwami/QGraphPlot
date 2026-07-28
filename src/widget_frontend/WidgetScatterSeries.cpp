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

//! @file WidgetScatterSeries.cpp

#include "WidgetScatterSeries.h"

#include <QtGui/QPainter>

#include "model/QAbstractSeriesModel.h"
#include "series/QAbstractSeries.h"
#include "series/QScatterSeries.h"

namespace qgraphplot
{

WidgetScatterSeries::WidgetScatterSeries(QObject* parent) : QScatterSeries(parent) {}

void WidgetScatterSeries::paint(QPainter* painter, const QCoordinateTransform& transform) const
{
    paintSeries(painter, transform, this);
}

void WidgetScatterSeries::paintSeries(QPainter* painter,
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

    // Caller guarantees series->type() == SeriesType::Scatter.
    const auto* scatter = static_cast<const QScatterSeries*>(series);
    const double markerSize = scatter->markerSize();
    const MarkerShape markerShape = scatter->markerShape();

    // Fold opacity into the color's alpha channel — same contract as
    // QmlScatterSeries and WidgetLineSeries (issue #71, AI.md §3.1 parity).
    QColor color = series->color();
    color.setAlphaF(color.alphaF() * series->opacity());

    // Filled markers with no outline, matching QmlScatterSeries's flat-color
    // QSGFlatColorMaterial (no border line).
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);

    const double r = markerSize * 0.5;
    const auto span = mdl->points(0, count - 1);

    if (markerShape == MarkerShape::Circle) {
        for (const QPointF& dp : span) {
            const QPointF px = transform.toPixel(dp);
            painter->drawEllipse(QRectF(px.x() - r, px.y() - r, markerSize, markerSize));
        }
    } else {
        // Square
        for (const QPointF& dp : span) {
            const QPointF px = transform.toPixel(dp);
            painter->fillRect(QRectF(px.x() - r, px.y() - r, markerSize, markerSize), color);
        }
    }
}

}  // namespace qgraphplot
