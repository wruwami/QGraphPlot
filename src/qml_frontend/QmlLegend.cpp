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

#include "QmlLegend.h"

#include "QmlChartView.h"

namespace qgraphplot
{

// ── QmlLegendItem ────────────────────────────────────────────────────────────

QmlLegendItem::QmlLegendItem(QObject* parent) : QObject(parent) {}

void QmlLegendItem::setSeries(QAbstractSeries* series)
{
    if (m_series == series) {
        return;
    }

    disconnect(m_nameConn);
    disconnect(m_colorConn);
    disconnect(m_visibleConn);

    m_series = series;

    if (series) {
        m_name = series->name();
        m_color = series->color();
        m_visible = series->isVisible();

        m_nameConn = connect(series, &QAbstractSeries::nameChanged, this, [this](QString name) {
            m_name = std::move(name);
            emit nameChanged();
        });
        m_colorConn = connect(series, &QAbstractSeries::colorChanged, this, [this](QColor color) {
            m_color = color;
            emit colorChanged();
        });
        m_visibleConn =
            connect(series, &QAbstractSeries::visibleChanged, this, [this](bool visible) {
                m_visible = visible;
                emit visibleChanged();
            });
    } else {
        m_name = QString{};
        m_color = QColor(Qt::transparent);
        m_visible = true;
    }

    emit nameChanged();
    emit colorChanged();
    emit visibleChanged();
}

void QmlLegendItem::toggle()
{
    if (!m_series) {
        return;
    }
    m_series->setVisible(!m_series->isVisible());
}

// ── QmlLegend ────────────────────────────────────────────────────────────────

QmlLegend::QmlLegend(QQuickItem* parent) : QQuickItem(parent) {}

QmlLegend::~QmlLegend()
{
    for (auto& conn : m_chartConnections) {
        disconnect(conn);
    }
}

void QmlLegend::setChart(QmlChartView* chart)
{
    if (m_chart == chart) {
        return;
    }

    for (auto& conn : m_chartConnections) {
        disconnect(conn);
    }
    m_chartConnections.clear();

    m_chart = chart;
    emit chartChanged();

    rebuildItems();
    connectChartSignals();
}

void QmlLegend::setPosition(Qt::Alignment position)
{
    if (m_position == position) {
        return;
    }
    m_position = position;
    emit positionChanged();
}

void QmlLegend::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        connectChartSignals();
    }
}

void QmlLegend::connectChartSignals()
{
    for (auto& conn : m_chartConnections) {
        disconnect(conn);
    }
    m_chartConnections.clear();

    if (!m_chart) {
        return;
    }

    m_chartConnections.append(
        connect(m_chart, &QmlChartView::seriesAdded, this, [this](QAbstractSeries* series) {
            auto* item = new QmlLegendItem(this);
            item->setSeries(series);
            m_items.append(QVariant::fromValue(item));
            emit itemsChanged();
        }));

    m_chartConnections.append(
        connect(m_chart, &QmlChartView::seriesRemoved, this, [this](QAbstractSeries* series) {
            for (int i = 0; i < m_items.size(); ++i) {
                auto* item = m_items.at(i).value<QmlLegendItem*>();
                if (item && item->series() == series) {
                    m_items.removeAt(i);
                    delete item;
                    emit itemsChanged();
                    break;
                }
            }
        }));
}

void QmlLegend::rebuildItems()
{
    for (const auto& var : m_items) {
        delete var.value<QmlLegendItem*>();
    }
    m_items.clear();

    if (m_chart) {
        for (auto* series : m_chart->series()) {
            auto* item = new QmlLegendItem(this);
            item->setSeries(series);
            m_items.append(QVariant::fromValue(item));
        }
    }

    emit itemsChanged();
}

}  // namespace qgraphplot
