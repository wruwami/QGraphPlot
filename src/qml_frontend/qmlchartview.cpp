#include "qmlchartview.h"

#include <QtCore/qmath.h>
#include <QtQuick/QSGSimpleRectNode>

#include <QtCore/QLoggingCategory>

namespace
{
Q_LOGGING_CATEGORY(lcChartView, "qgraphplot.chartview")
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
    if (!qIsFinite(val) || val >= m_xMax) {
        qCWarning(lcChartView) << "QmlChartView::setXMin: rejected non-finite or invalid xMin (must be < xMax):" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_xMin, val)) {
        m_xMin = val;
        emit xMinChanged();
        emit transformChanged();
    }
}

void QmlChartView::setXMax(double val)
{
    if (!qIsFinite(val) || val <= m_xMin) {
        qCWarning(lcChartView) << "QmlChartView::setXMax: rejected non-finite or invalid xMax (must be > xMin):" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_xMax, val)) {
        m_xMax = val;
        emit xMaxChanged();
        emit transformChanged();
    }
}

void QmlChartView::setYMin(double val)
{
    if (!qIsFinite(val) || val >= m_yMax) {
        qCWarning(lcChartView) << "QmlChartView::setYMin: rejected non-finite or invalid yMin (must be < yMax):" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_yMin, val)) {
        m_yMin = val;
        emit yMinChanged();
        emit transformChanged();
    }
}

void QmlChartView::setYMax(double val)
{
    if (!qIsFinite(val) || val <= m_yMin) {
        qCWarning(lcChartView) << "QmlChartView::setYMax: rejected non-finite or invalid yMax (must be > yMin):" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_yMax, val)) {
        m_yMax = val;
        emit yMaxChanged();
        emit transformChanged();
    }
}

void QmlChartView::setMarginLeft(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcChartView) << "QmlChartView::setMarginLeft: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginLeft, val)) {
        m_marginLeft = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void QmlChartView::setMarginRight(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcChartView) << "QmlChartView::setMarginRight: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginRight, val)) {
        m_marginRight = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void QmlChartView::setMarginTop(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcChartView) << "QmlChartView::setMarginTop: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginTop, val)) {
        m_marginTop = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void QmlChartView::setMarginBottom(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcChartView) << "QmlChartView::setMarginBottom: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginBottom, val)) {
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
