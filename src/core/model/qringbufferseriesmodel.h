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

//! @file qringbufferseriesmodel.h
//! @brief Fixed-capacity ring buffer series model for real-time streaming.
//!
//! QRingBufferSeriesModel is the first concrete QAbstractSeriesModel. It is
//! optimized for the 60fps streaming case: new points are appended in
//! batches via appendBatch() and the oldest points are silently evicted
//! when the fixed capacity is reached. No reallocations happen after
//! construction, so the cost of an append is O(batch_size) with constant
//! factor dominated by the memcpy.
//!
//! Threading model (QGraphPlot_MVP_Plan.md § 60fps 설계):
//!   - By default NO internal locking (fastest). Use only when all access
//!     is from a single thread or when the GUI thread is blocked during
//!     render (the common Qt Quick case).
//!   - Pass ThreadSafety::Enabled to ctor to take a QMutex on every
//!     mutating / read call. Required when a producer thread pushes data
//!     while the GUI/render thread reads.

#ifndef QGRAPHPLOT_RINGBUFFERSERIESMODEL_H
#define QGRAPHPLOT_RINGBUFFERSERIESMODEL_H

#include "qabstractseriesmodel.h"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSpan>
#include <vector>

namespace qgraphplot {

//! Whether QRingBufferSeriesModel takes a mutex on each public call.
//! Kept as a scoped enum (AI.md §1.4) and not duplicated as magic bools
//! (AI.md §3.2).
enum class ThreadSafety : unsigned char {
    Disabled,  //!< No locking. Single-threaded or GUI-blocks-render usage.
    Enabled    //!< QMutex on every mutating/read call.
};

//! @brief Fixed-capacity FIFO ring buffer of (x, y) points.
//!
//! Once @p capacity points are filled, further appends evict the oldest
//! points and emit pointsRemoved(0, k-1) followed by
//! pointsInserted(adjustedRange). The buffer always reports the most
//! recent @p capacity points via pointCount() / pointAt().
class QGRAPHPLOT_EXPORT QRingBufferSeriesModel final : public QAbstractSeriesModel {
    Q_OBJECT

public:
    //! Constructs an empty ring buffer with the given fixed @p capacity.
    //!
    //! @pre @p capacity > 0. An invalid capacity is rejected with
    //!      qWarning and clamped to 1 (AI.md §3.3: validate at boundary).
    explicit QRingBufferSeriesModel(qsizetype capacity,
                                    ThreadSafety threadSafety = ThreadSafety::Disabled,
                                    QObject* parent = nullptr);

    ~QRingBufferSeriesModel() override;

    QRingBufferSeriesModel(const QRingBufferSeriesModel&) = delete;
    QRingBufferSeriesModel& operator=(const QRingBufferSeriesModel&) = delete;
    QRingBufferSeriesModel(QRingBufferSeriesModel&&) = delete;
    QRingBufferSeriesModel& operator=(QRingBufferSeriesModel&&) = delete;

    // ─ Configuration ─────────────────────────────────────────

    //! The fixed capacity set at construction.
    [[nodiscard]] qsizetype capacity() const noexcept { return m_capacity; }

    //! Whether this instance locks on every public call.
    [[nodiscard]] ThreadSafety threadSafety() const noexcept { return m_threadSafety; }

    //! Clears all points. Does NOT change capacity.
    //! Emits pointsRemoved(0, count-1) if the buffer was non-empty.
    void clear();

    // ─ QAbstractSeriesModel ──────────────────────────────────

    [[nodiscard]] qsizetype pointCount() const override;
    [[nodiscard]] QPointF pointAt(qsizetype index) const override;
    [[nodiscard]] PointSpan points(qsizetype first, qsizetype last) const override;
    [[nodiscard]] QRectF bounds() const override;

    // ─ Mutators ──────────────────────────────────────────────

    //! Appends a batch of @p pts to the buffer. If the batch overflows the
    //! capacity, the oldest points are evicted. Emits:
    //!   - pointsRemoved(0, k-1) when @p k old points were evicted (>0 only)
    //!   - pointsInserted(newFirst, newLast) for the inserted indices
    //!   - modelChanged(pointCount())
    //!   - boundsChanged() if bounds shifted
    //!
    //! Validation (AI.md §3.3):
    //!   - Empty @p pts: no-op, no signals. Returns immediately.
    //!   - @p pts.size() > capacity: only the last @p capacity points are
    //!     retained (oldest in the batch also dropped).
    void appendBatch(QSpan<const QPointF> pts);

    //! Convenience overload for a single point.
    void append(QPointF pt);

    //! Convenience overload for STL-compatible containers of QPointF.
    template <typename Container>
    void appendRange(const Container& c) {
        appendBatch(QSpan<const QPointF>(c.data(), static_cast<qsizetype>(c.size())));
    }

private:
    //! Recompute m_bounds from scratch (O(n)). Called when incremental
    //! bound tracking would be more expensive than a full rescan
    //! (e.g. after clear or large eviction).
    void recomputeBounds() noexcept;

    //! Adjusts m_bounds incrementally to include @p pt. Cheap when the
    //! new point lies within or just outside the current bounds.
    void expandBounds(QPointF pt) noexcept;

    const qsizetype m_capacity;
    const ThreadSafety m_threadSafety;
    std::vector<QPointF> m_buffer;
    //! Index in m_buffer that holds the OLDEST live point.
    qsizetype m_head = 0;
    //! Number of live points (<= capacity).
    qsizetype m_size = 0;
    QRectF m_bounds;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_RINGBUFFERSERIESMODEL_H
