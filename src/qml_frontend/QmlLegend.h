#ifndef QGRAPHPLOT_QMLLEGEND_H
#define QGRAPHPLOT_QMLLEGEND_H

#include <QtCore/QPointer>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>

#include "series/QAbstractSeries.h"
#include "theme/QGraphPlotTheme.h"

namespace qgraphplot
{

class QmlChartView;

//! @brief Legend item for a single series.
//!
//! Exposed to QML as an attached type or model element. Each legend entry
//! shows the series name and color marker, and clicking toggles the series
//! visibility (issue #65).
class QGRAPHPLOT_QML_EXPORT QmlLegendItem : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color NOTIFY colorChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(qgraphplot::QAbstractSeries* series READ series CONSTANT)

public:
    explicit QmlLegendItem(QObject* parent = nullptr);
    ~QmlLegendItem() override = default;

    [[nodiscard]] QString name() const noexcept { return m_name; }
    [[nodiscard]] QColor color() const noexcept { return m_color; }
    [[nodiscard]] bool visible() const noexcept { return m_visible; }
    [[nodiscard]] qgraphplot::QAbstractSeries* series() const noexcept { return m_series; }

    //! Toggle the series visibility. Called from QML on marker click.
    Q_INVOKABLE void toggle();

    void setSeries(qgraphplot::QAbstractSeries* series);

signals:
    void nameChanged();
    void colorChanged();
    void visibleChanged();

private slots:
    void onSeriesNameChanged(const QString& name);
    void onSeriesColorChanged(const QColor& color);
    void onSeriesVisibleChanged(bool visible);

private:
    QString m_name;
    QColor m_color;
    bool m_visible{true};
    QPointer<qgraphplot::QAbstractSeries> m_series;
};

//! @brief Legend widget displaying all series in a chart.
//!
//! The legend auto-populates from the attached QmlChartView's series list.
//! Position can be set to Top/Bottom/Left/Right (default: Top). Styling
//! (font/color) comes from the shared QGraphPlotTheme (AI.md §3.2).
class QGRAPHPLOT_QML_EXPORT QmlLegend : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qgraphplot::QmlChartView* chart READ chart WRITE setChart NOTIFY chartChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(Qt::Alignment position READ position WRITE setPosition NOTIFY positionChanged)

public:
    explicit QmlLegend(QQuickItem* parent = nullptr);
    ~QmlLegend() override = default;

    [[nodiscard]] qgraphplot::QmlChartView* chart() const noexcept { return m_chart; }
    void setChart(qgraphplot::QmlChartView* chart);

    [[nodiscard]] QVariantList items() const noexcept;

    [[nodiscard]] Qt::Alignment position() const noexcept { return m_position; }
    void setPosition(Qt::Alignment position);

signals:
    void chartChanged();
    void itemsChanged();
    void positionChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private slots:
    void onChartSeriesAdded(qgraphplot::QAbstractSeries* series);
    void onChartSeriesRemoved(qgraphplot::QAbstractSeries* series);
    void onThemeChanged();

private:
    void rebuildItems();
    void connectChartSignals();

    QPointer<qgraphplot::QmlChartView> m_chart;
    QList<QmlLegendItem*> m_items;
    Qt::Alignment m_position{Qt::AlignTop};
    QPointer<qgraphplot::QGraphPlotTheme> m_cachedTheme;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_QMLLEGEND_H
