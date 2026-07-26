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

//! @file QAutoScaler.h
//! @brief Axis auto-range (auto-scale) computation shared by both frontends.

#ifndef QGRAPHPLOT_AUTOSCALER_H
#define QGRAPHPLOT_AUTOSCALER_H

#include <QtCore/QRectF>

#include "../qgraphplot_global.h"

namespace qgraphplot
{

class QAbstractSeries;

//! @brief Pure helper that computes padded axis bounds from a series set.
//!
//! QmlChartView and WidgetChartView both expose `autoScaleX` / `autoScaleY`
//! properties (issue #63). Rather than duplicate the "union all series model
//! bounds and pad" math in each frontend, the calculation lives here once
//! (AI.md §3.2 — no logic duplication). The helper is signal-free: frontends
//! own the `boundsChanged()` subscription and call `computePaddedBounds()`
//! whenever they need to refresh the axis range.
//!
//! @par Edge cases
//! - **No usable series** (empty list, null models, or all-empty models):
//!   returns `QRectF(0, 0, 1, 1)` so the view always has a valid, finite
//!   transform instead of a degenerate zero-area range. A model with zero
//!   points is treated as empty even if its `bounds()` returns non-null.
//! - **Zero-width / zero-height span** (single point, or all points share an
//!   axis coordinate): the axis is expanded by a unit (`1.0`) on each side
//!   so `min < max` always holds, regardless of the padding ratio.
//! - **Padding ratio**: `0.0` means "fit exactly" (no expansion beyond the
//!   data), `0.05` (the frontend default) means "5% of the span on each
//!   side". Negative ratios are clamped to `0.0` (AI.md §3.3 — validate).
class QGRAPHPLOT_EXPORT QAutoScaler
{
public:
    QAutoScaler() = delete;  // Static helper, no instances.

    //! Computes the padded data bounds covering the union of all @p series'
    //! model bounds.
    //!
    //! @param series        The tracked series collection (may contain null
    //!                      entries or series with null/empty models — both
    //!                      are skipped).
    //! @param paddingRatio  Fraction of each axis span to pad on both sides.
    //!                      Negative values are treated as `0.0`.
    //! @return A finite `QRectF` with strictly positive width and height.
    //!         `QRectF(0, 0, 1, 1)` if no usable series were found.
    [[nodiscard]] static QRectF computePaddedBounds(const QList<QAbstractSeries*>& series,
                                                    double paddingRatio);
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_AUTOSCALER_H
