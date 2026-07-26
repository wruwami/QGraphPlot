#include "qmlchartview.h"

#include <QtCore/qmath.h>

namespace
{
//! qFuzzyCompare is a *relative*-epsilon comparison, so it misbehaves
//! whenever either operand is exactly (or very near) zero: any nonzero
//! difference against 0.0 is reported as "not equal", never "fuzzy equal"
//! (see Qt's own docs: "comparing values where either p1 or p2 is 0.0 will
//! not work"). QmlChartView's axes commonly start at/near 0 (e.g. the
//! streaming demo's xMin), so every setter needs this near-zero-safe rule,
//! not just one of them (#35).
bool valuesDiffer(double a, double b) noexcept
{
    constexpr double kNearZeroEpsilon = 1e-10;
    if (!qIsFinite(a) || !qIsFinite(b)) {
        return true;  // Treat NaN/Inf as always different
    }
    if (qAbs(a) < kNearZeroEpsilon || qAbs(b) < kNearZeroEpsilon) {
        return qAbs(a - b) > kNearZeroEpsilon;
    }
    return !qFuzzyCompare(a, b);
}
}  // namespace

QmlChartView::QmlChartView(QQuickItem* parent) : QQuickItem(parent)
{
    // QQuickItem의 QSG 노드를 사용해 배경 등을 렌더링하고 싶을 경우 활용할 수 있으나,
    // 현재는 자식 렌더 아이템들의 레이아웃 및 뷰포트 영역 정보 관리를 주 목적으로 함.
    setFlag(ItemHasContents, false);
}

void QmlChartView::setXMin(double val)
{
    if (valuesDiffer(m_xMin, val)) {
        m_xMin = val;
        emit xMinChanged();
        emit transformChanged();
    }
}

void QmlChartView::setXMax(double val)
{
    if (valuesDiffer(m_xMax, val)) {
        m_xMax = val;
        emit xMaxChanged();
        emit transformChanged();
    }
}

void QmlChartView::setYMin(double val)
{
    if (valuesDiffer(m_yMin, val)) {
        m_yMin = val;
        emit yMinChanged();
        emit transformChanged();
    }
}

void QmlChartView::setYMax(double val)
{
    if (valuesDiffer(m_yMax, val)) {
        m_yMax = val;
        emit yMaxChanged();
        emit transformChanged();
    }
}

void QmlChartView::setMarginLeft(double val)
{
    if (valuesDiffer(m_marginLeft, val)) {
        m_marginLeft = val;
        emit marginsChanged();
        emit transformChanged();
    }
}

void QmlChartView::setMarginRight(double val)
{
    if (valuesDiffer(m_marginRight, val)) {
        m_marginRight = val;
        emit marginsChanged();
        emit transformChanged();
    }
}

void QmlChartView::setMarginTop(double val)
{
    if (valuesDiffer(m_marginTop, val)) {
        m_marginTop = val;
        emit marginsChanged();
        emit transformChanged();
    }
}

void QmlChartView::setMarginBottom(double val)
{
    if (valuesDiffer(m_marginBottom, val)) {
        m_marginBottom = val;
        emit marginsChanged();
        emit transformChanged();
    }
}

qgraphplot::QCoordinateTransform QmlChartView::coordinateTransform() const noexcept
{
    const QRectF dataBounds(m_xMin, m_yMin, m_xMax - m_xMin, m_yMax - m_yMin);
    const double pixelWidth = qMax(0.0, width() - m_marginLeft - m_marginRight);
    const double pixelHeight = qMax(0.0, height() - m_marginTop - m_marginBottom);
    const QRectF pixelRect(m_marginLeft, m_marginTop, pixelWidth, pixelHeight);
    return qgraphplot::QCoordinateTransform(dataBounds, pixelRect);
}

QPointF QmlChartView::mapToPixel(double x, double y) const noexcept
{
    return coordinateTransform().toPixel(QPointF(x, y));
}

void QmlChartView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        emit transformChanged();
    }
}
