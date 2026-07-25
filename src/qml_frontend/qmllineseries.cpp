#include "qmllineseries.h"
#include "qmlchartview.h"
#include <QtQuick/QSGGeometryNode>
#include <QtQuick/QSGFlatColorMaterial>

QmlLineSeries::QmlLineSeries(QQuickItem* parent)
    : QQuickItem(parent)
{
    // updatePaintNode가 호출되도록 설정
    setFlag(ItemHasContents, true);
}

void QmlLineSeries::setModel(qgraphplot::QAbstractSeriesModel* model)
{
    if (m_model != model) {
        disconnectModelSignals();
        m_model = model;
        connectModelSignals();
        emit modelChanged();
        update();
    }
}

void QmlLineSeries::setColor(const QColor& color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged();
        update();
    }
}

void QmlLineSeries::setName(const QString& name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}

void QmlLineSeries::connectModelSignals()
{
    if (m_model) {
        connect(m_model, &qgraphplot::QAbstractSeriesModel::dataChanged,
                this, &QmlLineSeries::handleDataChanged, Qt::UniqueConnection);
        connect(m_model, &qgraphplot::QAbstractSeriesModel::pointsInserted,
                this, &QmlLineSeries::handleDataChanged, Qt::UniqueConnection);
        connect(m_model, &qgraphplot::QAbstractSeriesModel::pointsRemoved,
                this, &QmlLineSeries::handleDataChanged, Qt::UniqueConnection);
        connect(m_model, &qgraphplot::QAbstractSeriesModel::boundsChanged,
                this, &QmlLineSeries::handleDataChanged, Qt::UniqueConnection);
        connect(m_model, &QObject::destroyed,
                this, &QmlLineSeries::handleModelReset, Qt::UniqueConnection);
    }
}

void QmlLineSeries::disconnectModelSignals()
{
    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }
}

void QmlLineSeries::handleModelReset()
{
    m_model = nullptr;
    emit modelChanged();
    update();
}

void QmlLineSeries::handleDataChanged()
{
    update();
}

void QmlLineSeries::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        connectChartViewSignals();
    }
}

void QmlLineSeries::connectChartViewSignals()
{
    QmlChartView* chartView = qobject_cast<QmlChartView*>(parentItem());
    if (chartView) {
        connect(chartView, &QmlChartView::transformChanged, this, &QQuickItem::update, Qt::UniqueConnection);
    }
}

QSGNode* QmlLineSeries::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData)
{
    Q_UNUSED(updateData);

    // 1. 그릴 데이터가 없는 경우
    if (!m_model || m_model->pointCount() == 0) {
        delete oldNode;
        return nullptr;
    }

    // 2. 부모 ChartView 검증
    QmlChartView* chartView = qobject_cast<QmlChartView*>(parentItem());
    if (!chartView) {
        delete oldNode;
        return nullptr;
    }

    const QCoordinateTransform transform = chartView->coordinateTransform();
    const qsizetype count = m_model->pointCount();

    // 3. Scene Graph 노드 생성 및 재활용
    QSGGeometryNode* node = qobject_cast<QSGGeometryNode*>(oldNode);
    QSGGeometry* geometry = nullptr;

    if (!node) {
        node = new QSGGeometryNode();

        // 2D 포인트 포맷 사용, 라인 스트립으로 연속 선 그리기
        if (count > INT_MAX) {
            delete node;
            return nullptr;
        }
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), static_cast<int>(count));
        geometry->setLineWidth(2.0f);
        geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);

        // 단색 마테리얼 적용
        QSGFlatColorMaterial* material = new QSGFlatColorMaterial();
        material->setColor(m_color);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    } else {
        geometry = node->geometry();
        // 포인트 개수 변화 대응
        if (geometry->vertexCount() != static_cast<int>(count)) {
            geometry->allocate(static_cast<int>(count));
        }

        QSGFlatColorMaterial* material = qobject_cast<QSGFlatColorMaterial*>(node->material());
        if (material) {
            material->setColor(m_color);
            node->markDirty(QSGNode::DirtyMaterial);
        }
    }

    // 4. 버퍼 좌표 대입 및 스케일링
    QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();
    for (qsizetype i = 0; i < count; ++i) {
        const QPointF dataPt = m_model->pointAt(i);
        const QPointF pixelPt = transform.toPixel(dataPt);
        vertices[i].set(static_cast<float>(pixelPt.x()), static_cast<float>(pixelPt.y()));
    }

    node->markDirty(QSGNode::DirtyGeometry);
    return node;
}
