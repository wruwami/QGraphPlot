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

//! @file QScatterSeries.h
//! @brief Concrete discrete-marker series (Scatter variant).
//!
//! QScatterSeries is the core-side counterpart of the frontend scatter
//! renderers (QmlScatterSeries in the QML frontend). Like QLineSeries it
//! inherits the shared property contract from QAbstractSeries (model,
//! color, name, visibility, opacity) and contributes its SeriesType
//! identity. In addition it owns two scatter-specific properties —
//! markerSize and markerShape — that the line series does not need.
//!
//! Keeping markerSize/markerShape on this concrete subclass (rather than
//! on QAbstractSeries) follows the QLineSeries precedent ("the concrete
//! subclass contributes only what its type needs") and keeps unrelated
//! properties off Line/Area/Bar (AI.md §3.2 — no dead properties). If a
//! future type also needs markers, the properties can be promoted to a
//! shared mixin or the base at that point — see #67.

#ifndef QGRAPHPLOT_SCATTERSERIES_H
#define QGRAPHPLOT_SCATTERSERIES_H

#include "../qgraphplot_global.h"
#include "QAbstractSeries.h"

namespace qgraphplot
{

//! Marker geometry used by scatter renderers. Defined here (AI.md §3.2 —
//! no magic numbers/strings duplicated in frontends) so QML and Widget
//! renderers switch on the same enum. New shapes add a value here and
//! consume it via the enum.
enum class MarkerShape : unsigned char
{
    Circle,  //!< Disc (approximated as a triangle fan)
    Square,  //!< Axis-aligned square (two triangles)
};

//! @brief Discrete-marker series — the concrete Scatter variant of
//! QAbstractSeries.
//!
//! Shared property behavior (defaults, validation, change-guarded signals)
//! comes from the base class. This class adds markerSize and markerShape
//! with the same validation idiom, plus its SeriesType identity so
//! frontend dispatch can switch on a real enum value.
class QGRAPHPLOT_EXPORT QScatterSeries : public QAbstractSeries
{
    Q_OBJECT
    Q_PROPERTY(double markerSize READ markerSize WRITE setMarkerSize NOTIFY markerSizeChanged)
    Q_PROPERTY(
        MarkerShape markerShape READ markerShape WRITE setMarkerShape NOTIFY markerShapeChanged)
    Q_ENUM(MarkerShape)

public:
    //! Constructs an empty scatter series. @p parent follows Qt ownership
    //! (RAII, AI.md §1.4). When used as a composition backend by a
    //! frontend series (QmlScatterSeries), the frontend passes itself as
    //! parent so the core object is destroyed with its host.
    explicit QScatterSeries(QObject* parent = nullptr);

    //! @return SeriesType::Scatter — what every frontend scatter
    //! renderer matches against.
    [[nodiscard]] SeriesType type() const override { return SeriesType::Scatter; }

    //! Marker diameter in device-independent pixels. Must be finite and
    //! > 0; invalid values are rejected with a qWarning (AI.md §3.3),
    //! mirroring QAbstractSeries::setLineWidth.
    [[nodiscard]] double markerSize() const { return m_markerSize; }
    void setMarkerSize(double size);

    //! Marker geometry. @see MarkerShape.
    [[nodiscard]] MarkerShape markerShape() const { return m_markerShape; }
    void setMarkerShape(MarkerShape shape);

Q_SIGNALS:
    void markerSizeChanged(double markerSize);
    void markerShapeChanged(MarkerShape markerShape);

private:
    double m_markerSize = 8.0;
    MarkerShape m_markerShape = MarkerShape::Circle;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_SCATTERSERIES_H
