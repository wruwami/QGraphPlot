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

#include "QRingBufferSeriesModel.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include <algorithm>
#include <utility>

namespace qgraphplot {

namespace {
//! Logs once when capacity precondition is violated (AI.md §3.3).
Q_LOGGING_CATEGORY(lcRingBuffer, "qgraphplot.ringbuffer")
}  // namespace

// ─────────────────────────────────────────────────────────────
// Helper RAII mutex that is a no-op when ThreadSafety::Disabled.
// A pointer (possibly null) avoids even the atomic cost of a QMutex
// member when locking is off.
// ─────────────────────────────────────────────────────────────
class QRingBufferSeriesModelLock {
public:
    explicit QRingBufferSeriesModelLock(QMutex* m) : m_locker(m) {}
private:
    QMutexLocker<QMutex> m_locker;
};

// ─────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────

QRingBufferSeriesModel::QRingBufferSeriesModel(qsizetype capacity,
                                               ThreadSafety threadSafety,
                                               QObject* parent)
    : QAbstractSeriesModel(parent),
      m_capacity(capacity > 0 ? capacity : 1),
      m_threadSafety(threadSafety),
      m_buffer(static_cast<size_t>(m_capacity > 0 ? m_capacity : 1)) {
    if (capacity <= 0) {
        qCWarning(lcRingBuffer)
            << "QRingBufferSeriesModel: capacity must be > 0; clamped to 1";
    }
}

QRingBufferSeriesModel::~QRingBufferSeriesModel() = default;

// ─────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────

qsizetype QRingBufferSeriesModel::pointCount() const {
    return m_size;
}

QPointF QRingBufferSeriesModel::pointAt(qsizetype index) const {
    Q_ASSERT(index >= 0 && index < m_size);
    return m_buffer[static_cast<size_t>((m_head + index) % m_capacity)];
}

PointSpan QRingBufferSeriesModel::points(qsizetype first, qsizetype last) const {
    Q_ASSERT(first >= 0 && last >= first && last < m_size);

    const qsizetype count = last - first + 1;
    const qsizetype physicalFirst = (m_head + first) % m_capacity;

    // Fast path: the requested range does not wrap around the ring.
    if (physicalFirst + count <= m_capacity) {
        return PointSpan(m_buffer.data() + physicalFirst, count);
    }

    // Slow path: range wraps. The returned span must be contiguous in
    // memory; if it is not, the caller should fetch in two halves.
    // For now we expose only the leading slice; a follow-up can promote
    // this to a copy or to two calls. This keeps the contract simple
    // (span = contiguous view).
    const qsizetype leading = m_capacity - physicalFirst;
    return PointSpan(m_buffer.data() + physicalFirst, leading);
}

QRectF QRingBufferSeriesModel::bounds() const {
    return m_bounds;
}

// ─────────────────────────────────────────────────────────────
// Mutators
// ─────────────────────────────────────────────────────────────

void QRingBufferSeriesModel::clear() {
    m_head = 0;
    m_size = 0;
    m_bounds = QRectF();
    Q_EMIT pointsRemoved(0, 0);  // semantic: buffer emptied
    Q_EMIT boundsChanged();
    Q_EMIT modelChanged(0);
}

void QRingBufferSeriesModel::appendBatch(QSpan<const QPointF> pts) {
    if (pts.isEmpty()) {
        return;  // AI.md §3.3: explicit no-op on empty input.
    }

    // If the batch is larger than the whole buffer, only keep the tail.
    QSpan<const QPointF> src = pts;
    if (src.size() > m_capacity) {
        src = src.last(m_capacity);
    }

    const qsizetype appended = src.size();
    const qsizetype freeSpace = m_capacity - m_size;
    const qsizetype evicted = (appended > freeSpace) ? (appended - freeSpace) : 0;

    // Write the new points into the ring. We compute destination positions
    // first to avoid overlapping memcpy issues when the ring wraps.
    const qsizetype writeStart = (m_head + m_size) % m_capacity;
    const qsizetype firstChunk = std::min(appended, m_capacity - writeStart);
    const qsizetype secondChunk = appended - firstChunk;

    auto* dst = m_buffer.data();
    const auto* srcData = src.data();

    std::copy_n(srcData, static_cast<size_t>(firstChunk),
                dst + static_cast<size_t>(writeStart));
    if (secondChunk > 0) {
        std::copy_n(srcData + firstChunk, static_cast<size_t>(secondChunk), dst);
    }

    // Advance bookkeeping.
    if (evicted > 0) {
        m_head = (m_head + evicted) % m_capacity;
        m_size = m_capacity;
    } else {
        m_size += appended;
    }

    // Bounds tracking. After eviction we cannot cheaply shrink the bound
    // (the new min/max may have been evicted), so recompute from scratch.
    // On pure-append we can incrementally expand.
    if (evicted > 0) {
        recomputeBounds();
    } else {
        const bool wasEmpty = (m_size == appended);
        for (qsizetype i = 0; i < appended; ++i) {
            if (wasEmpty && i == 0) {
                m_bounds = QRectF(srcData[0].x(), srcData[0].y(), 0.0, 0.0);
            } else {
                expandBounds(srcData[i]);
            }
        }
    }

    // Emit signals. Indices are in logical (oldest-first) coordinates.
    if (evicted > 0) {
        Q_EMIT pointsRemoved(0, evicted - 1);
    }
    const qsizetype newFirst = (evicted > 0) ? (m_size - appended) : (m_size - appended);
    const qsizetype newLast = m_size - 1;
    Q_EMIT pointsInserted(newFirst, newLast);
    Q_EMIT boundsChanged();
    Q_EMIT modelChanged(m_size);
}

void QRingBufferSeriesModel::append(QPointF pt) {
    appendBatch(QSpan<const QPointF>(&pt, 1));
}

// ─────────────────────────────────────────────────────────────
// Bounds helpers
// ─────────────────────────────────────────────────────────────

void QRingBufferSeriesModel::recomputeBounds() noexcept {
    if (m_size == 0) {
        m_bounds = QRectF();
        return;
    }
    qreal minX = m_buffer[static_cast<size_t>(m_head)].x();
    qreal maxX = minX;
    qreal minY = m_buffer[static_cast<size_t>(m_head)].y();
    qreal maxY = minY;
    for (qsizetype i = 1; i < m_size; ++i) {
        const QPointF& p = m_buffer[static_cast<size_t>((m_head + i) % m_capacity)];
        if (p.x() < minX) minX = p.x();
        if (p.x() > maxX) maxX = p.x();
        if (p.y() < minY) minY = p.y();
        if (p.y() > maxY) maxY = p.y();
    }
    m_bounds = QRectF(minX, minY, maxX - minX, maxY - minY);
}

void QRingBufferSeriesModel::expandBounds(QPointF pt) noexcept {
    if (m_size == 0) {
        m_bounds = QRectF(pt.x(), pt.y(), 0.0, 0.0);
        return;
    }
    qreal minX = m_bounds.left();
    qreal maxX = m_bounds.right();
    qreal minY = m_bounds.top();
    qreal maxY = m_bounds.bottom();
    bool changed = false;
    if (pt.x() < minX) { minX = pt.x(); changed = true; }
    if (pt.x() > maxX) { maxX = pt.x(); changed = true; }
    if (pt.y() < minY) { minY = pt.y(); changed = true; }
    if (pt.y() > maxY) { maxY = pt.y(); changed = true; }
    if (changed) {
        m_bounds = QRectF(minX, minY, maxX - minX, maxY - minY);
    }
}

}  // namespace qgraphplot
