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
//! @brief Load-test data generator for the QML 60fps streaming demo.

#ifndef STREAMINGDATASOURCE_H
#define STREAMINGDATASOURCE_H

#include <vector>

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "model/QRingBufferSeriesModel.h"

class StreamingDataSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qgraphplot::QRingBufferSeriesModel* model READ model CONSTANT)
    Q_PROPERTY(double xMin READ xMin NOTIFY xMinChanged)
    Q_PROPERTY(double xMax READ xMax NOTIFY xMaxChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(int batchSize READ batchSize WRITE setBatchSize NOTIFY batchSizeChanged)

public:
    explicit StreamingDataSource(QObject* parent = nullptr);
    ~StreamingDataSource() override = default;

    StreamingDataSource(const StreamingDataSource&) = delete;
    StreamingDataSource& operator=(const StreamingDataSource&) = delete;
    StreamingDataSource(StreamingDataSource&&) = delete;
    StreamingDataSource& operator=(StreamingDataSource&&) = delete;

    [[nodiscard]] qgraphplot::QRingBufferSeriesModel* model() noexcept { return &m_model; }
    [[nodiscard]] double xMin() const noexcept { return m_xMin; }
    [[nodiscard]] double xMax() const noexcept { return m_xMax; }
    [[nodiscard]] bool running() const noexcept { return m_running; }
    [[nodiscard]] int interval() const noexcept { return m_interval; }
    [[nodiscard]] int batchSize() const noexcept { return m_batchSize; }

    void setRunning(bool running);
    void setInterval(int interval);
    void setBatchSize(int batchSize);

Q_SIGNALS:
    void xMinChanged();
    void xMaxChanged();
    void runningChanged();
    void intervalChanged();
    void batchSizeChanged();

private slots:
    void generateBatch();

private:
    void prefill();
    void updateViewport();

    QTimer m_timer;
    qgraphplot::QRingBufferSeriesModel m_model;
    std::vector<QPointF> m_batch;

    double m_xMin = 0.0;
    double m_xMax = 60.0;
    double m_nextX = 0.0;
    double m_phase = 0.0;
    bool m_running = true;
    int m_interval = 16;
    int m_batchSize = 1000;
    double m_xStep = 0.001;
    double m_windowWidth = 60.0;
    double m_phaseStep = 0.05;
};

#endif  // STREAMINGDATASOURCE_H
