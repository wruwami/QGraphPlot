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

//! @file WidgetScatterSeries.h
//! @brief QPainter-based scatter series renderer for the Widget frontend.

#ifndef WIDGETSCATTERSERIES_H
#define WIDGETSCATTERSERIES_H

#include "series/QScatterSeries.h"
#include "transform/QCoordinateTransform.h"

class QPainter;

namespace qgraphplot
{

class QAbstractSeries;

//! @brief QPainter scatter renderer for the Widget frontend (AI.md §3.1 parity).
//!
//! Mirrors QmlScatterSeries behaviour using QPainter instead of QSGGeometry:
//! each model point is mapped to pixel space and drawn as a filled circle
//! (MarkerShape::Circle) or filled square (MarkerShape::Square) using
//! QPainter::drawEllipse / QPainter::fillRect.
//!
//! The static overload paintSeries() lets WidgetChartView render any
//! SeriesType::Scatter series (including plain QScatterSeries instances)
//! without requiring a WidgetScatterSeries wrapper, matching the
//! WidgetLineSeries pattern.
class WidgetScatterSeries : public QScatterSeries
{
    Q_OBJECT

public:
    explicit WidgetScatterSeries(QObject* parent = nullptr);
    ~WidgetScatterSeries() override = default;

    WidgetScatterSeries(const WidgetScatterSeries&) = delete;
    WidgetScatterSeries& operator=(const WidgetScatterSeries&) = delete;
    WidgetScatterSeries(WidgetScatterSeries&&) = delete;
    WidgetScatterSeries& operator=(WidgetScatterSeries&&) = delete;

    //! Render this series into @p painter using @p transform. Delegates to
    //! paintSeries(). No-op if !isVisible(), model is null, or no data points.
    void paint(QPainter* painter, const QCoordinateTransform& transform) const;

    //! Render any Scatter-type QAbstractSeries without creating a
    //! WidgetScatterSeries wrapper. WidgetChartView::paintAllSeries() calls
    //! this for every SeriesType::Scatter series (AI.md §3.1 parity with
    //! QmlScatterSeries). Internally downcasts to QScatterSeries* to read
    //! markerSize and markerShape; the caller guarantees series->type() ==
    //! SeriesType::Scatter.
    static void paintSeries(QPainter* painter,
                            const QCoordinateTransform& transform,
                            const QAbstractSeries* series);
};

}  // namespace qgraphplot

#endif  // WIDGETSCATTERSERIES_H
