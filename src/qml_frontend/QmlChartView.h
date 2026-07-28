#ifndef QGRAPHPLOT_QMLCHARTVIEW_H
#define QGRAPHPLOT_QMLCHARTVIEW_H

#include <QtCore/QHash>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtQuick/QQuickItem>

#include "series/QAbstractSeries.h"
#include "theme/QGraphPlotTheme.h"
#include "transform/QCoordinateTransform.h"

namespace qgraphplot
{

class QGRAPHPLOT_QML_EXPORT QmlChartView : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double xMin READ xMin WRITE setXMin NOTIFY xMinChanged)
    Q_PROPERTY(double xMax READ xMax WRITE setXMax NOTIFY xMaxChanged)
    Q_PROPERTY(double yMin READ yMin WRITE setYMin NOTIFY yMinChanged)
    Q_PROPERTY(double yMax READ yMax WRITE setYMax NOTIFY yMaxChanged)

    Q_PROPERTY(double marginLeft READ marginLeft WRITE setMarginLeft NOTIFY marginsChanged)
    Q_PROPERTY(double marginRight READ marginRight WRITE setMarginRight NOTIFY marginsChanged)
    Q_PROPERTY(double marginTop READ marginTop WRITE setMarginTop NOTIFY marginsChanged)
    Q_PROPERTY(double marginBottom READ marginBottom WRITE setMarginBottom NOTIFY marginsChanged)

    // Axis auto-range (issue #63). When enabled, the view recomputes the
    // axis bounds from the union of all series' model bounds (+ padding)
    // on every boundsChanged. See § Priority rule in setAutoScaleX/Y docs.
    Q_PROPERTY(bool autoScaleX READ autoScaleX WRITE setAutoScaleX NOTIFY autoScaleXChanged)
    Q_PROPERTY(bool autoScaleY READ autoScaleY WRITE setAutoScaleY NOTIFY autoScaleYChanged)
    Q_PROPERTY(double autoScalePadding READ autoScalePadding WRITE setAutoScalePadding NOTIFY
                   autoScalePaddingChanged)

    Q_PROPERTY(qgraphplot::QGraphPlotTheme* theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QRectF plotArea READ plotArea NOTIFY transformChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

    // ── Zoom / pan interaction (issue #64) ────────────────────────
    // When false the corresponding axis is locked: wheel/drag/rubberband
    // gestures that would change that axis are silently ignored.
    Q_PROPERTY(bool zoomXEnabled READ zoomXEnabled WRITE setZoomXEnabled NOTIFY zoomXEnabledChanged)
    Q_PROPERTY(bool zoomYEnabled READ zoomYEnabled WRITE setZoomYEnabled NOTIFY zoomYEnabledChanged)

public:
    explicit QmlChartView(QQuickItem* parent = nullptr);
    ~QmlChartView() override;

    // Getters
    double xMin() const noexcept { return m_xMin; }
    double xMax() const noexcept { return m_xMax; }
    double yMin() const noexcept { return m_yMin; }
    double yMax() const noexcept { return m_yMax; }

    double marginLeft() const noexcept { return m_marginLeft; }
    double marginRight() const noexcept { return m_marginRight; }
    double marginTop() const noexcept { return m_marginTop; }
    double marginBottom() const noexcept { return m_marginBottom; }

    //! @name Axis auto-range (issue #63)
    //! When `autoScaleX`/`autoScaleY` is true, the corresponding axis
    //! min/max are recomputed from the union of all tracked series' model
    //! bounds plus `autoScalePadding` on every `boundsChanged`. The shared
    //! math lives in `qgraphplot::QAutoScaler` (AI.md §3.2).
    //!
    //! @par Priority rule
    //! Manual `setXMin/setXMax/setYMin/setYMax` calls still store the value
    //! (so disabling auto-scale reverts to it), but while auto-scale is on,
    //! the next `boundsChanged` from any tracked model overrides the axis
    //! range with the freshly computed one.
    //!@{
    [[nodiscard]] bool autoScaleX() const noexcept { return m_autoScaleX; }
    [[nodiscard]] bool autoScaleY() const noexcept { return m_autoScaleY; }
    [[nodiscard]] double autoScalePadding() const noexcept { return m_autoScalePadding; }
    //!@}

    //! Visual style shared with the axes / series and with the Widget
    //! frontend. Null (the default) means "paint no background", which keeps
    //! the chart transparent over whatever the QML host draws.
    qgraphplot::QGraphPlotTheme* theme() const noexcept { return m_theme; }

    //! The plot rectangle in item pixels (item rect minus the margins).
    //! Exposed so QML can position labels/legends against the same rect the
    //! scene graph nodes use.
    QRectF plotArea() const noexcept { return coordinateTransform().pixelRect(); }

    // Setters
    void setXMin(double val);
    void setXMax(double val);
    void setYMin(double val);
    void setYMax(double val);

    void setMarginLeft(double val);
    void setMarginRight(double val);
    void setMarginTop(double val);
    void setMarginBottom(double val);

    void setAutoScaleX(bool enabled);
    void setAutoScaleY(bool enabled);
    void setAutoScalePadding(double ratio);

    void setTheme(qgraphplot::QGraphPlotTheme* theme);

    // Title property accessors
    [[nodiscard]] QString title() const noexcept { return m_title; }
    void setTitle(const QString& title);

    // ── Zoom / pan (issue #64) ────────────────────────────────────
    [[nodiscard]] bool zoomXEnabled() const noexcept { return m_zoomXEnabled; }
    [[nodiscard]] bool zoomYEnabled() const noexcept { return m_zoomYEnabled; }
    void setZoomXEnabled(bool enabled);
    void setZoomYEnabled(bool enabled);

    //! Atomically update both X-axis bounds.  Disables autoScaleX.
    //! Silently ignored when min >= max or either value is non-finite.
    Q_INVOKABLE void setXRange(double min, double max);
    //! Atomically update both Y-axis bounds.  Disables autoScaleY.
    Q_INVOKABLE void setYRange(double min, double max);

    // Helpers
    qgraphplot::QCoordinateTransform coordinateTransform() const noexcept;
    Q_INVOKABLE QPointF mapToPixel(double x, double y) const noexcept;

    // ── Child series management ────────────────────────────────
    // Mirrors WidgetChartView's API surface (AI.md §3.1 parity) so the QML
    // and Widget frontends expose the same series-collection contract. The
    // chart tracks the *core* QAbstractSeries* objects composed by the QML
    // wrappers (QmlLineSeries/QmlScatterSeries), not the QQuickItem wrappers
    // themselves — the wrappers remain ordinary children of the chart item.
    void addSeries(qgraphplot::QAbstractSeries* aSeries);
    void removeSeries(qgraphplot::QAbstractSeries* aSeries);
    void clearSeries();
    [[nodiscard]] QList<qgraphplot::QAbstractSeries*> series() const noexcept { return m_series; }
    [[nodiscard]] qsizetype seriesCount() const noexcept { return m_series.size(); }
    [[nodiscard]] qgraphplot::QAbstractSeries* seriesAt(qsizetype index) const noexcept;

signals:
    void xMinChanged();
    void xMaxChanged();
    void yMinChanged();
    void yMaxChanged();
    void marginsChanged();
    void transformChanged();
    void themeChanged();
    void autoScaleXChanged();
    void autoScaleYChanged();
    void autoScalePaddingChanged();
    void seriesAdded(qgraphplot::QAbstractSeries* aSeries);
    void seriesRemoved(qgraphplot::QAbstractSeries* aSeries);
    void zoomXEnabledChanged();
    void zoomYEnabledChanged();
    void titleChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    void applyThemeConnections();
    //! Recomputes the axis bounds via QAutoScaler and writes them back to
    //! m_xMin/m_xMax/m_yMin/m_yMax for whichever axes have auto-scale on.
    //! Called on setAutoScale*(true), on addSeries, and on every tracked
    //! model's boundsChanged (signal-driven, not per-frame). Emits the
    //! corresponding *Changed + transformChanged signals only when a value
    //! actually moves (fuzzyValuesDiffer).
    void applyAutoScale();
    //! (Re)subscribes to the model of @p series (if any) so boundsChanged
    //! triggers applyAutoScale. Safe to call with a null model. Also wires
    //! modelChanged so a model assigned later re-subscribes.
    void connectAutoScaleModel(qgraphplot::QAbstractSeries* aSeries);
    //! Tears down the bounds/model subscriptions wired by
    //! connectAutoScaleModel for @p series.
    void disconnectAutoScaleModel(qgraphplot::QAbstractSeries* aSeries);
    //! Extracts the core QAbstractSeries* composed by a QML series wrapper
    //! (QmlLineSeries/QmlScatterSeries) if @p item is one, otherwise nullptr.
    //! Used by itemChange to keep the series collection in sync with child
    //! add/remove events (issue #58).
    qgraphplot::QAbstractSeries* coreSeriesFromItem(QQuickItem* item) const noexcept;

    // QPointer: the theme is typically owned by the QML host, not by the
    // chart, so it can outlive or predecease the chart in either order.
    QPointer<qgraphplot::QGraphPlotTheme> m_theme;

    double m_xMin{0.0};
    double m_xMax{10.0};
    double m_yMin{0.0};
    double m_yMax{10.0};

    double m_marginLeft{50.0};
    double m_marginRight{20.0};
    double m_marginTop{20.0};
    double m_marginBottom{40.0};

    bool m_autoScaleX{false};
    bool m_autoScaleY{false};
    double m_autoScalePadding{0.05};

    // Tracked core series. The QML wrappers (QmlLineSeries/QmlScatterSeries)
    // own these core objects via Qt parent/child, so the chart does NOT take
    // ownership — it only observes destruction to drop stale entries. This
    // matches WidgetChartView's tracking model (AI.md §3.1).
    QList<qgraphplot::QAbstractSeries*> m_series;
    QHash<qgraphplot::QAbstractSeries*, QMetaObject::Connection> m_seriesDestroyedConnections;
    // Auto-scale subscriptions keyed by series: [modelChanged conn,
    // current-model boundsChanged conn]. The boundsChanged entry is rebuilt
    // whenever the series swaps models (modelChanged fires).
    QHash<qgraphplot::QAbstractSeries*, QPair<QMetaObject::Connection, QMetaObject::Connection>>
        m_autoScaleConnections;

    bool m_zoomXEnabled{true};
    bool m_zoomYEnabled{true};
    QString m_title;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_QMLCHARTVIEW_H
