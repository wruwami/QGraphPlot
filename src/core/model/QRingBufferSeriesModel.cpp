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

#include <algorithm>
#include <utility>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

namespace qgraphplot
{

namespace
{
//! Logs once when capacity precondition is violated (AI.md §3.3).
Q_LOGGING_CATEGORY(lcRingBuffer, "qgraphplot.ringbuffer")
}  // namespace

// ─────────────────────────────────────────────────────────────
// Helper RAII mutex that is a no-op when ThreadSafety::Disabled.
// A pointer (possibly null) avoids even the atomic cost of a QMutex
// member when locking is off.
// ─────────────────────────────────────────────────────────────
class QRingBufferSeriesModelLock
{
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
    : QAbstractSeriesModel(parent), m_capacity(capacity > 0 ? capacity : 1),
      m_threadSafety(threadSafety),
      m_mutex(threadSafety == ThreadSafety::Enabled ? std::make_unique<QMutex>() : nullptr),
      // Sized 2x capacity: every write below is mirrored into slot
      // (physical + m_capacity) so that any logical range [first, last]
      // is readable as ONE contiguous span starting at (m_head + first),
      // even when it wraps past the end of the primary region. Without
      // this, points() would have to silently truncate wrapped ranges.
      m_buffer(static_cast<size_t>(m_capacity) * 2)
{
    if (capacity <= 0) {
        qCWarning(lcRingBuffer) << "QRingBufferSeriesModel: capacity must be > 0; clamped to 1";
    }
}

QRingBufferSeriesModel::~QRingBufferSeriesModel() = default;

// ─────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────

qsizetype QRingBufferSeriesModel::pointCount() const
{
    const QRingBufferSeriesModelLock lock(m_mutex.get());
    return m_size;
}

QPointF QRingBufferSeriesModel::pointAt(qsizetype index) const
{
    const QRingBufferSeriesModelLock lock(m_mutex.get());
    Q_ASSERT(index >= 0 && index < m_size);
    return m_buffer[static_cast<size_t>((m_head + index) % m_capacity)];
}

PointSpan QRingBufferSeriesModel::points(qsizetype first, qsizetype last) const
{
    const QRingBufferSeriesModelLock lock(m_mutex.get());
    Q_ASSERT(first >= 0 && last >= first && last < m_size);

    const qsizetype count = last - first + 1;
    // m_head < m_capacity and first < m_size <= m_capacity, so this index
    // (and physicalFirst + count - 1) always lands inside the mirrored
    // 2x-capacity buffer — no wraparound handling needed here.
    //
    // Note (ThreadSafety::Enabled): the lock only protects THIS call's read
    // of m_head/m_size and span construction. It cannot protect the
    // caller's subsequent iteration of the returned span against a
    // concurrent appendBatch() on another thread — same contract as
    // documented on QAbstractSeriesModel::points() ("invalidated by any
    // mutating call"). Safe under the intended usage (GUI thread blocked
    // during render); a fully-synchronized snapshot would require copying.
    const qsizetype physicalFirst = m_head + first;
    return PointSpan(m_buffer.data() + physicalFirst, count);
}

QRectF QRingBufferSeriesModel::bounds() const
{
    const QRingBufferSeriesModelLock lock(m_mutex.get());
    return m_bounds;
}

// ─────────────────────────────────────────────────────────────
// Mutators
// ─────────────────────────────────────────────────────────────

void QRingBufferSeriesModel::clear()
{
    // Signals are emitted after the lock is released below: emitting while
    // held would deadlock a direct-connected slot that re-enters this
    // object (QMutex is non-recursive), even in the single-threaded case.
    qsizetype oldSize = 0;
    {
        const QRingBufferSeriesModelLock lock(m_mutex.get());
        oldSize = m_size;
        m_head = 0;
        m_size = 0;
        m_bounds = QRectF();
    }
    if (oldSize > 0) {
        Q_EMIT pointsRemoved(0, oldSize - 1);
    }
    Q_EMIT boundsChanged();
    Q_EMIT modelChanged(0);
}

void QRingBufferSeriesModel::appendBatch(QSpan<const QPointF> pts)
{
    if (pts.isEmpty()) {
        return;  // AI.md §3.3: explicit no-op on empty input.
    }

    // Mutation happens under the lock; signals are emitted after it's
    // released below (see the note in clear() about direct-connected
    // slots re-entering this object while the mutex is held).
    qsizetype evicted = 0;
    qsizetype newFirst = 0;
    qsizetype newLast = 0;
    qsizetype newSize = 0;
    {
        const QRingBufferSeriesModelLock lock(m_mutex.get());

        // If the batch is larger than the whole buffer, only keep the tail.
        QSpan<const QPointF> src = pts;
        if (src.size() > m_capacity) {
            src = src.last(m_capacity);
        }

        const qsizetype appended = src.size();
        const qsizetype freeSpace = m_capacity - m_size;
        evicted = (appended > freeSpace) ? (appended - freeSpace) : 0;

        // Write the new points into the ring. We compute destination positions
        // first to avoid overlapping memcpy issues when the ring wraps.
        const qsizetype writeStart = (m_head + m_size) % m_capacity;
        const qsizetype firstChunk = std::min(appended, m_capacity - writeStart);
        const qsizetype secondChunk = appended - firstChunk;

        auto* dst = m_buffer.data();
        const auto* srcData = src.data();
        const auto capacityOffset = static_cast<size_t>(m_capacity);

        std::copy_n(
            srcData, static_cast<size_t>(firstChunk), dst + static_cast<size_t>(writeStart));
        if (secondChunk > 0) {
            std::copy_n(srcData + firstChunk, static_cast<size_t>(secondChunk), dst);
        }

        // Mirror the same writes into the [m_capacity, 2*m_capacity) half so
        // that points() can always return a single contiguous span (see the
        // m_buffer sizing comment in the constructor).
        std::copy_n(srcData,
                    static_cast<size_t>(firstChunk),
                    dst + static_cast<size_t>(writeStart) + capacityOffset);
        if (secondChunk > 0) {
            std::copy_n(
                srcData + firstChunk, static_cast<size_t>(secondChunk), dst + capacityOffset);
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
            // m_size was already advanced above, so m_size == appended means
            // the buffer was empty before this batch: seed m_bounds directly
            // instead of expanding from the stale default-constructed QRectF()
            // (which would anchor min/max at (0,0) instead of the first point).
            const bool wasEmpty = (m_size == appended);
            for (qsizetype i = 0; i < appended; ++i) {
                if (wasEmpty && i == 0) {
                    m_bounds = QRectF(srcData[0].x(), srcData[0].y(), 0.0, 0.0);
                } else {
                    expandBounds(srcData[i]);
                }
            }
        }

        newFirst = m_size - appended;
        newLast = m_size - 1;
        newSize = m_size;
    }

    // Emit signals. Indices are in logical (oldest-first) coordinates.
    if (evicted > 0) {
        Q_EMIT pointsRemoved(0, evicted - 1);
    }
    Q_EMIT pointsInserted(newFirst, newLast);
    Q_EMIT boundsChanged();
    Q_EMIT modelChanged(newSize);
}

void QRingBufferSeriesModel::append(QPointF pt)
{
    appendBatch(QSpan<const QPointF>(&pt, 1));
}

// ─────────────────────────────────────────────────────────────
// Bounds helpers
// ─────────────────────────────────────────────────────────────

void QRingBufferSeriesModel::recomputeBounds() noexcept
{
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
        if (p.x() < minX)
            minX = p.x();
        if (p.x() > maxX)
            maxX = p.x();
        if (p.y() < minY)
            minY = p.y();
        if (p.y() > maxY)
            maxY = p.y();
    }
    m_bounds = QRectF(minX, minY, maxX - minX, maxY - minY);
}

void QRingBufferSeriesModel::expandBounds(QPointF pt) noexcept
{
    if (m_size == 0) {
        m_bounds = QRectF(pt.x(), pt.y(), 0.0, 0.0);
        return;
    }
    qreal minX = m_bounds.left();
    qreal maxX = m_bounds.right();
    qreal minY = m_bounds.top();
    qreal maxY = m_bounds.bottom();
    bool changed = false;
    if (pt.x() < minX) {
        minX = pt.x();
        changed = true;
    }
    if (pt.x() > maxX) {
        maxX = pt.x();
        changed = true;
    }
    if (pt.y() < minY) {
        minY = pt.y();
        changed = true;
    }
    if (pt.y() > maxY) {
        maxY = pt.y();
        changed = true;
    }
    if (changed) {
        m_bounds = QRectF(minX, minY, maxX - minX, maxY - minY);
    }
}

}  // namespace qgraphplot
