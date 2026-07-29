// QGraphPlot — High-performance Qt chart library
//
// Copyright 2026 QGraphPlot Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef QGRAPHPLOT_QMLLEGEND_H
#define QGRAPHPLOT_QMLLEGEND_H

#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QVariantList>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>

#include "series/QAbstractSeries.h"

namespace qgraphplot
{

class QmlChartView;

//! One entry in a chart legend, tracking a single series' name, colour, and
//! visibility.  Instances are created by QmlLegend and exposed to QML via the
//! `items` property; they should not be constructed directly from QML.
class QGRAPHPLOT_QML_EXPORT QmlLegendItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color NOTIFY colorChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(qgraphplot::QAbstractSeries* series READ series)

public:
    explicit QmlLegendItem(QObject* parent = nullptr);
    ~QmlLegendItem() override = default;

    QString name() const noexcept { return m_name; }
    QColor color() const noexcept { return m_color; }
    bool visible() const noexcept { return m_visible; }
    QAbstractSeries* series() const noexcept { return m_series; }

    //! Attach (or detach) this item from @p series.  Passing null resets all
    //! properties to their defaults and emits the corresponding changed signals.
    //! Passing the same series pointer again is a no-op.
    void setSeries(QAbstractSeries* series);

    //! Toggle the attached series' visibility.  Safe to call when no series is
    //! attached (returns without doing anything).
    Q_INVOKABLE void toggle();

signals:
    void nameChanged();
    void colorChanged();
    void visibleChanged();

private:
    QString m_name;
    QColor m_color;  // default-constructed QColor is invalid (no colour set)
    bool m_visible{true};
    QPointer<QAbstractSeries> m_series;

    QMetaObject::Connection m_nameConn;
    QMetaObject::Connection m_colorConn;
    QMetaObject::Connection m_visibleConn;
};

//! QML quick item that auto-populates a legend from a QmlChartView's series
//! collection.  Declare it as a child of (or alongside) a ChartView in QML
//! and set the `chart` property; the legend tracks series additions and
//! removals automatically.
//!
//! Registered as **QmlLegend** (not Legend). The public QML type is the
//! `Legend.qml` skin, which wraps this type and applies `position` layout
//! (issue #93) — same pattern as ChartView / LineSeries.
class QGRAPHPLOT_QML_EXPORT QmlLegend : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(QmlLegend)

    Q_PROPERTY(qgraphplot::QmlChartView* chart READ chart WRITE setChart NOTIFY chartChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(Qt::Alignment position READ position WRITE setPosition NOTIFY positionChanged)

public:
    explicit QmlLegend(QQuickItem* parent = nullptr);
    ~QmlLegend() override;

    QmlChartView* chart() const noexcept { return m_chart; }
    QVariantList items() const noexcept { return m_items; }
    Qt::Alignment position() const noexcept { return m_position; }

    void setChart(QmlChartView* chart);
    void setPosition(Qt::Alignment position);

signals:
    void chartChanged();
    void itemsChanged();
    void positionChanged();

protected:
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    //! (Re)subscribe to the chart's seriesAdded/seriesRemoved signals.
    //! Always clears previous connections first so calling this multiple
    //! times (e.g. on every ItemParentHasChanged) never accumulates
    //! duplicate connections.
    void connectChartSignals();
    void rebuildItems();
    //! Create a visual row QQuickItem (with marker and label children) for
    //! @p item, wiring up color/name/visibility signals.  The row is owned
    //! by this QmlLegend (QObject parent = this).
    QQuickItem* addVisualRow(QmlLegendItem* item);

    QPointer<QmlChartView> m_chart;
    QVariantList m_items;
    QList<QQuickItem*> m_rows;
    Qt::Alignment m_position{Qt::AlignTop};
    QList<QMetaObject::Connection> m_chartConnections;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_QMLLEGEND_H
