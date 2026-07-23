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

//! @file QScaleEngine.h
//! @brief Axis tick / label computation (nice-number algorithm).

#ifndef QGRAPHPLOT_SCALEENGINE_H
#define QGRAPHPLOT_SCALEENGINE_H

#include "../qgraphplot_global.h"

#include <QtCore/QString>
#include <vector>

namespace qgraphplot {

//! @brief Computes nice tick positions and text labels for chart axes.
class QGRAPHPLOT_EXPORT QScaleEngine {
public:
    //! Bundles a single tick's value/position and its formatted label.
    struct TickInfo {
        double position; //!< Position in data coordinates.
        QString label;   //!< Formatted label text.
    };

    QScaleEngine() = default;

    //! Computes nice tick marks for the range [min, max] given targetTickCount.
    //! Supports both linear and logarithmic scales (AI.md §3.3: validate inputs).
    [[nodiscard]] static std::vector<TickInfo> calculateTicks(
        double min, double max, int targetTickCount, bool isLog = false);

    //! Formats a tick value nicely based on the step size to avoid float precision noise.
    [[nodiscard]] static QString formatLabel(double value, double step);
};

} // namespace qgraphplot

#endif // QGRAPHPLOT_SCALEENGINE_H
