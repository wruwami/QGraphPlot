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

//! @file StreamingDataSource.h
//! @brief Synthetic high-rate signal generator driving the QML streaming demo.

#ifndef STREAMINGDATASOURCE_H
#define STREAMINGDATASOURCE_H

#include <QtCore/QObject>
#include <QtQml/qqmlregistration.h>

#include "model/QRingBufferSeriesModel.h"

class StreamingDataSource : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qgraphplot::QAbstractSeriesModel* model READ model CONSTANT)
    Q_PROPERTY(double xMin READ xMin NOTIFY windowChanged)
    Q_PROPERTY(double xMax READ xMax NOTIFY windowChanged)
    Q_PROPERTY(int pointCount READ pointCount NOTIFY windowChanged)

public:
    explicit StreamingDataSource(QObject* parent = nullptr);

    qgraphplot::QAbstractSeriesModel* model() noexcept { return &m_model; }
    double xMin() const noexcept { return m_xMax - kWindowSeconds; }
    double xMax() const noexcept { return m_xMax; }
    int pointCount() const noexcept { return static_cast<int>(m_model.pointCount()); }

    //! Appends one frame's worth of samples (kPointsPerFrame). Driven by a
    //! QML Timer (16ms) so the demo exercises sustained streaming throughput
    //! (QGraphPlot_MVP_Plan.md § 실시간 60fps 설계).
    Q_INVOKABLE void generateFrame();

signals:
    void windowChanged();

private:
    // Ring buffer sized to hold ~1s of history at kSampleRateHz, matching the
    // "60K points, 60fps streaming" performance target (issue #10).
    static constexpr qsizetype kCapacity = 60000;
    static constexpr int kPointsPerFrame = 1000;
    static constexpr double kFrameIntervalSeconds = 0.016;  // matches the QML Timer (16ms)
    static constexpr double kSampleRateHz = kPointsPerFrame / kFrameIntervalSeconds;
    static constexpr double kSignalFrequencyHz = 5.0;
    static constexpr double kSampleDt = 1.0 / kSampleRateHz;
    static constexpr double kWindowSeconds = kCapacity * kSampleDt;
    static constexpr double kTwoPi = 6.283185307179586;

    qgraphplot::QRingBufferSeriesModel m_model{kCapacity};
    double m_xMax = 0.0;
};

#endif  // STREAMINGDATASOURCE_H
