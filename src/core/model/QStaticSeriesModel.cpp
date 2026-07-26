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

#include "QStaticSeriesModel.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace qgraphplot
{

// ─────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────

QStaticSeriesModel::QStaticSeriesModel(QObject* parent) : QAbstractSeriesModel(parent) {}

QStaticSeriesModel::~QStaticSeriesModel() = default;

// ─────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────

qsizetype QStaticSeriesModel::pointCount() const
{
    return static_cast<qsizetype>(m_points.size());
}

QPointF QStaticSeriesModel::pointAt(qsizetype index) const
{
    Q_ASSERT(index >= 0 && index < pointCount());
    return m_points[static_cast<size_t>(index)];
}

PointSpan QStaticSeriesModel::points(qsizetype first, qsizetype last) const
{
    Q_ASSERT(first >= 0 && last >= first && last < pointCount());
    const qsizetype count = last - first + 1;
    return PointSpan(m_points.data() + first, count);
}

QRectF QStaticSeriesModel::bounds() const
{
    return m_bounds;
}

// ─────────────────────────────────────────────────────────────
// Bulk operations
// ─────────────────────────────────────────────────────────────

void QStaticSeriesModel::setPoints(QSpan<const QPointF> pts)
{
    const qsizetype oldSize = pointCount();

    m_points.assign(pts.begin(), pts.end());

    const qsizetype newSize = pointCount();

    recomputeBounds();

    // Emit removal for old data (if any)
    if (oldSize > 0) {
        Q_EMIT pointsRemoved(0, oldSize - 1);
    }
    // Emit insertion for new data (if any)
    if (newSize > 0) {
        Q_EMIT pointsInserted(0, newSize - 1);
    }
    Q_EMIT boundsChanged();
    Q_EMIT modelChanged(newSize);
}

void QStaticSeriesModel::appendBatch(QSpan<const QPointF> pts)
{
    if (pts.isEmpty()) {
        return;  // AI.md §3.3: explicit no-op on empty input.
    }

    const qsizetype oldSize = pointCount();
    const QRectF oldBounds = m_bounds;

    m_points.insert(m_points.end(), pts.begin(), pts.end());

    // Incrementally expand bounds for new points.
    for (const QPointF& pt : pts) {
        expandBounds(pt);
    }

    const qsizetype newSize = pointCount();
    const qsizetype newFirst = oldSize;
    const qsizetype newLast = newSize - 1;

    Q_EMIT pointsInserted(newFirst, newLast);
    if (m_bounds != oldBounds) {
        Q_EMIT boundsChanged();
    }
    Q_EMIT modelChanged(newSize);
}

void QStaticSeriesModel::append(QPointF pt)
{
    appendBatch(QSpan<const QPointF>(&pt, 1));
}

void QStaticSeriesModel::replacePoint(qsizetype index, QPointF pt)
{
    Q_ASSERT(index >= 0 && index < pointCount());

    const QRectF oldBounds = m_bounds;
    m_points[static_cast<size_t>(index)] = pt;

    // A replacement can shrink or shift the bounds (unlike append which only
    // expands), so we must recompute from scratch. O(n) but replacePoint is
    // expected to be infrequent compared to batch operations.
    recomputeBounds();

    Q_EMIT dataChanged(index, index);
    if (m_bounds != oldBounds) {
        Q_EMIT boundsChanged();
    }
    Q_EMIT modelChanged(pointCount());
}

void QStaticSeriesModel::clear()
{
    const qsizetype oldSize = pointCount();

    m_points.clear();
    m_bounds = QRectF();

    if (oldSize > 0) {
        Q_EMIT pointsRemoved(0, oldSize - 1);
    }
    Q_EMIT boundsChanged();
    Q_EMIT modelChanged(0);
}

void QStaticSeriesModel::reserve(qsizetype count)
{
    if (count > 0) {
        m_points.reserve(static_cast<size_t>(count));
    }
}

// ─────────────────────────────────────────────────────────────
// Bounds helpers
// ─────────────────────────────────────────────────────────────

void QStaticSeriesModel::recomputeBounds() noexcept
{
    if (m_points.empty()) {
        m_bounds = QRectF();
        return;
    }

    qreal minX = m_points[0].x();
    qreal maxX = minX;
    qreal minY = m_points[0].y();
    qreal maxY = minY;

    for (size_t i = 1; i < m_points.size(); ++i) {
        const qreal x = m_points[i].x();
        const qreal y = m_points[i].y();
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }

    m_bounds = QRectF(minX, minY, maxX - minX, maxY - minY);
}

void QStaticSeriesModel::expandBounds(QPointF pt) noexcept
{
    if (m_bounds.isNull() && m_points.size() == 1) {
        // First point: initialize bounds as zero-area rect at this point.
        m_bounds = QRectF(pt.x(), pt.y(), 0.0, 0.0);
        return;
    }

    const qreal x = pt.x();
    const qreal y = pt.y();

    qreal left = m_bounds.left();
    qreal top = m_bounds.top();
    qreal right = m_bounds.right();
    qreal bottom = m_bounds.bottom();

    bool changed = false;

    if (x < left) {
        left = x;
        changed = true;
    } else if (x > right) {
        right = x;
        changed = true;
    }

    if (y < top) {
        top = y;
        changed = true;
    } else if (y > bottom) {
        bottom = y;
        changed = true;
    }

    if (changed) {
        m_bounds = QRectF(left, top, right - left, bottom - top);
    }
}

}  // namespace qgraphplot
