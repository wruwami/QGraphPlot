#include "QmlLegend.h"

#include <QtCore/QLoggingCategory>
#include <QtQuick/QSGSimpleRectNode>
#include <QtQuick/QSGTextNode>

#include "QmlChartView.h"

namespace qgraphplot
{

namespace
{
Q_LOGGING_CATEGORY(lcLegend, "qgraphplot.legend")
}  // namespace

// ── QmlLegendItem ─────────────────────────────────────────────

QmlLegendItem::QmlLegendItem(QObject* parent) : QObject(parent)
{
}

void QmlLegendItem::toggle()
{
    if (!m_series) {
        return;
    }
    m_series->setVisible(!m_series->isVisible());
}

void QmlLegendItem::setSeries(qgraphplot::QAbstractSeries* series)
{
    if (m_series == series) {
        return;
    }
    if (m_series) {
        disconnect(m_series, nullptr, this, nullptr);
    }
    m_series = series;
    if (series) {
        m_name = series->name();
        m_color = series->color();
        m_visible = series->isVisible();
        connect(series, &QAbstractSeries::nameChanged, this,
                &QmlLegendItem::onSeriesNameChanged);
        connect(series, &QAbstractSeries::colorChanged, this,
                &QmlLegendItem::onSeriesColorChanged);
        connect(series, &QAbstractSeries::visibleChanged, this,
                &QmlLegendItem::onSeriesVisibleChanged);
    } else {
        m_name.clear();
        m_color = Qt::transparent;
        m_visible = true;
    }
    emit nameChanged();
    emit colorChanged();
    emit visibleChanged();
}

void QmlLegendItem::onSeriesNameChanged(const QString& name)
{
    m_name = name;
    emit nameChanged();
}

void QmlLegendItem::onSeriesColorChanged(const QColor& color)
{
    m_color = color;
    emit colorChanged();
}

void QmlLegendItem::onSeriesVisibleChanged(bool visible)
{
    m_visible = visible;
    emit visibleChanged();
}

// ── QmlLegend ────────────────────────────────────────────────

QmlLegend::QmlLegend(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void QmlLegend::setChart(qgraphplot::QmlChartView* chart)
{
    if (m_chart == chart) {
        return;
    }
    if (m_chart) {
        disconnect(m_chart, nullptr, this, nullptr);
        for (auto* item : std::as_const(m_items)) {
            item->deleteLater();
        }
        m_items.clear();
    }
    m_chart = chart;
    connectChartSignals();
    rebuildItems();
    emit chartChanged();
    emit itemsChanged();
    update();
}

void QmlLegend::connectChartSignals()
{
    if (!m_chart) {
        return;
    }
    connect(m_chart, &QmlChartView::seriesAdded, this, &QmlLegend::onChartSeriesAdded);
    connect(m_chart, &QmlChartView::seriesRemoved, this, &QmlLegend::onChartSeriesRemoved);
    if (auto* theme = m_chart->theme()) {
        m_cachedTheme = theme;
        connect(theme, &QGraphPlotTheme::themeChanged, this, &QmlLegend::onThemeChanged);
    }
}

void QmlLegend::rebuildItems()
{
    for (auto* item : std::as_const(m_items)) {
        item->deleteLater();
    }
    m_items.clear();
    if (!m_chart) {
        return;
    }
    const auto seriesList = m_chart->series();
    for (auto* series : seriesList) {
        auto* legendItem = new QmlLegendItem(this);
        legendItem->setSeries(series);
        m_items.append(legendItem);
    }
}

void QmlLegend::onChartSeriesAdded(qgraphplot::QAbstractSeries* series)
{
    Q_UNUSED(series);
    rebuildItems();
    emit itemsChanged();
    update();
}

void QmlLegend::onChartSeriesRemoved(qgraphplot::QAbstractSeries* series)
{
    Q_UNUSED(series);
    rebuildItems();
    emit itemsChanged();
    update();
}

void QmlLegend::onThemeChanged()
{
    update();
}

QVariantList QmlLegend::items() const noexcept
{
    QVariantList list;
    for (auto* item : m_items) {
        list.append(QVariant::fromValue(item));
    }
    return list;
}

void QmlLegend::setPosition(Qt::Alignment position)
{
    if (m_position == position) {
        return;
    }
    m_position = position;
    emit positionChanged();
    update();
}

void QmlLegend::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        connectChartSignals();
    }
}

QSGNode* QmlLegend::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData)
{
    Q_UNUSED(updateData);

    if (m_items.isEmpty() || width() <= 0.0 || height() <= 0.0) {
        delete oldNode;
        return nullptr;
    }

    QSGNode* root = oldNode;
    if (!root) {
        root = new QSGNode();
    }

    // Simple background rect for the legend box
    QSGSimpleRectNode* bgNode = nullptr;
    if (root->firstChild()) {
        bgNode = static_cast<QSGSimpleRectNode*>(root->firstChild());
    } else {
        bgNode = new QSGSimpleRectNode();
        root->appendChildNode(bgNode);
    }

    // Compute legend box geometry based on position
    const qreal padding = 8.0;
    const qreal markerSize = 16.0;
    const qreal textHeight = 14.0;
    const qreal itemHeight = markerSize + padding;
    const qreal totalHeight = m_items.size() * itemHeight + padding * 2;
    const qreal boxWidth = 120.0;  // Fixed width for simplicity

    qreal x = padding;
    qreal y = padding;
    if (m_position & Qt::AlignBottom) {
        y = height() - totalHeight - padding;
    } else if (m_position & Qt::AlignVCenter) {
        y = (height() - totalHeight) / 2;
    }
    if (m_position & Qt::AlignRight) {
        x = width() - boxWidth - padding;
    } else if (m_position & Qt::AlignHCenter) {
        x = (width() - boxWidth) / 2;
    }

    bgNode->setRect(QRectF(x, y, boxWidth, totalHeight));
    bgNode->setColor(QColor(255, 255, 255, 200));  // Semi-transparent white

    // Clear old text/marker nodes
    while (root->firstChild() != bgNode) {
        delete root->lastChild();
    }

    // Render each legend item (marker + name)
    for (int i = 0; i < m_items.size(); ++i) {
        auto* item = m_items.at(i);
        const qreal itemY = y + padding + i * itemHeight;

        // Color marker rect
        QSGSimpleRectNode* marker = new QSGSimpleRectNode();
        marker->setRect(QRectF(x + padding, itemY, markerSize, markerSize));
        marker->setColor(item->color());
        root->appendChildNode(marker);

        // Series name text (simplified - in production would use QSGTextNode)
        // For now we skip text rendering complexity; the QML Legend.qml will handle it
    }

    return root;
}

}  // namespace qgraphplot
