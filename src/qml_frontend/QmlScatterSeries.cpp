#include "QmlScatterSeries.h"

#include <cmath>
#include <vector>

#include <QtQuick/QSGFlatColorMaterial>
#include <QtQuick/QSGGeometryNode>

#include "QmlChartView.h"

namespace qgraphplot
{

namespace
{

//! Standard mathematical constant pi (3.14159...). Replaces non-standard
//! pi macro to ensure portability across all platforms.
constexpr double pi = 3.14159265358979323846;

//! Triangle count per circle marker. A 12-gon (12 triangles in a fan,
//! unrolled into an indexed-less triangle list) is visually smooth enough
//! at typical marker sizes without bloating the vertex buffer.
constexpr int kCircleSegments = 12;

//! Triangles per marker by shape: circle = kCircleSegments, square = 2.
//! (3 vertices per triangle, DrawTriangles mode.)
inline int trianglesPerMarker(MarkerShape shape)
{
    return shape == MarkerShape::Circle ? kCircleSegments : 2;
}

//! Emits marker triangles centered at @p center with diameter @p size into
//! @p vertices. Circle = triangle fan unrolled (center + ring vertex i +
//! ring vertex i+1, per segment); square = two triangles covering an
//! axis-aligned box.
void appendMarker(std::vector<QSGGeometry::Point2D>& vertices,
                  QPointF center,
                  double size,
                  MarkerShape shape)
{
    const float cx = static_cast<float>(center.x());
    const float cy = static_cast<float>(center.y());
    const float r = static_cast<float>(size * 0.5);

    if (shape == MarkerShape::Circle) {
        for (int i = 0; i < kCircleSegments; ++i) {
            const double a0 = (2.0 * pi * i) / kCircleSegments;
            const double a1 = (2.0 * pi * (i + 1)) / kCircleSegments;
            vertices.push_back({cx, cy});
            vertices.push_back({cx + r * static_cast<float>(std::cos(a0)),
                                cy + r * static_cast<float>(std::sin(a0))});
            vertices.push_back({cx + r * static_cast<float>(std::cos(a1)),
                                cy + r * static_cast<float>(std::sin(a1))});
        }
    } else {
        // Square: two triangles (TL,BL,TR) and (BL,BR,TR).
        vertices.push_back({cx - r, cy - r});  // top-left
        vertices.push_back({cx - r, cy + r});  // bottom-left
        vertices.push_back({cx + r, cy - r});  // top-right
        vertices.push_back({cx - r, cy + r});  // bottom-left
        vertices.push_back({cx + r, cy + r});  // bottom-right
        vertices.push_back({cx + r, cy - r});  // top-right
    }
}

}  // namespace

QmlScatterSeries::QmlScatterSeries(QQuickItem* parent)
    : QQuickItem(parent), m_series(new QScatterSeries(this))
{
    setFlag(ItemHasContents, true);

    // Core 시리즈의 NOTIFY 시그널을 이 QML 항목으로 전달(forward)한다.
    // Q_PROPERTY NOTIFY는 아래 무인자 시그널을 가리키며, Qt는 인자가 있는
    // 시그널을 무인자 시그널에 연결하는 것을 허용하므로 기존 QML 핸들러와
    // 하위 호환된다. 검증·change-guard는 core 한 곳에서만 수행된다 (#57).
    connectForwardingSignals();

    // 프로퍼티 변경 시 씬그래프 노드를 다시 그린다. 컴포지션 구조에서는
    // setter가 core에 위임하므로, 렌더 갱신 트거도 core 시그널에서 발생한다.
    // Geometry-affecting changes set m_geometryDirty to avoid full vertex
    // regeneration on every frame (issue #1).
    connect(m_series, &QScatterSeries::colorChanged, this, [this]() {
        m_geometryDirty = true;
        update();
    });
    connect(m_series, &QScatterSeries::visibleChanged, this, [this]() { update(); });
    connect(m_series, &QScatterSeries::opacityChanged, this, [this]() {
        m_geometryDirty = true;
        update();
    });
    connect(m_series, &QScatterSeries::markerSizeChanged, this, [this]() {
        m_geometryDirty = true;
        update();
    });
    connect(m_series, &QScatterSeries::markerShapeChanged, this, [this]() {
        m_geometryDirty = true;
        update();
    });
    // modelChanged는 모델 시그널 재연결까지 담당하므로 별도 처리.
    connect(m_series, &QScatterSeries::modelChanged, this, [this]() {
        disconnectModelSignals();
        connectModelSignals();
        m_geometryDirty = true;
        update();
    });

    // 초기 모델(있다면)의 데이터 시그널을 연결한다.
    connectModelSignals();
}

QmlScatterSeries::~QmlScatterSeries()
{
    disconnectModelSignals();
    if (m_previousChartView) {
        disconnect(m_previousChartView, nullptr, this, nullptr);
    }
}

void QmlScatterSeries::connectForwardingSignals()
{
    connect(m_series, &QScatterSeries::modelChanged, this, &QmlScatterSeries::modelChanged);
    connect(m_series, &QScatterSeries::colorChanged, this, &QmlScatterSeries::colorChanged);
    connect(m_series, &QScatterSeries::nameChanged, this, &QmlScatterSeries::nameChanged);
    connect(m_series, &QScatterSeries::opacityChanged, this, &QmlScatterSeries::opacityChanged);
    connect(
        m_series, &QScatterSeries::markerSizeChanged, this, &QmlScatterSeries::markerSizeChanged);
    connect(
        m_series, &QScatterSeries::markerShapeChanged, this, &QmlScatterSeries::markerShapeChanged);
}

void QmlScatterSeries::connectModelSignals()
{
    auto* model = m_series->model();
    if (model) {
        connect(model,
                &qgraphplot::QAbstractSeriesModel::dataChanged,
                this,
                &QmlScatterSeries::handleDataChanged,
                Qt::UniqueConnection);
        connect(model,
                &qgraphplot::QAbstractSeriesModel::pointsInserted,
                this,
                &QmlScatterSeries::handleDataChanged,
                Qt::UniqueConnection);
        connect(model,
                &qgraphplot::QAbstractSeriesModel::pointsRemoved,
                this,
                &QmlScatterSeries::handleDataChanged,
                Qt::UniqueConnection);
        connect(model,
                &qgraphplot::QAbstractSeriesModel::boundsChanged,
                this,
                &QmlScatterSeries::handleDataChanged,
                Qt::UniqueConnection);
    }
}

void QmlScatterSeries::disconnectModelSignals()
{
    auto* model = m_series->model();
    if (model) {
        disconnect(model, nullptr, this, nullptr);
    }
}

void QmlScatterSeries::handleDataChanged()
{
    m_geometryDirty = true;
    update();
}

void QmlScatterSeries::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        connectChartViewSignals();
    }
}

void QmlScatterSeries::connectChartViewSignals()
{
    if (m_previousChartView) {
        disconnect(m_previousChartView, &QmlChartView::transformChanged, this, nullptr);
    }

    QmlChartView* chartView = qobject_cast<QmlChartView*>(parentItem());
    if (chartView) {
        connect(
            chartView,
            &QmlChartView::transformChanged,
            this,
            [this]() {
                m_geometryDirty = true;
                update();
            });
        m_previousChartView = chartView;
    } else {
        m_previousChartView = nullptr;
    }
}

QSGNode* QmlScatterSeries::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData)
{
    Q_UNUSED(updateData);

    // 0. core 시리즈가 보이지 않으면 렌더링하지 않는다 (#57 과 동일 시맨틱).
    if (!m_series->isVisible()) {
        delete oldNode;
        return nullptr;
    }

    // 핫패스 반복 호출을 피해 core 프로퍼티를 한 번 캐시한다.
    auto* model = m_series->model();
    const double markerSize = m_series->markerSize();
    const MarkerShape markerShape = m_series->markerShape();
    // opacity(0.0–1.0)를 색 알파에 곱한다 (#71).
    QColor color = m_series->color();
    color.setAlphaF(static_cast<float>(static_cast<double>(color.alphaF()) * m_series->opacity()));

    // 1. 그릴 데이터가 없는 경우
    if (!model || model->pointCount() == 0) {
        delete oldNode;
        return nullptr;
    }

    // 2. 부모 ChartView 검증
    QmlChartView* chartView = qobject_cast<QmlChartView*>(parentItem());
    if (!chartView) {
        delete oldNode;
        return nullptr;
    }

    // 3. Dirty-flag guard: skip full vertex regeneration when no data,
    //    property, or transform has changed (issue #1). Avoids tessellating
    //    60K points on every frame when only repainting.
    if (oldNode && !m_geometryDirty) {
        // Even without geometry changes, material (color/opacity) might need update.
        QSGGeometryNode* node = static_cast<QSGGeometryNode*>(oldNode);
        QSGFlatColorMaterial* material = static_cast<QSGFlatColorMaterial*>(node->material());
        if (material && material->color() != color) {
            material->setColor(color);
            node->markDirty(QSGNode::DirtyMaterial);
        }
        return oldNode;
    }

    const qgraphplot::QCoordinateTransform transform = chartView->coordinateTransform();
    const qsizetype count = model->pointCount();
    if (count > INT_MAX) {
        delete oldNode;
        return nullptr;
    }

    // 4. 점 데이터를 한 번에 가져온다 (#36 span-based traversal).
    const qgraphplot::PointSpan points = model->points(0, count - 1);

    // 5. 마커를 삼각형으로 테셀레이션한다. 마커당 정점 수 = 삼각형 수 * 3.
    //    DrawPoints 는 일부 RHI 백엔드에서 크기 제한이 있어 DrawTriangles
    //    방식을 사용한다 (#67).
    const int tris = trianglesPerMarker(markerShape);
    const qsizetype vertexCount64 = static_cast<qsizetype>(count) * tris * 3;
    if (vertexCount64 > static_cast<qsizetype>(INT_MAX)) {
        delete oldNode;
        return nullptr;
    }
    const int vertexCount = static_cast<int>(vertexCount64);

    std::vector<QSGGeometry::Point2D> markerVertices;
    markerVertices.reserve(static_cast<size_t>(vertexCount));
    for (qsizetype i = 0; i < count; ++i) {
        appendMarker(markerVertices, transform.toPixel(points[i]), markerSize, markerShape);
    }

    // 6. Scene Graph 노드 생성 및 재활용
    QSGGeometryNode* node = static_cast<QSGGeometryNode*>(oldNode);
    QSGGeometry* geometry = nullptr;

    if (!node) {
        node = new QSGGeometryNode();
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial* material = new QSGFlatColorMaterial();
        material->setColor(color);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    } else {
        geometry = node->geometry();
        if (geometry->vertexCount() != vertexCount) {
            geometry->allocate(vertexCount);
        }

        QSGFlatColorMaterial* material = static_cast<QSGFlatColorMaterial*>(node->material());
        if (material && material->color() != color) {
            material->setColor(color);
            node->markDirty(QSGNode::DirtyMaterial);
        }
    }

    geometry->setDrawingMode(QSGGeometry::DrawTriangles);

    // 7. 정점 좌표 대입
    QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();
    std::copy(markerVertices.begin(), markerVertices.end(), vertices);

    node->markDirty(QSGNode::DirtyGeometry);

    // 8. Clear dirty flag after successful regeneration (issue #1).
    m_geometryDirty = false;

    return node;
}

}  // namespace qgraphplot
