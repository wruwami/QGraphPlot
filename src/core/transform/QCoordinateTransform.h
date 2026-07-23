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

//! @file QCoordinateTransform.h
//! @brief Data <-> pixel coordinate transformation (linear / log scale).

#ifndef QGRAPHPLOT_COORDINATETRANSFORM_H
#define QGRAPHPLOT_COORDINATETRANSFORM_H

#include "../qgraphplot_global.h"

#include <QtCore/QPointF>
#include <QtCore/QRectF>

namespace qgraphplot {

//! @brief Handles mapping between data coordinates and screen pixel coordinates.
//!
//! QCoordinateTransform encapsulates the transformation of 2D data coordinates
//! to pixel space (and vice-versa). It accounts for linear/logarithmic axes, Y-axis
//! inversion (screen space y=0 is top, data space y=0 is bottom), and safeguards
//! against division by zero for degenerate dimensions (e.g. empty or single-point models).
class QGRAPHPLOT_EXPORT QCoordinateTransform {
public:
    //! Constructs a transform between data bounds and pixel rect.
    //! If log scale is enabled for x or y, the corresponding bounds dimensions
    //! must be strictly positive. If invalid, a warning is printed and the scale
    //! falls back to linear (AI.md §3.3: validate boundaries).
    QCoordinateTransform(const QRectF& dataBounds, const QRectF& pixelRect,
                         bool xLog = false, bool yLog = false);

    //! Maps data point to screen pixels. Y-axis is inverted.
    [[nodiscard]] QPointF toPixel(const QPointF& data) const noexcept;

    //! Maps screen pixels back to data space.
    [[nodiscard]] QPointF toData(const QPointF& pixel) const noexcept;

    //! Data coordinate boundaries.
    [[nodiscard]] QRectF dataBounds() const noexcept { return m_dataBounds; }

    //! Screen pixel boundaries.
    [[nodiscard]] QRectF pixelRect() const noexcept { return m_pixelRect; }

    //! Whether the X-axis is logarithmic.
    [[nodiscard]] bool xLog() const noexcept { return m_xLog; }

    //! Whether the Y-axis is logarithmic.
    [[nodiscard]] bool yLog() const noexcept { return m_yLog; }

private:
    QRectF m_dataBounds;
    QRectF m_pixelRect;
    bool m_xLog;
    bool m_yLog;

    // Cached values to avoid repetitive log calculations
    double m_logMinX = 0.0;
    double m_logMaxX = 0.0;
    double m_logMinY = 0.0;
    double m_logMaxY = 0.0;
};

} // namespace qgraphplot

#endif // QGRAPHPLOT_COORDINATETRANSFORM_H
