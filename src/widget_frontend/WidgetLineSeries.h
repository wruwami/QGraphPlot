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

//! @file WidgetLineSeries.h
//! @brief QPainter-based line series renderer for the Widget frontend (Phase 1).

#ifndef WIDGETLINESERIES_H
#define WIDGETLINESERIES_H

#include "series/QLineSeries.h"
#include "transform/QCoordinateTransform.h"

class QPainter;

namespace qgraphplot
{

class QAbstractSeries;

//! @brief QPainter line renderer for the Widget frontend (Phase 1, issue #70).
//!
//! Extends QLineSeries with a paint() method that maps the core series
//! properties (color, lineWidth, dashPattern, opacity) to a QPen and draws a
//! polyline through all model points — the same mapping the QML scene-graph
//! renderer uses (AI.md §3.1 parity).
//!
//! The static overload paintSeries() lets WidgetChartView render any
//! SeriesType::Line series (including plain QLineSeries instances) without
//! requiring a WidgetLineSeries wrapper, preserving backward compatibility.
class WidgetLineSeries : public QLineSeries
{
    Q_OBJECT

public:
    explicit WidgetLineSeries(QObject* parent = nullptr);
    ~WidgetLineSeries() override = default;

    WidgetLineSeries(const WidgetLineSeries&) = delete;
    WidgetLineSeries& operator=(const WidgetLineSeries&) = delete;
    WidgetLineSeries(WidgetLineSeries&&) = delete;
    WidgetLineSeries& operator=(WidgetLineSeries&&) = delete;

    //! Render this series into @p painter using @p transform. Delegates to
    //! paintSeries(). No-op if !isVisible(), model is null, or no data points.
    //! Caller is responsible for setting a clip rect that confines drawing to
    //! the plot area.
    void paint(QPainter* painter, const QCoordinateTransform& transform) const;

    //! Render any Line-type QAbstractSeries without creating a WidgetLineSeries
    //! wrapper. WidgetChartView::paintEvent calls this for every
    //! SeriesType::Line series so that plain QLineSeries instances work too.
    static void paintSeries(QPainter* painter,
                            const QCoordinateTransform& transform,
                            const QAbstractSeries* series);
};

}  // namespace qgraphplot

#endif  // WIDGETLINESERIES_H
