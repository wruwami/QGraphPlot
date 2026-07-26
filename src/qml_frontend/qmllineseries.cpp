#include "qmllineseries.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <QtQuick/QSGFlatColorMaterial>
#include <QtQuick/QSGGeometryNode>

#include "qmlchartview.h"
#include "series/QAbstractSeries.h"

namespace qgraphplot
{

namespace
{

//! Upper bound on the vertices a dashed line may expand to. A very short
//! dash pattern over a long polyline could otherwise allocate unboundedly;
//! beyond this the tail is dropped rather than stalling the render thread.
constexpr size_t kMaxDashVertices = 1u << 21;

//! Splits @p pixels into dash segments (2 vertices per segment).
//!
//! Pattern lengths follow QPen::setDashPattern(): they are multiples of the
//! stroke width, so the same pattern looks identical at any lineWidth — and
//! identical to what QPen will draw in the Widget frontend (AI.md §3.1).
std::vector<QSGGeometry::Point2D>
tessellateDashes(const std::vector<QPointF>& pixels, const QList<qreal>& pattern, double lineWidth)
{
    std::vector<QSGGeometry::Point2D> vertices;
    if (pixels.size() < 2 || pattern.isEmpty()) {
        return vertices;
    }

    qsizetype patternIndex = 0;
    bool penDown = true;  // Even pattern entries are dashes, odd ones are gaps.
    double remaining = pattern.at(0) * lineWidth;

    for (size_t i = 1; i < pixels.size(); ++i) {
        QPointF from = pixels[i - 1];
        const QPointF to = pixels[i];
        double segmentLength = std::hypot(to.x() - from.x(), to.y() - from.y());
        if (!std::isfinite(segmentLength) || segmentLength <= 0.0) {
            continue;
        }

        while (segmentLength > 0.0) {
            if (vertices.size() + 2 > kMaxDashVertices) {
                return vertices;
            }
            const double step = std::min(remaining, segmentLength);
            const double ratio = step / segmentLength;
            const QPointF next = from + (to - from) * ratio;

            if (penDown) {
                vertices.push_back({static_cast<float>(from.x()), static_cast<float>(from.y())});
                vertices.push_back({static_cast<float>(next.x()), static_cast<float>(next.y())});
            }

            segmentLength -= step;
            remaining -= step;
            from = next;

            if (remaining <= 0.0) {
                patternIndex = (patternIndex + 1) % pattern.size();
                penDown = (patternIndex % 2 == 0);
                remaining = pattern.at(patternIndex) * lineWidth;
            }
        }
    }
    return vertices;
}

}  // namespace

QmlLineSeries::QmlLineSeries(QQuickItem* parent) : QQuickItem(parent)
{
    // updatePaintNode가 호출되도록 설정
    setFlag(ItemHasContents, true);
}

QmlLineSeries::~QmlLineSeries()
{
    disconnectModelSignals();
    if (m_previousChartView) {
        disconnect(m_previousChartView, nullptr, this, nullptr);
    }
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

void QmlLineSeries::setLineWidth(double lineWidth)
{
    if (!std::isfinite(lineWidth) || lineWidth <= 0.0) {
        qWarning("QmlLineSeries::setLineWidth: lineWidth must be finite and > 0");
        return;
    }
    if (!qFuzzyCompare(m_lineWidth, lineWidth)) {
        m_lineWidth = lineWidth;
        emit lineWidthChanged();
        update();
    }
}

void QmlLineSeries::setDashPattern(const QList<qreal>& dashPattern)
{
    if (!qgraphplot::QAbstractSeries::isValidDashPattern(dashPattern)) {
        qWarning("QmlLineSeries::setDashPattern: pattern must have an even number of finite, "
                 "positive entries");
        return;
    }
    if (m_dashPattern != dashPattern) {
        m_dashPattern = dashPattern;
        emit dashPatternChanged();
        update();
    }
}

void QmlLineSeries::connectModelSignals()
{
    if (m_model) {
        connect(m_model,
                &qgraphplot::QAbstractSeriesModel::dataChanged,
                this,
                &QmlLineSeries::handleDataChanged,
                Qt::UniqueConnection);
        connect(m_model,
                &qgraphplot::QAbstractSeriesModel::pointsInserted,
                this,
                &QmlLineSeries::handleDataChanged,
                Qt::UniqueConnection);
        connect(m_model,
                &qgraphplot::QAbstractSeriesModel::pointsRemoved,
                this,
                &QmlLineSeries::handleDataChanged,
                Qt::UniqueConnection);
        connect(m_model,
                &qgraphplot::QAbstractSeriesModel::boundsChanged,
                this,
                &QmlLineSeries::handleDataChanged,
                Qt::UniqueConnection);
        connect(m_model,
                &QObject::destroyed,
                this,
                &QmlLineSeries::handleModelReset,
                Qt::UniqueConnection);
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
    if (m_previousChartView) {
        disconnect(m_previousChartView, &QmlChartView::transformChanged, this, &QQuickItem::update);
    }

    QmlChartView* chartView = qobject_cast<QmlChartView*>(parentItem());
    if (chartView) {
        connect(chartView,
                &QmlChartView::transformChanged,
                this,
                &QQuickItem::update,
                Qt::UniqueConnection);
        m_previousChartView = chartView;
    } else {
        m_previousChartView = nullptr;
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

    const qgraphplot::QCoordinateTransform transform = chartView->coordinateTransform();
    const qsizetype count = m_model->pointCount();
    if (count > INT_MAX) {
        delete oldNode;
        return nullptr;
    }

    // 3. points()는 링 버퍼가 wrap된 경우에도 항상 하나의 연속된 span을 반환하도록
    //    설계되어 있으므로(QRingBufferSeriesModel 참고), 매 포인트마다 가상함수인
    //    pointAt()을 호출하는 대신 span을 한 번만 가져와 순회한다(#36).
    const qgraphplot::PointSpan points = m_model->points(0, count - 1);

    // 4. dash 패턴이 있을 때만 픽셀 좌표를 모아 선분 단위로 분할한다.
    //    solid는 중간 버퍼 없이 정점 버퍼에 바로 쓴다 (60fps hot path).
    const bool dashed = !m_dashPattern.isEmpty();
    std::vector<QSGGeometry::Point2D> dashVertices;
    if (dashed) {
        std::vector<QPointF> pixels;
        pixels.reserve(static_cast<size_t>(count));
        for (qsizetype i = 0; i < count; ++i) {
            pixels.push_back(transform.toPixel(points[i]));
        }
        dashVertices = tessellateDashes(pixels, m_dashPattern, m_lineWidth);
        if (dashVertices.empty()) {
            delete oldNode;
            return nullptr;
        }
    }

    const size_t vertexCount = dashed ? dashVertices.size() : static_cast<size_t>(count);
    if (vertexCount > static_cast<size_t>(INT_MAX)) {
        delete oldNode;
        return nullptr;
    }
    const QSGGeometry::DrawingMode drawingMode =
        dashed ? QSGGeometry::DrawLines : QSGGeometry::DrawLineStrip;

    // 5. Scene Graph 노드 생성 및 재활용
    QSGGeometryNode* node = static_cast<QSGGeometryNode*>(oldNode);
    QSGGeometry* geometry = nullptr;

    if (!node) {
        node = new QSGGeometryNode();
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                                   static_cast<int>(vertexCount));
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
        // (오버플로 가드는 노드 생성/재사용 공통으로 위에서 처리한다)
        if (geometry->vertexCount() != static_cast<int>(vertexCount)) {
            geometry->allocate(static_cast<int>(vertexCount));
        }

        QSGFlatColorMaterial* material = static_cast<QSGFlatColorMaterial*>(node->material());
        if (material && material->color() != m_color) {
            material->setColor(m_color);
            node->markDirty(QSGNode::DirtyMaterial);
        }
    }

    geometry->setLineWidth(static_cast<float>(m_lineWidth));
    geometry->setDrawingMode(static_cast<unsigned int>(drawingMode));

    // 6. 버퍼 좌표 대입 및 스케일링
    QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();
    if (dashed) {
        std::copy(dashVertices.begin(), dashVertices.end(), vertices);
    } else {
        for (qsizetype i = 0; i < count; ++i) {
            const QPointF pixelPt = transform.toPixel(points[i]);
            vertices[i].set(static_cast<float>(pixelPt.x()), static_cast<float>(pixelPt.y()));
        }
    }

    node->markDirty(QSGNode::DirtyGeometry);
    return node;
}

}  // namespace qgraphplot
