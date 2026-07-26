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

//! @file WidgetChartView.cpp
//! @brief QWidget-based chart view stub implementation.

#include "WidgetChartView.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/qmath.h>
#include <QtGui/QPainter>

namespace qgraphplot
{

namespace
{
Q_LOGGING_CATEGORY(lcWidgetChartView, "qgraphplot.widgetchartview")
}  // namespace

WidgetChartView::WidgetChartView(QWidget* parent) : QWidget(parent) {}

WidgetChartView::~WidgetChartView()
{
    // Disconnect external-destroy tracking before ~QObject deletes children,
    // otherwise the destroyed handlers would access already-destroyed members.
    for (auto it = m_seriesDestroyedConnections.constBegin();
         it != m_seriesDestroyedConnections.constEnd();
         ++it) {
        disconnect(it.value());
    }
    m_seriesDestroyedConnections.clear();
}

void WidgetChartView::setTheme(QGraphPlotTheme* theme)
{
    if (m_theme == theme) {
        return;
    }
    if (m_theme) {
        disconnect(m_theme, nullptr, this, nullptr);
    }
    m_theme = theme;
    if (m_theme) {
        connect(m_theme,
                &QGraphPlotTheme::themeChanged,
                this,
                qOverload<>(&QWidget::update),
                Qt::UniqueConnection);
        connect(m_theme, &QObject::destroyed, this, [this]() {
            emit themeChanged();
            update();
        });
    }
    emit themeChanged();
    update();
}

void WidgetChartView::setXMin(double val)
{
    if (!qIsFinite(val) || val >= m_xMax) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setXMin: rejected non-finite or invalid xMin (must be < xMax):" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_xMin, val)) {
        m_xMin = val;
        emit xMinChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setXMax(double val)
{
    if (!qIsFinite(val) || val <= m_xMin) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setXMax: rejected non-finite or invalid xMax (must be > xMin):" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_xMax, val)) {
        m_xMax = val;
        emit xMaxChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setYMin(double val)
{
    if (!qIsFinite(val) || val >= m_yMax) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setYMin: rejected non-finite or invalid yMin (must be < yMax):" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_yMin, val)) {
        m_yMin = val;
        emit yMinChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setYMax(double val)
{
    if (!qIsFinite(val) || val <= m_yMin) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setYMax: rejected non-finite or invalid yMax (must be > yMin):" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_yMax, val)) {
        m_yMax = val;
        emit yMaxChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setMarginLeft(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setMarginLeft: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginLeft, val)) {
        m_marginLeft = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setMarginRight(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setMarginRight: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginRight, val)) {
        m_marginRight = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setMarginTop(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setMarginTop: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginTop, val)) {
        m_marginTop = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setMarginBottom(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setMarginBottom: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginBottom, val)) {
        m_marginBottom = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::addSeries(QAbstractSeries* aSeries)
{
    if (!aSeries || m_series.contains(aSeries)) {
        return;
    }
    m_series.append(aSeries);
    aSeries->setParent(this);
    m_seriesDestroyedConnections[aSeries] =
        connect(aSeries, &QObject::destroyed, this, [this, aSeries](QObject*) {
            m_series.removeAll(aSeries);
            m_seriesDestroyedConnections.remove(aSeries);
        });
    emit seriesAdded(aSeries);
    update();
}

void WidgetChartView::removeSeries(QAbstractSeries* aSeries)
{
    if (!m_series.contains(aSeries)) {
        return;
    }
    if (m_seriesDestroyedConnections.contains(aSeries)) {
        disconnect(m_seriesDestroyedConnections.take(aSeries));
    }
    m_series.removeOne(aSeries);
    if (aSeries) {
        aSeries->setParent(nullptr);
    }
    emit seriesRemoved(aSeries);
    update();
}

void WidgetChartView::clearSeries()
{
    while (!m_series.isEmpty()) {
        QAbstractSeries* aSeries = m_series.takeLast();
        if (m_seriesDestroyedConnections.contains(aSeries)) {
            disconnect(m_seriesDestroyedConnections.take(aSeries));
        }
        if (aSeries) {
            aSeries->setParent(nullptr);
        }
        emit seriesRemoved(aSeries);
    }
    update();
}

QCoordinateTransform WidgetChartView::coordinateTransform() const noexcept
{
    const QRectF dataBounds(m_xMin, m_yMin, m_xMax - m_xMin, m_yMax - m_yMin);
    const double pixelWidth =
        qMax(0.0, static_cast<double>(width()) - m_marginLeft - m_marginRight);
    const double pixelHeight =
        qMax(0.0, static_cast<double>(height()) - m_marginTop - m_marginBottom);
    const QRectF pixelRect(m_marginLeft, m_marginTop, pixelWidth, pixelHeight);
    return QCoordinateTransform(dataBounds, pixelRect);
}

QPointF WidgetChartView::mapToPixel(double x, double y) const noexcept
{
    return coordinateTransform().toPixel(QPointF(x, y));
}

void WidgetChartView::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    // Theme chrome only: canvas + plot area, matching what QmlChartView
    // paints for the same theme (AI.md §3.1). Grid, axes and series are
    // still unrendered here — the Widget rendering backend is a Phase 1
    // decision (QGraphPlot_MVP_Plan.md § 렌더링 백엔드 결정), so this
    // deliberately stays at the chrome level instead of pre-committing to
    // QPainter for the data path.
    if (!m_theme) {
        return;
    }

    QPainter painter(this);
    painter.fillRect(rect(), m_theme->backgroundColor());
    painter.fillRect(coordinateTransform().pixelRect(), m_theme->plotAreaColor());
}

void WidgetChartView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    emit transformChanged();
}

}  // namespace qgraphplot
