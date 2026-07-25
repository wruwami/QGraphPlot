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

#include <QtCore/qmath.h>

namespace
{
constexpr qsizetype DEFAULT_CAPACITY = 60000;
}  // namespace

StreamingDataSource::StreamingDataSource(QObject* parent)
    : QObject(parent), m_model(DEFAULT_CAPACITY, qgraphplot::ThreadSafety::Disabled, nullptr)
{
    m_timer.setInterval(m_interval);
    m_timer.setTimerType(Qt::PreciseTimer);

    QObject::connect(&m_timer, &QTimer::timeout, this, &StreamingDataSource::generateBatch);

    prefill();

    if (m_running) {
        m_timer.start();
    }
}

void StreamingDataSource::setRunning(bool running)
{
    if (m_running == running) {
        return;
    }
    m_running = running;
    if (m_running) {
        m_timer.start();
    } else {
        m_timer.stop();
    }
    Q_EMIT runningChanged();
}

void StreamingDataSource::setInterval(int interval)
{
    if (interval <= 0 || m_interval == interval) {
        return;
    }
    m_interval = interval;
    m_timer.setInterval(m_interval);
    Q_EMIT intervalChanged();
}

void StreamingDataSource::setBatchSize(int batchSize)
{
    if (batchSize <= 0 || m_batchSize == batchSize) {
        return;
    }
    m_batchSize = batchSize;
    m_batch.resize(static_cast<size_t>(m_batchSize));
    Q_EMIT batchSizeChanged();
}

void StreamingDataSource::prefill()
{
    m_batch.clear();
    m_batch.reserve(static_cast<size_t>(m_batchSize));

    const qsizetype capacity = m_model.capacity();
    std::vector<QPointF> initial(static_cast<size_t>(capacity));
    for (qsizetype i = 0; i < capacity; ++i) {
        const double x = i * m_xStep;
        initial[static_cast<size_t>(i)] = QPointF(x, qSin(x));
    }
    m_model.appendBatch(QSpan<const QPointF>(initial.data(), capacity));

    m_nextX = capacity * m_xStep;
    updateViewport();
}

void StreamingDataSource::generateBatch()
{
    m_batch.resize(static_cast<size_t>(m_batchSize));
    for (int i = 0; i < m_batchSize; ++i) {
        const double x = m_nextX + i * m_xStep;
        const double y = qSin(x) + 0.2 * qSin(10.0 * x + m_phase);
        m_batch[static_cast<size_t>(i)] = QPointF(x, y);
    }

    m_model.appendBatch(QSpan<const QPointF>(m_batch.data(), m_batchSize));

    m_nextX += m_batchSize * m_xStep;
    m_phase += m_phaseStep;
    updateViewport();
}

void StreamingDataSource::updateViewport()
{
    m_xMax = m_nextX;
    m_xMin = qMax(0.0, m_xMax - m_windowWidth);
    Q_EMIT xMinChanged();
    Q_EMIT xMaxChanged();
}
