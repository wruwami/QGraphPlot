#include "qmlaxis.h"

#include <cmath>

#include <QtCore/QVariantMap>
#include <QtQuick/QSGFlatColorMaterial>
#include <QtQuick/QSGGeometryNode>

#include "qmlchartview.h"

namespace
{

//! Length of a tick mark sticking out of the axis baseline, in pixels.
constexpr float kTickLength = 5.0f;

//! Creates or refreshes a DrawLines node holding @p vertexCount vertices in
//! @p color at @p width. Axis chrome and grid lines live in separate nodes
//! because a QSGFlatColorMaterial node carries exactly one color and one line
//! width — sharing a node is what made gridColor/gridWidth unobservable.
QSGGeometryNode*
refreshLineNode(QSGGeometryNode* node, int vertexCount, const QColor& color, double width)
{
    if (!node) {
        node = new QSGGeometryNode();
        QSGGeometry* newGeometry =
            new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        newGeometry->setDrawingMode(QSGGeometry::DrawLines);
        node->setGeometry(newGeometry);
        node->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial* material = new QSGFlatColorMaterial();
        material->setColor(color);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    } else {
        QSGFlatColorMaterial* material = static_cast<QSGFlatColorMaterial*>(node->material());
        if (material && material->color() != color) {
            material->setColor(color);
            node->markDirty(QSGNode::DirtyMaterial);
        }
    }

    QSGGeometry* geometry = node->geometry();
    if (geometry->vertexCount() != vertexCount) {
        geometry->allocate(vertexCount);
    }
    geometry->setLineWidth(static_cast<float>(width));
    return node;
}

}  // namespace

QmlAxis::QmlAxis(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

QVariantList QmlAxis::ticks() const noexcept
{
    QVariantList list;
    for (const auto& tick : m_tickInfos) {
        QVariantMap map;
        map["label"] = tick.label;
        map["position"] = tick.position;
        list.append(map);
    }
    return list;
}

void QmlAxis::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation != orientation) {
        m_orientation = orientation;
        emit orientationChanged();
        updateTicks();
    }
}

void QmlAxis::setTickCount(int count)
{
    if (m_tickCount != count && count > 0) {
        m_tickCount = count;
        emit tickCountChanged();
        updateTicks();
    }
}

void QmlAxis::setColor(const QColor& color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged();
        update();
    }
}

void QmlAxis::setGridColor(const QColor& gridColor)
{
    if (m_gridColor != gridColor) {
        m_gridColor = gridColor;
        emit gridColorChanged();
        update();
    }
}

void QmlAxis::setLineWidth(double lineWidth)
{
    if (!std::isfinite(lineWidth) || lineWidth <= 0.0) {
        qWarning("QmlAxis::setLineWidth: lineWidth must be finite and > 0");
        return;
    }
    if (!qFuzzyCompare(m_lineWidth, lineWidth)) {
        m_lineWidth = lineWidth;
        emit lineWidthChanged();
        update();
    }
}

void QmlAxis::setGridWidth(double gridWidth)
{
    if (!std::isfinite(gridWidth) || gridWidth <= 0.0) {
        qWarning("QmlAxis::setGridWidth: gridWidth must be finite and > 0");
        return;
    }
    if (!qFuzzyCompare(m_gridWidth, gridWidth)) {
        m_gridWidth = gridWidth;
        emit gridWidthChanged();
        update();
    }
}

void QmlAxis::setShowGrid(bool show)
{
    if (m_showGrid != show) {
        m_showGrid = show;
        emit showGridChanged();
        updateTicks();  // 틱 개수나 지오메트리 크기 영향을 재계산
    }
}

void QmlAxis::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        connectChartViewSignals();
        updateTicks();
    }
}

void QmlAxis::connectChartViewSignals()
{
    if (m_previousChartView) {
        disconnect(m_previousChartView,
                   &QmlChartView::transformChanged,
                   this,
                   &QmlAxis::handleTransformChanged);
    }

    QmlChartView* chartView = qobject_cast<QmlChartView*>(parentItem());
    if (chartView) {
        connect(chartView,
                &QmlChartView::transformChanged,
                this,
                &QmlAxis::handleTransformChanged,
                Qt::UniqueConnection);
        m_previousChartView = chartView;
    } else {
        m_previousChartView = nullptr;
    }
}

void QmlAxis::handleTransformChanged()
{
    updateTicks();
}

void QmlAxis::updateTicks()
{
    QmlChartView* chartView = qobject_cast<QmlChartView*>(parentItem());
    if (!chartView) {
        return;
    }

    const double min = (m_orientation == Qt::Horizontal) ? chartView->xMin() : chartView->yMin();
    const double max = (m_orientation == Qt::Horizontal) ? chartView->xMax() : chartView->yMax();

    if (qFuzzyCompare(min, max)) {
        m_tickInfos.clear();
    } else {
        m_tickInfos = qgraphplot::QScaleEngine::calculateTicks(min, max, m_tickCount);
    }
    emit ticksChanged();
    update();
}

QSGNode* QmlAxis::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData)
{
    Q_UNUSED(updateData);

    QmlChartView* chartView = qobject_cast<QmlChartView*>(parentItem());
    if (!chartView || m_tickInfos.empty()) {
        delete oldNode;
        return nullptr;
    }

    const qgraphplot::QCoordinateTransform transform = chartView->coordinateTransform();
    const QRectF pixelRect = transform.pixelRect();

    const size_t tickCount = m_tickInfos.size();
    if (tickCount > static_cast<size_t>(INT_MAX / 4) - 1) {
        delete oldNode;
        return nullptr;
    }

    // 축 기준선 1개 + 틱 눈금선 tickCount개 → 축 노드
    // 그리드선 tickCount개 → 그리드 노드 (색상/두께가 축과 다르므로 별도 노드)
    const int axisVertexCount = static_cast<int>((1 + tickCount) * 2);
    const int gridVertexCount = static_cast<int>(tickCount * 2);

    // 노드 트리: root ─┬─ grid (showGrid일 때만, 축보다 먼저 그려짐)
    //                  └─ axis
    QSGNode* root = oldNode ? oldNode : new QSGNode();
    QSGGeometryNode* gridNode = nullptr;
    QSGGeometryNode* axisNode = nullptr;
    if (root->childCount() == 2) {
        gridNode = static_cast<QSGGeometryNode*>(root->firstChild());
        axisNode = static_cast<QSGGeometryNode*>(root->lastChild());
    } else if (root->childCount() == 1) {
        axisNode = static_cast<QSGGeometryNode*>(root->firstChild());
    }

    if (!m_showGrid && gridNode) {
        root->removeChildNode(gridNode);
        delete gridNode;
        gridNode = nullptr;
    }

    const bool axisNodeIsNew = (axisNode == nullptr);
    axisNode = refreshLineNode(axisNode, axisVertexCount, m_color, m_lineWidth);
    if (axisNodeIsNew) {
        root->appendChildNode(axisNode);
    }

    if (m_showGrid) {
        const bool gridNodeIsNew = (gridNode == nullptr);
        gridNode = refreshLineNode(gridNode, gridVertexCount, m_gridColor, m_gridWidth);
        if (gridNodeIsNew) {
            root->prependChildNode(gridNode);
        }
    }

    QSGGeometry::Point2D* axisVertices = axisNode->geometry()->vertexDataAsPoint2D();
    QSGGeometry::Point2D* gridVertices =
        gridNode ? gridNode->geometry()->vertexDataAsPoint2D() : nullptr;
    int axisIdx = 0;
    int gridIdx = 0;

    // 1. 축 기준선 (Border line)
    if (m_orientation == Qt::Horizontal) {
        const float y = static_cast<float>(pixelRect.bottom());
        axisVertices[axisIdx++].set(static_cast<float>(pixelRect.left()), y);
        axisVertices[axisIdx++].set(static_cast<float>(pixelRect.right()), y);
    } else {
        const float x = static_cast<float>(pixelRect.left());
        axisVertices[axisIdx++].set(x, static_cast<float>(pixelRect.top()));
        axisVertices[axisIdx++].set(x, static_cast<float>(pixelRect.bottom()));
    }

    // 2. 눈금선 및 그리드선 배치
    for (const auto& tick : m_tickInfos) {
        if (m_orientation == Qt::Horizontal) {
            const QPointF p = transform.toPixel(QPointF(tick.position, 0.0));
            const float x = static_cast<float>(p.x());
            const float borderY = static_cast<float>(pixelRect.bottom());

            // 눈금선 (아래쪽 5px)
            axisVertices[axisIdx++].set(x, borderY);
            axisVertices[axisIdx++].set(x, borderY + kTickLength);

            // 그리드선 (뷰포트 수직선)
            if (gridVertices) {
                gridVertices[gridIdx++].set(x, borderY);
                gridVertices[gridIdx++].set(x, static_cast<float>(pixelRect.top()));
            }
        } else {
            const QPointF p = transform.toPixel(QPointF(0.0, tick.position));
            const float y = static_cast<float>(p.y());
            const float borderX = static_cast<float>(pixelRect.left());

            // 눈금선 (왼쪽 5px)
            axisVertices[axisIdx++].set(borderX, y);
            axisVertices[axisIdx++].set(borderX - kTickLength, y);

            // 그리드선 (뷰포트 수평선)
            if (gridVertices) {
                gridVertices[gridIdx++].set(borderX, y);
                gridVertices[gridIdx++].set(static_cast<float>(pixelRect.right()), y);
            }
        }
    }

    axisNode->markDirty(QSGNode::DirtyGeometry);
    if (gridNode) {
        gridNode->markDirty(QSGNode::DirtyGeometry);
    }
    return root;
}
