#include "qmlchartview.h"

#include <QtCore/qmath.h>
#include <QtQuick/QSGSimpleRectNode>

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
    // 배경(전체 아이템)과 플롯 영역(마진 제외 영역)을 테마 색상으로 직접 렌더링한다.
    // 테마가 없으면 updatePaintNode가 nullptr을 반환하여 투명하게 유지된다.
    setFlag(ItemHasContents, true);
}

void QmlChartView::setTheme(qgraphplot::QGraphPlotTheme* theme)
{
    if (m_theme == theme) {
        return;
    }
    if (m_theme) {
        disconnect(m_theme, nullptr, this, nullptr);
    }
    m_theme = theme;
    applyThemeConnections();
    emit themeChanged();
    update();
}

void QmlChartView::applyThemeConnections()
{
    if (!m_theme) {
        return;
    }
    connect(m_theme,
            &qgraphplot::QGraphPlotTheme::themeChanged,
            this,
            &QQuickItem::update,
            Qt::UniqueConnection);
    // A theme destroyed while still assigned must repaint (QPointer nulls
    // itself, so the next updatePaintNode drops the background nodes).
    connect(m_theme, &QObject::destroyed, this, [this]() {
        emit themeChanged();
        update();
    });
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
        update();
    }
}

void QmlChartView::setMarginRight(double val)
{
    if (valuesDiffer(m_marginRight, val)) {
        m_marginRight = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void QmlChartView::setMarginTop(double val)
{
    if (valuesDiffer(m_marginTop, val)) {
        m_marginTop = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void QmlChartView::setMarginBottom(double val)
{
    if (valuesDiffer(m_marginBottom, val)) {
        m_marginBottom = val;
        emit marginsChanged();
        emit transformChanged();
        update();
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
        update();
    }
}

QSGNode* QmlChartView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData)
{
    Q_UNUSED(updateData);

    if (!m_theme || width() <= 0.0 || height() <= 0.0) {
        delete oldNode;
        return nullptr;
    }

    // 노드 트리: root ─┬─ background (아이템 전체)
    //                  └─ plotArea   (마진 제외 영역)
    QSGNode* root = oldNode;
    QSGSimpleRectNode* background = nullptr;
    QSGSimpleRectNode* plotAreaNode = nullptr;

    if (!root) {
        root = new QSGNode();
        background = new QSGSimpleRectNode();
        plotAreaNode = new QSGSimpleRectNode();
        root->appendChildNode(background);
        root->appendChildNode(plotAreaNode);
    } else {
        background = static_cast<QSGSimpleRectNode*>(root->firstChild());
        plotAreaNode = static_cast<QSGSimpleRectNode*>(root->lastChild());
    }

    background->setRect(QRectF(0.0, 0.0, width(), height()));
    background->setColor(m_theme->backgroundColor());

    plotAreaNode->setRect(plotArea());
    plotAreaNode->setColor(m_theme->plotAreaColor());

    return root;
}
