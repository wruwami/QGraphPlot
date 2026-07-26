#ifndef QMLCHARTVIEW_H
#define QMLCHARTVIEW_H

#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtQuick/QQuickItem>

#include "theme/QGraphPlotTheme.h"
#include "transform/QCoordinateTransform.h"

class QmlChartView : public QQuickItem
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
    ~QmlChartView() override = default;

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

signals:
    void xMinChanged();
    void xMaxChanged();
    void yMinChanged();
    void yMaxChanged();
    void marginsChanged();
    void transformChanged();
    void themeChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    void applyThemeConnections();

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
};

#endif  // QMLCHARTVIEW_H
