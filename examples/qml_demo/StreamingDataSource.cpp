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

//! @file StreamingDataSource.cpp
//! @brief 60fps streaming data generator implementation.

#include "StreamingDataSource.h"

#include <cmath>
#include <vector>

StreamingDataSource::StreamingDataSource(QObject* parent) : QObject(parent) {}

void StreamingDataSource::generateFrame()
{
    std::vector<QPointF> batch;
    batch.reserve(static_cast<size_t>(kPointsPerFrame));
    for (int i = 0; i < kPointsPerFrame; ++i) {
        const double x = static_cast<double>(m_sampleIndex) * kSampleDt;
        const double y = std::sin(kTwoPi * kSignalFrequencyHz * x);
        batch.emplace_back(x, y);
        ++m_sampleIndex;
    }
    m_model.appendRange(batch);
    emit windowChanged();
}
