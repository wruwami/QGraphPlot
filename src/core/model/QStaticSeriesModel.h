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

//! @file QStaticSeriesModel.h
//! @brief Vector-backed series model for static / batch data.
//!
//! QStaticSeriesModel is the second concrete QAbstractSeriesModel. While
//! QRingBufferSeriesModel is optimized for 60fps streaming with a fixed
//! capacity, this model targets the more common case of plotting data
//! loaded from a file, database, or computed in one go.
//!
//! Design rationale (QGraphPlot_MVP_Plan.md § Model Layer):
//!   - Simple std::vector<QPointF> storage — no ring bookkeeping overhead.
//!   - setPoints() replaces all data in one call (analogous to
//!     QXYSeries::replace() in Qt Charts, or QCustomPlot setData()).
//!   - append() / appendBatch() grow the vector for incremental additions.
//!   - replacePoint() supports in-place edits without a full rebuild.
//!   - Bounds are computed once on bulk operations and updated incrementally
//!     on appends; the same boundsChanged() contract as QRingBufferSeriesModel.

#ifndef QGRAPHPLOT_STATICSERIESMODEL_H
#define QGRAPHPLOT_STATICSERIESMODEL_H

#include <iterator>
#include <vector>

#include <QtCore/QMutex>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSpan>

#include "QAbstractSeriesModel.h"

namespace qgraphplot
{

//! @brief Vector-backed model for static / batch data.
//!
//! Unlike QRingBufferSeriesModel, this model has no fixed capacity — data
//! grows as needed. It is the natural choice when the full dataset is
//! available upfront (CSV import, computed curves, etc.).
class QGRAPHPLOT_EXPORT QStaticSeriesModel final : public QAbstractSeriesModel
{
    Q_OBJECT

public:
    //! Constructs an empty static model.
    explicit QStaticSeriesModel(QObject* parent = nullptr);

    ~QStaticSeriesModel() override;

    QStaticSeriesModel(const QStaticSeriesModel&) = delete;
    QStaticSeriesModel& operator=(const QStaticSeriesModel&) = delete;
    QStaticSeriesModel(QStaticSeriesModel&&) = delete;
    QStaticSeriesModel& operator=(QStaticSeriesModel&&) = delete;

    // ─ QAbstractSeriesModel ──────────────────────────────────

    [[nodiscard]] qsizetype pointCount() const override;
    [[nodiscard]] QPointF pointAt(qsizetype index) const override;
    [[nodiscard]] PointSpan points(qsizetype first, qsizetype last) const override;
    [[nodiscard]] QRectF bounds() const override;

    // ─ Bulk operations ───────────────────────────────────────

    //! Replaces all data with @p pts. Emits:
    //!   - pointsRemoved(0, oldCount-1)   if the model was non-empty
    //!   - pointsInserted(0, newCount-1)  if @p pts is non-empty
    //!   - boundsChanged()
    //!   - modelChanged(newPointCount)
    //!
    //! This is the primary way to load a full dataset.
    void setPoints(QSpan<const QPointF> pts);

    //! Appends a batch of @p pts to the end of the vector. Emits:
    //!   - pointsInserted(firstNew, lastNew)
    //!   - boundsChanged() if bounds expanded
    //!   - modelChanged(newPointCount)
    //!
    //! @note Empty @p pts is a no-op, no signals emitted.
    void appendBatch(QSpan<const QPointF> pts);

    //! Convenience overload for a single point.
    void append(QPointF pt);

    //! Convenience overload for STL-compatible containers of QPointF.
    template <typename Container>
    void appendRange(const Container& c)
    {
        std::vector<QPointF> snapshot(std::begin(c), std::end(c));
        appendBatch(QSpan<const QPointF>(snapshot.data(), static_cast<qsizetype>(snapshot.size())));
    }

    //! Replaces the point at @p index with @p pt. Emits:
    //!   - dataChanged(index, index)
    //!   - boundsChanged() if bounds changed (full recompute)
    //!   - modelChanged(pointCount())
    //!
    //! @pre 0 <= @p index < pointCount()
    void replacePoint(qsizetype index, QPointF pt);

    //! Clears all points. Emits:
    //!   - pointsRemoved(0, count-1) if the model was non-empty
    //!   - boundsChanged()
    //!   - modelChanged(0)
    void clear();

    //! Reserves storage for at least @p count points.
    //! Does not change pointCount() or emit signals.
    void reserve(qsizetype count);

private:
    //! Recomputes m_bounds from scratch by scanning all points. O(n).
    void recomputeBounds() noexcept;

    //! Incrementally expands m_bounds to include @p pt. O(1).
    void expandBounds(QPointF pt) noexcept;

    mutable QMutex m_mutex;
    std::vector<QPointF> m_points;
    QRectF m_bounds;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_STATICSERIESMODEL_H
