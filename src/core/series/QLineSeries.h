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

//! @file QLineSeries.h
//! @brief Concrete connected-polyline series (the core series type).
//!
//! QLineSeries is the core-side counterpart of the frontend line renderers
//! (QmlLineSeries in the QML frontend, WidgetLineSeries in the Widget
//! frontend). It owns no rendering knowledge — it is purely the property
//! contract inherited from QAbstractSeries (model, color, name, visibility,
//! lineWidth, dashPattern) plus the SeriesType identity.
//!
//! The frontends compose (rather than subclass) this object because they
//! must derive from a Qt view class (QQuickItem / QWidget) and QObject
//! diamond inheritance is forbidden. By delegating their Q_PROPERTY getters
//! and setters to a QLineSeries instance, validation, change-guarding and
//! NOTIFY signals live in exactly one place (QAbstractSeries), satisfying
//! AI.md §3.1 (parity) and §3.2 (no duplicated logic) — see #57.

#ifndef QGRAPHPLOT_LINESERIES_H
#define QGRAPHPLOT_LINESERIES_H

#include "../qgraphplot_global.h"
#include "QAbstractSeries.h"

namespace qgraphplot
{

//! @brief Connected-polyline series — the concrete Line variant of
//! QAbstractSeries.
//!
//! This is the single shipped concrete QAbstractSeries subclass in core.
//! All property behavior (defaults, validation, change-guarded signals)
//! comes from the base class; this class only contributes its
//! SeriesType identity so frontend dispatch (e.g. choosing the line
//! renderer) can switch on a real enum value instead of a dead one.
class QGRAPHPLOT_EXPORT QLineSeries : public QAbstractSeries
{
    Q_OBJECT

public:
    //! Constructs an empty line series. @p parent follows Qt ownership
    //! (RAII, AI.md §1.4). When used as a composition backend by a
    //! frontend series (QmlLineSeries), the frontend passes itself as
    //! parent so the core object is destroyed with its host.
    explicit QLineSeries(QObject* parent = nullptr);

    //! @return SeriesType::Line — what every frontend line renderer
    //! matches against.
    [[nodiscard]] SeriesType type() const override { return SeriesType::Line; }
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_LINESERIES_H
