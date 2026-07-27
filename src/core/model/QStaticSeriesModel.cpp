// QGraphPlot ??High-performance Qt chart library
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

#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>

namespace qgraphplot
{

namespace
{
bool isFinitePoint(const QPointF& pt) noexcept
{
    return std::isfinite(pt.x()) && std::isfinite(pt.y());
}

bool spansOverlap(QSpan<const QPointF> pts, const std::vector<QPointF>& points) noexcept
{
    if (pts.isEmpty() || points.empty()) {
        return false;
    }

    const auto* inputBegin = pts.data();
    const auto* inputEnd = inputBegin + pts.size();
    const auto* storageBegin = points.data();
    const auto* storageEnd = storageBegin + points.size();
    return inputBegin < storageEnd && storageBegin < inputEnd;
}

bool hasFinitePoints(QSpan<const QPointF> pts) noexcept
{
    return std::all_of(pts.begin(), pts.end(), [](const QPointF& pt) { return isFinitePoint(pt); });
}
}  // namespace

// Construction / destruction

QStaticSeriesModel::QStaticSeriesModel(QObject* parent) : QAbstractSeriesModel(parent) {}

QStaticSeriesModel::~QStaticSeriesModel() = default;

// Queries

qsizetype QStaticSeriesModel::pointCount() const
{
    QMutexLocker locker(&m_mutex);
    return static_cast<qsizetype>(m_points.size());
}

QPointF QStaticSeriesModel::pointAt(qsizetype index) const
{
    QMutexLocker locker(&m_mutex);
    Q_ASSERT(index >= 0 && index < static_cast<qsizetype>(m_points.size()));
    return m_points[static_cast<size_t>(index)];
}

PointSpan QStaticSeriesModel::points(qsizetype first, qsizetype last) const
{
    QMutexLocker locker(&m_mutex);
    Q_ASSERT(first >= 0 && last >= first && last < static_cast<qsizetype>(m_points.size()));
    const qsizetype count = last - first + 1;
    return PointSpan(m_points.data() + first, count);
}

QRectF QStaticSeriesModel::bounds() const
{
    QMutexLocker locker(&m_mutex);
    return m_bounds;
}

// Bulk operations

void QStaticSeriesModel::setPoints(QSpan<const QPointF> pts)
{
    // Mutation happens under the lock; signals are emitted after it's released
    // below. Emitting while the mutex is held would deadlock a direct-connected
    // slot that re-enters this object — e.g. QAutoScaler-driven views call
    // pointCount()/bounds() from their boundsChanged() handler (QMutex is
    // non-recursive), even in the single-threaded case. Same pattern as
    // QRingBufferSeriesModel (see the note in its clear()).
    qsizetype oldSize = 0;
    qsizetype newSize = 0;
    {
        QMutexLocker locker(&m_mutex);

        // `snapshot` intentionally lives to the end of the block: when the
        // input span aliases m_points, `source` must keep pointing at valid
        // storage until the assign() below has consumed it. Reducing its scope
        // (as cppcheck suggests) would dangle `source`.
        QSpan<const QPointF> source = pts;
        std::vector<QPointF> snapshot;
        if (spansOverlap(pts, m_points)) {
            snapshot.assign(pts.begin(), pts.end());
            source = QSpan<const QPointF>(snapshot.data(), static_cast<qsizetype>(snapshot.size()));
        }

        if (!hasFinitePoints(source)) {
            qWarning("QStaticSeriesModel::setPoints: points must be finite");
            return;
        }

        oldSize = static_cast<qsizetype>(m_points.size());
        m_points.assign(source.begin(), source.end());
        newSize = static_cast<qsizetype>(m_points.size());

        recomputeBounds();
    }

    if (oldSize > 0) {
        Q_EMIT pointsRemoved(0, oldSize - 1);
    }
    if (newSize > 0) {
        Q_EMIT pointsInserted(0, newSize - 1);
    }
    Q_EMIT boundsChanged();
    Q_EMIT modelChanged(newSize);
}

void QStaticSeriesModel::appendBatch(QSpan<const QPointF> pts)
{
    if (pts.isEmpty()) {
        return;
    }

    // Mutation happens under the lock; signals are emitted after it's released
    // below. Emitting while the mutex is held would deadlock a direct-connected
    // slot that re-enters this object — e.g. QAutoScaler-driven views call
    // pointCount()/bounds() from their boundsChanged() handler (QMutex is
    // non-recursive), even in the single-threaded case. Same pattern as
    // QRingBufferSeriesModel (see the note in its clear()).
    qsizetype newFirst = 0;
    qsizetype newLast = 0;
    qsizetype newSize = 0;
    bool boundsShifted = false;
    {
        QMutexLocker locker(&m_mutex);

        // `snapshot` intentionally lives to the end of the block: when the
        // input span aliases m_points, `source` must keep pointing at valid
        // storage until the insert() below has consumed it. Reducing its scope
        // (as cppcheck suggests) would dangle `source`.
        QSpan<const QPointF> source = pts;
        std::vector<QPointF> snapshot;
        if (spansOverlap(pts, m_points)) {
            snapshot.assign(pts.begin(), pts.end());
            source = QSpan<const QPointF>(snapshot.data(), static_cast<qsizetype>(snapshot.size()));
        }

        if (!hasFinitePoints(source)) {
            qWarning("QStaticSeriesModel::appendBatch: points must be finite");
            return;
        }

        const qsizetype oldSize = static_cast<qsizetype>(m_points.size());
        const QRectF oldBounds = m_bounds;

        m_points.insert(m_points.end(), source.begin(), source.end());

        if (oldSize == 0) {
            recomputeBounds();
        } else {
            for (const QPointF& pt : source) {
                expandBounds(pt);
            }
        }

        newSize = static_cast<qsizetype>(m_points.size());
        newFirst = oldSize;
        newLast = newSize - 1;
        boundsShifted = (m_bounds != oldBounds);
    }

    Q_EMIT pointsInserted(newFirst, newLast);
    if (boundsShifted) {
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
    // Mutation happens under the lock; signals are emitted after it's released
    // below (see the note in setPoints() about direct-connected slots
    // re-entering this object while the mutex is held).
    bool boundsShifted = false;
    qsizetype newSize = 0;
    {
        QMutexLocker locker(&m_mutex);
        Q_ASSERT(index >= 0 && index < static_cast<qsizetype>(m_points.size()));
        if (!isFinitePoint(pt)) {
            qWarning("QStaticSeriesModel::replacePoint: point must be finite");
            return;
        }

        const QRectF oldBounds = m_bounds;
        m_points[static_cast<size_t>(index)] = pt;

        recomputeBounds();
        boundsShifted = (m_bounds != oldBounds);
        newSize = static_cast<qsizetype>(m_points.size());
    }

    Q_EMIT dataChanged(index, index);
    if (boundsShifted) {
        Q_EMIT boundsChanged();
    }
    Q_EMIT modelChanged(newSize);
}

void QStaticSeriesModel::clear()
{
    // Mutation happens under the lock; signals are emitted after it's released
    // below (see the note in setPoints() about direct-connected slots
    // re-entering this object while the mutex is held).
    qsizetype oldSize = 0;
    {
        QMutexLocker locker(&m_mutex);
        oldSize = static_cast<qsizetype>(m_points.size());

        m_points.clear();
        m_bounds = QRectF();
    }

    if (oldSize > 0) {
        Q_EMIT pointsRemoved(0, oldSize - 1);
    }
    Q_EMIT boundsChanged();
    Q_EMIT modelChanged(0);
}

void QStaticSeriesModel::reserve(qsizetype count)
{
    QMutexLocker locker(&m_mutex);
    if (count > 0) {
        m_points.reserve(static_cast<size_t>(count));
    }
}

// Bounds helpers

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
