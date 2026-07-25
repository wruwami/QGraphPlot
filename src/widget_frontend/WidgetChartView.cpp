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

#include <QtCore/qmath.h>

namespace qgraphplot
{

namespace
{
constexpr double kFuzzyEpsilon = 1e-10;

bool fuzzyDifferent(double oldValue, double newValue)
{
    if (qAbs(oldValue - newValue) <= kFuzzyEpsilon) {
        return false;
    }
    if (qAbs(oldValue) < kFuzzyEpsilon || qAbs(newValue) < kFuzzyEpsilon) {
        return true;
    }
    return !qFuzzyCompare(oldValue, newValue);
}
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

void WidgetChartView::setXMin(double val)
{
    if (fuzzyDifferent(m_xMin, val)) {
        m_xMin = val;
        emit xMinChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setXMax(double val)
{
    if (fuzzyDifferent(m_xMax, val)) {
        m_xMax = val;
        emit xMaxChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setYMin(double val)
{
    if (fuzzyDifferent(m_yMin, val)) {
        m_yMin = val;
        emit yMinChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setYMax(double val)
{
    if (fuzzyDifferent(m_yMax, val)) {
        m_yMax = val;
        emit yMaxChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setMarginLeft(double val)
{
    if (!qFuzzyCompare(m_marginLeft, val)) {
        m_marginLeft = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setMarginRight(double val)
{
    if (!qFuzzyCompare(m_marginRight, val)) {
        m_marginRight = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setMarginTop(double val)
{
    if (!qFuzzyCompare(m_marginTop, val)) {
        m_marginTop = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setMarginBottom(double val)
{
    if (!qFuzzyCompare(m_marginBottom, val)) {
        m_marginBottom = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::addSeries(QAbstractSeries* series)
{
    if (!series || m_series.contains(series)) {
        return;
    }
    m_series.append(series);
    series->setParent(this);
    m_seriesDestroyedConnections[series] =
        connect(series, &QObject::destroyed, this, [this, series](QObject*) {
            m_series.removeAll(series);
            m_seriesDestroyedConnections.remove(series);
        });
    emit seriesAdded(series);
    update();
}

void WidgetChartView::removeSeries(QAbstractSeries* series)
{
    if (!m_series.contains(series)) {
        return;
    }
    if (m_seriesDestroyedConnections.contains(series)) {
        disconnect(m_seriesDestroyedConnections.take(series));
    }
    m_series.removeOne(series);
    if (series) {
        series->setParent(nullptr);
    }
    emit seriesRemoved(series);
    update();
}

void WidgetChartView::clearSeries()
{
    while (!m_series.isEmpty()) {
        QAbstractSeries* series = m_series.takeLast();
        if (m_seriesDestroyedConnections.contains(series)) {
            disconnect(m_seriesDestroyedConnections.take(series));
        }
        if (series) {
            series->setParent(nullptr);
        }
        emit seriesRemoved(series);
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
    // Phase 0.8 stub: rendering intentionally left empty.
    // Phase 1 will add QPainter/QOpenGLWidget/QRhi backend selection.
    QWidget::paintEvent(event);
}

void WidgetChartView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    emit transformChanged();
}

}  // namespace qgraphplot
