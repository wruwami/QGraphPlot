#include "qmlaxis.h"

#include <QtCore/QVariantMap>
#include <QtQuick/QSGFlatColorMaterial>
#include <QtQuick/QSGGeometryNode>

#include "qmlchartview.h"

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

    // 렌더링 세그먼트 개수 계산. 축 기준선 + 눈금선은 m_color로, 그리드선은
    // m_gridColor로 그려야 하므로(#35) 두 개의 QSGGeometryNode로 분리한다.
    const size_t tickCount = m_tickInfos.size();
    const size_t axisLineCount = 1 + tickCount;               // 기준선(1) + 눈금선(tickCount)
    const size_t gridLineCount = m_showGrid ? tickCount : 0;  // 그리드선
    if (axisLineCount > static_cast<size_t>(INT_MAX / 2) ||
        gridLineCount > static_cast<size_t>(INT_MAX / 2)) {
        delete oldNode;
        return nullptr;
    }
    const int axisVertexCount = static_cast<int>(axisLineCount * 2);
    const int gridVertexCount = static_cast<int>(gridLineCount * 2);

    QSGGeometryNode* axisNode = static_cast<QSGGeometryNode*>(oldNode);
    QSGGeometryNode* gridNode = nullptr;
    QSGGeometry* axisGeometry = nullptr;
    QSGGeometry* gridGeometry = nullptr;

    if (!axisNode) {
        axisNode = new QSGGeometryNode();
        axisGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), axisVertexCount);
        axisGeometry->setLineWidth(1.0f);
        axisGeometry->setDrawingMode(QSGGeometry::DrawLines);
        axisNode->setGeometry(axisGeometry);
        axisNode->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial* axisMaterial = new QSGFlatColorMaterial();
        axisMaterial->setColor(m_color);
        axisNode->setMaterial(axisMaterial);
        axisNode->setFlag(QSGNode::OwnsMaterial);

        gridNode = new QSGGeometryNode();
        gridGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), gridVertexCount);
        gridGeometry->setLineWidth(1.0f);
        gridGeometry->setDrawingMode(QSGGeometry::DrawLines);
        gridNode->setGeometry(gridGeometry);
        gridNode->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial* gridMaterial = new QSGFlatColorMaterial();
        gridMaterial->setColor(m_gridColor);
        gridNode->setMaterial(gridMaterial);
        gridNode->setFlag(QSGNode::OwnsMaterial);

        axisNode->appendChildNode(gridNode);
    } else {
        axisGeometry = axisNode->geometry();
        if (axisGeometry->vertexCount() != axisVertexCount) {
            axisGeometry->allocate(axisVertexCount);
        }
        QSGFlatColorMaterial* axisMaterial =
            static_cast<QSGFlatColorMaterial*>(axisNode->material());
        if (axisMaterial) {
            axisMaterial->setColor(m_color);
            axisNode->markDirty(QSGNode::DirtyMaterial);
        }

        gridNode = static_cast<QSGGeometryNode*>(axisNode->childAtIndex(0));
        gridGeometry = gridNode->geometry();
        if (gridGeometry->vertexCount() != gridVertexCount) {
            gridGeometry->allocate(gridVertexCount);
        }
        QSGFlatColorMaterial* gridMaterial =
            static_cast<QSGFlatColorMaterial*>(gridNode->material());
        if (gridMaterial) {
            gridMaterial->setColor(m_gridColor);
            gridNode->markDirty(QSGNode::DirtyMaterial);
        }
    }

    QSGGeometry::Point2D* axisVertices = axisGeometry->vertexDataAsPoint2D();
    QSGGeometry::Point2D* gridVertices =
        gridVertexCount > 0 ? gridGeometry->vertexDataAsPoint2D() : nullptr;
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
    const float tickLength = 5.0f;

    for (const auto& tick : m_tickInfos) {
        if (m_orientation == Qt::Horizontal) {
            const QPointF p = transform.toPixel(QPointF(tick.position, 0.0));
            const float x = static_cast<float>(p.x());
            const float borderY = static_cast<float>(pixelRect.bottom());

            // 눈금선 (아래쪽 5px)
            axisVertices[axisIdx++].set(x, borderY);
            axisVertices[axisIdx++].set(x, borderY + tickLength);

            // 그리드선 (뷰포트 수직선)
            if (m_showGrid) {
                gridVertices[gridIdx++].set(x, static_cast<float>(pixelRect.bottom()));
                gridVertices[gridIdx++].set(x, static_cast<float>(pixelRect.top()));
            }
        } else {
            const QPointF p = transform.toPixel(QPointF(0.0, tick.position));
            const float y = static_cast<float>(p.y());
            const float borderX = static_cast<float>(pixelRect.left());

            // 눈금선 (왼쪽 5px)
            axisVertices[axisIdx++].set(borderX, y);
            axisVertices[axisIdx++].set(borderX - tickLength, y);

            // 그리드선 (뷰포트 수평선)
            if (m_showGrid) {
                gridVertices[gridIdx++].set(static_cast<float>(pixelRect.left()), y);
                gridVertices[gridIdx++].set(static_cast<float>(pixelRect.right()), y);
            }
        }
    }

    axisNode->markDirty(QSGNode::DirtyGeometry);
    gridNode->markDirty(QSGNode::DirtyGeometry);
    return axisNode;
}
