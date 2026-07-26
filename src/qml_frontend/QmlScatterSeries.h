#ifndef QGRAPHPLOT_QMLSCATTERSERIES_H
#define QGRAPHPLOT_QMLSCATTERSERIES_H

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>

#include "model/QAbstractSeriesModel.h"
#include "series/QScatterSeries.h"

namespace qgraphplot
{

class QmlChartView;

//! QML scatter-series renderer.
//!
//! This is the QML/Scene-Graph frontend for a discrete-marker series. It
//! mirrors QmlLineSeries's composition pattern (issue #57): it derives
//! from QQuickItem and composes a core QScatterSeries, delegating every
//! Q_PROPERTY READ/WRITE to it and forwarding NOTIFY signals. Shared
//! property validation lives in QAbstractSeries; scatter-specific
//! validation (markerSize > 0, markerShape) lives in QScatterSeries.
//!
//! Rendering emits markers as triangles (QSGGeometry::DrawTriangles) so
//! marker size and shape are portable across graphics APIs — unlike
//! DrawPoints, whose per-vertex size is platform-limited (issue #67).
//!
//! @sa QScatterSeries, QAbstractSeries, QmlLineSeries
class QmlScatterSeries : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    // Re-export MarkerShape so QML can write `markerShape: MarkerShape.Square`.
    Q_ENUM(qgraphplot::MarkerShape)

    Q_PROPERTY(
        qgraphplot::QAbstractSeriesModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    // NOTE: shadows QQuickItem::opacity — exposes the core series stroke
    // alpha (folded into marker color) so overlapping series compose
    // predictably (issue #71).
    Q_PROPERTY(double opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(double markerSize READ markerSize WRITE setMarkerSize NOTIFY markerSizeChanged)
    Q_PROPERTY(qgraphplot::MarkerShape markerShape READ markerShape WRITE setMarkerShape NOTIFY
                   markerShapeChanged)

public:
    explicit QmlScatterSeries(QQuickItem* parent = nullptr);
    ~QmlScatterSeries() override;

    // Getters — delegate to the core series so there is a single source of
    // truth for every property value.
    qgraphplot::QAbstractSeriesModel* model() const noexcept { return m_series->model(); }
    QColor color() const noexcept { return m_series->color(); }
    QString name() const noexcept { return m_series->name(); }
    double opacity() const noexcept { return m_series->opacity(); }

    //! Marker diameter in device-independent pixels. Validated by the core
    //! series (finite, > 0) so the QML and Widget frontends reject exactly
    //! the same inputs (AI.md §3.1).
    double markerSize() const noexcept { return m_series->markerSize(); }

    //! Marker geometry. @see qgraphplot::MarkerShape.
    qgraphplot::MarkerShape markerShape() const noexcept { return m_series->markerShape(); }

    // Setters — delegate to the core series, which performs validation and
    // emits the corresponding NOTIFY signal (forwarded below).
    void setModel(qgraphplot::QAbstractSeriesModel* model) { m_series->setModel(model); }
    void setColor(const QColor& color) { m_series->setColor(color); }
    void setName(const QString& name) { m_series->setName(name); }
    void setOpacity(double opacity) { m_series->setOpacity(opacity); }
    void setMarkerSize(double size) { m_series->setMarkerSize(size); }
    void setMarkerShape(qgraphplot::MarkerShape shape) { m_series->setMarkerShape(shape); }

signals:
    // Forwarded from the core series. Declared parameter-less for backward
    // compatibility with existing QML handlers; Qt allows connecting a
    // signal that carries arguments to one that takes fewer/none.
    void modelChanged();
    void colorChanged();
    void nameChanged();
    void opacityChanged();
    void markerSizeChanged();
    void markerShapeChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private slots:
    void handleDataChanged();

private:
    void connectForwardingSignals();
    void connectModelSignals();
    void disconnectModelSignals();
    void connectChartViewSignals();

    //! Core property backend — owned via Qt parent/child (this), so it is
    //! destroyed with the QmlScatterSeries. All property state lives here.
    QScatterSeries* const m_series;

    // Per-instance (NOT static — see #34): the ChartView this series is
    // currently connected to. QPointer so a disconnect() against an
    // already-destroyed chart (reparented away without going through
    // itemChange) is a safe no-op instead of dangling-pointer UB.
    QPointer<QmlChartView> m_previousChartView;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_QMLSCATTERSERIES_H
