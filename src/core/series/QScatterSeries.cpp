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

//! @file QScatterSeries.cpp
//! @brief QScatterSeries implementation.
//!
//! The shared property logic (validation, change-guarded signals, model
//! lifecycle) lives in QAbstractSeries; QScatterSeries contributes its
//! SeriesType identity plus the scatter-specific markerSize/markerShape
//! setters, which mirror the lineWidth validation idiom.

#include "QScatterSeries.h"

#include <cmath>

namespace qgraphplot
{

QScatterSeries::QScatterSeries(QObject* parent) : QAbstractSeries(parent) {}

void QScatterSeries::setMarkerSize(double size)
{
    if (!std::isfinite(size) || size <= 0.0) {
        qWarning("QScatterSeries::setMarkerSize: markerSize must be finite and > 0");
        return;
    }
    if (qFuzzyCompare(m_markerSize, size)) {
        return;
    }
    m_markerSize = size;
    Q_EMIT markerSizeChanged(m_markerSize);
}

void QScatterSeries::setMarkerShape(MarkerShape shape)
{
    if (m_markerShape == shape) {
        return;
    }
    m_markerShape = shape;
    Q_EMIT markerShapeChanged(m_markerShape);
}

}  // namespace qgraphplot
