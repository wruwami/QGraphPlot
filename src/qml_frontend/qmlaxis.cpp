#include "qmlaxis.h"

#include <QtCore/QVariantMap>
#include <QtQuick/QSGFlatColorMaterial>
#include <QtQuick/QSGGeometryNode>

#include "qmlchartview.h"

namespace
{
//! Whether two computed tick sets are identical. Ticks are recomputed
//! deterministically from (min, max, tickCount), so an unchanged input
//! range/count produces bit-identical output -- plain equality (not a
//! fuzzy comparison) is the right tool here (#36).
bool tickInfosEqual(const std::vector<qgraphplot::QScaleEngine::TickInfo>& a,
                    const std::vector<qgraphplot::QScaleEngine::TickInfo>& b) noexcept
{
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].position != b[i].position || a[i].label != b[i].label) {
            return false;
        }
    }
    return true;
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

    std::vector<qgraphplot::QScaleEngine::TickInfo> newTicks;
    if (!qFuzzyCompare(min, max)) {
        newTicks = qgraphplot::QScaleEngine::calculateTicks(min, max, m_tickCount);
    }

    // Skip the emit (and the QML Repeater's full delegate rebuild it
    // triggers) when the tick set didn't actually change -- important during
    // streaming, where updateTicks() runs almost every frame but the visible
    // "nice" tick values often don't move every single frame (#36).
    //
    // However, even when tick data is unchanged, we must emit if the axis
    // range (min/max) is also unchanged, because that indicates a
    // geometry-only transform change (e.g., window resize, margin adjust).
    // ChartView.qml delegates use root.mapToPixel() in position bindings,
    // which doesn't auto-track dependencies, so they only reposition when
    // ticksChanged() forces Repeater model change.
    const bool ticksEqual = tickInfosEqual(newTicks, m_tickInfos);
    // Use absolute epsilon comparison for range equality (simpler and more
    // robust than qFuzzyCompare, which fails when values are near zero).
    const bool rangeEqual = (qAbs(min - m_previousMin) < 1e-10) && (qAbs(max - m_previousMax) < 1e-10);
    const bool geometryOnlyChange = (ticksEqual && rangeEqual && !m_tickInfos.empty());

    if (!ticksEqual || geometryOnlyChange) {
        if (!ticksEqual) {
            m_tickInfos = std::move(newTicks);
        }
        emit ticksChanged();
    }

    m_previousMin = min;
    m_previousMax = max;
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

    // 렌더링 세그먼트 개수 계산
    // 1. 축 기준선 (1개)
    // 2. 틱 눈금선 (ticks.size() 개)
    // 3. 그리드선 (m_showGrid ? ticks.size() 개 : 0)
    const size_t tickCount = m_tickInfos.size();
    const size_t lineCount = 1 + tickCount + (m_showGrid ? tickCount : 0);
    if (lineCount > static_cast<size_t>(INT_MAX / 2)) {
        delete oldNode;
        return nullptr;
    }
    const int vertexCount = static_cast<int>(lineCount * 2);

    QSGGeometryNode* node = static_cast<QSGGeometryNode*>(oldNode);
    QSGGeometry* geometry = nullptr;

    if (!node) {
        node = new QSGGeometryNode();
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        geometry->setLineWidth(1.0f);
        geometry->setDrawingMode(QSGGeometry::DrawLines);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial* material = new QSGFlatColorMaterial();
        material->setColor(m_color);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    } else {
        geometry = node->geometry();
        if (geometry->vertexCount() != vertexCount) {
            geometry->allocate(vertexCount);
        }
        QSGFlatColorMaterial* material = static_cast<QSGFlatColorMaterial*>(node->material());
        if (material) {
            material->setColor(m_color);
            node->markDirty(QSGNode::DirtyMaterial);
        }
    }

    QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();
    int idx = 0;

    // 1. 축 기준선 (Border line)
    if (m_orientation == Qt::Horizontal) {
        const float y = static_cast<float>(pixelRect.bottom());
        vertices[idx++].set(static_cast<float>(pixelRect.left()), y);
        vertices[idx++].set(static_cast<float>(pixelRect.right()), y);
    } else {
        const float x = static_cast<float>(pixelRect.left());
        vertices[idx++].set(x, static_cast<float>(pixelRect.top()));
        vertices[idx++].set(x, static_cast<float>(pixelRect.bottom()));
    }

    // 2. 눈금선 및 그리드선 배치
    const float tickLength = 5.0f;

    for (const auto& tick : m_tickInfos) {
        if (m_orientation == Qt::Horizontal) {
            const QPointF p = transform.toPixel(QPointF(tick.position, 0.0));
            const float x = static_cast<float>(p.x());
            const float borderY = static_cast<float>(pixelRect.bottom());

            // 눈금선 (아래쪽 5px)
            vertices[idx++].set(x, borderY);
            vertices[idx++].set(x, borderY + tickLength);

            // 그리드선 (뷰포트 수직선)
            if (m_showGrid) {
                vertices[idx++].set(x, static_cast<float>(pixelRect.bottom()));
                vertices[idx++].set(x, static_cast<float>(pixelRect.top()));
            }
        } else {
            const QPointF p = transform.toPixel(QPointF(0.0, tick.position));
            const float y = static_cast<float>(p.y());
            const float borderX = static_cast<float>(pixelRect.left());

            // 눈금선 (왼쪽 5px)
            vertices[idx++].set(borderX, y);
            vertices[idx++].set(borderX - tickLength, y);

            // 그리드선 (뷰포트 수평선)
            if (m_showGrid) {
                vertices[idx++].set(static_cast<float>(pixelRect.left()), y);
                vertices[idx++].set(static_cast<float>(pixelRect.right()), y);
            }
        }
    }

    node->markDirty(QSGNode::DirtyGeometry);
    return node;
}
