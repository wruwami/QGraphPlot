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

    Q_PROPERTY(qgraphplot::QGraphPlotTheme* theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QRectF plotArea READ plotArea NOTIFY transformChanged)

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

    void setTheme(qgraphplot::QGraphPlotTheme* theme);

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
    void seriesAdded(qgraphplot::QAbstractSeries* aSeries);
    void seriesRemoved(qgraphplot::QAbstractSeries* aSeries);

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    void applyThemeConnections();
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

    // Tracked core series. The QML wrappers (QmlLineSeries/QmlScatterSeries)
    // own these core objects via Qt parent/child, so the chart does NOT take
    // ownership — it only observes destruction to drop stale entries. This
    // matches WidgetChartView's tracking model (AI.md §3.1).
    QList<qgraphplot::QAbstractSeries*> m_series;
    QHash<qgraphplot::QAbstractSeries*, QMetaObject::Connection> m_seriesDestroyedConnections;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_QMLCHARTVIEW_H
