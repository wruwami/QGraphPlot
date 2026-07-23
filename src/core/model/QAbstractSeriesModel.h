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

//! @file QAbstractSeriesModel.h
//! @brief Abstract data-source interface for chart series.
//!
//! QAbstractSeriesModel is the contract between a data source (ring buffer,
//! static vector, database feed, ...) and the rendering layer. Concrete
//! subclasses feed (x, y) points to the chart and emit fine-grained signals
//! so that frontends can perform incremental GPU updates instead of full
//! rebuilds each frame.
//!
//! Design rationale (QGraphPlot_MVP_Plan.md § Model Layer):
//!   - Chart-specialized API, NOT QModelIndex-based. Charts need range
//!     queries and bounds, not hierarchical row/column access.
//!   - Pure interface: no rendering awareness. Both QML and Widget
//!     frontends consume the same model instance.

#ifndef QGRAPHPLOT_ABSTRACTSERIESMODEL_H
#define QGRAPHPLOT_ABSTRACTSERIESMODEL_H

#include "../qgraphplot_global.h"

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSpan>

namespace qgraphplot {

//! A view over a contiguous run of (x, y) points in the model.
//!
//! Returned by points(first, last). The underlying storage is owned by
//! the model; callers must not retain the span beyond the next mutating
//! call (append/insert/remove) on the same model.
using PointSpan = QSpan<const QPointF>;

//! @brief Abstract data-source for chart series.
//!
//! Subclasses represent a 2D point stream that the rendering layer turns
//! into a visible series (line, scatter, ...). Implementations MUST be
//! thread-safe with respect to their own producer thread; GUI thread
//! access is always safe because Qt Quick blocks the GUI thread while
//! the render thread reads (QGraphPlot_MVP_Plan.md § 실시간 60fps 설계).
class QGRAPHPLOT_EXPORT QAbstractSeriesModel : public QObject {
    Q_OBJECT

public:
    //! Constructs a model with the given @p parent for Qt ownership (RAII).
    explicit QAbstractSeriesModel(QObject* parent = nullptr)
        : QObject(parent) {}

    //! Polymorphic base → virtual destructor (AI.md §1.4: only when needed).
    ~QAbstractSeriesModel() override = default;

    QAbstractSeriesModel(const QAbstractSeriesModel&) = delete;
    QAbstractSeriesModel& operator=(const QAbstractSeriesModel&) = delete;
    QAbstractSeriesModel(QAbstractSeriesModel&&) = delete;
    QAbstractSeriesModel& operator=(QAbstractSeriesModel&&) = delete;

    // ── Queries ──────────────────────────────────────────────

    //! Number of points currently held by the model.
    [[nodiscard]] virtual qsizetype pointCount() const = 0;

    //! Point at @p index. Behavior for out-of-range @p index is undefined;
    //! callers must bounds-check via pointCount() first (AI.md §3.3:
    //! validators belong at API boundary, callers enforce preconditions).
    [[nodiscard]] virtual QPointF pointAt(qsizetype index) const = 0;

    //! Contiguous view of points [@p first, @p last] inclusive.
    //!
    //! Used by the rendering layer for incremental updates and by LOD /
    //! downsampling passes that need a window without copying. The returned
    //! span is invalidated by any mutating call on this model.
    //!
    //! @pre 0 <= @p first <= @p last < pointCount()
    [[nodiscard]] virtual PointSpan points(qsizetype first, qsizetype last) const = 0;

    //! Bounding box of all points in data coordinates. Used to autoscale
    //! axes. Implementations should cache and emit boundsChanged() when
    //! the box moves/grows. Returns a default-constructed QRectF when the
    //! model is empty.
    [[nodiscard]] virtual QRectF bounds() const = 0;

Q_SIGNALS:
    //! Points were inserted into the range [@p first, @p last] inclusive.
    //! Existing indices >= @p first have shifted right by (@p last - @p first + 1).
    void pointsInserted(qsizetype first, qsizetype last);

    //! Points were removed from the range [@p first, @p last] inclusive.
    void pointsRemoved(qsizetype first, qsizetype last);

    //! Existing points changed value (e.g. y-values updated in place)
    //! without count changing. @p first/@p last bound the dirty range;
    //! -1 means "all points".
    void dataChanged(qsizetype first, qsizetype last);

    //! The cached bounds() may have changed; consumers should re-query.
    void boundsChanged();

    //! Convenience signal emitted after any structural mutation
    //! (insert / remove / data change) for consumers that just want a
    //! "something changed" tick. Carries the new point count.
    void modelChanged(qsizetype newPointCount);
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_ABSTRACTSERIESMODEL_H
