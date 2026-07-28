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

#include "transform/QAutoScaler.h"

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
    for (auto it = m_autoScaleConnections.constBegin(); it != m_autoScaleConnections.constEnd();
         ++it) {
        disconnect(it.value().first);
        disconnect(it.value().second);
    }
    m_autoScaleConnections.clear();
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
        qCWarning(lcWidgetChartView)
            << "WidgetChartView::setXMin: rejected non-finite or invalid xMin (must be < xMax):"
            << val;
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
        qCWarning(lcWidgetChartView)
            << "WidgetChartView::setXMax: rejected non-finite or invalid xMax (must be > xMin):"
            << val;
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
        qCWarning(lcWidgetChartView)
            << "WidgetChartView::setYMin: rejected non-finite or invalid yMin (must be < yMax):"
            << val;
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
        qCWarning(lcWidgetChartView)
            << "WidgetChartView::setYMax: rejected non-finite or invalid yMax (must be > yMin):"
            << val;
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
        qCWarning(lcWidgetChartView)
            << "WidgetChartView::setMarginLeft: rejected negative or non-finite margin:" << val;
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
        qCWarning(lcWidgetChartView)
            << "WidgetChartView::setMarginRight: rejected negative or non-finite margin:" << val;
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
        qCWarning(lcWidgetChartView)
            << "WidgetChartView::setMarginTop: rejected negative or non-finite margin:" << val;
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
        qCWarning(lcWidgetChartView)
            << "WidgetChartView::setMarginBottom: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginBottom, val)) {
        m_marginBottom = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setAutoScaleX(bool enabled)
{
    if (m_autoScaleX == enabled) {
        return;
    }
    m_autoScaleX = enabled;
    emit autoScaleXChanged();
    if (enabled) {
        applyAutoScale();
    }
}

void WidgetChartView::setAutoScaleY(bool enabled)
{
    if (m_autoScaleY == enabled) {
        return;
    }
    m_autoScaleY = enabled;
    emit autoScaleYChanged();
    if (enabled) {
        applyAutoScale();
    }
}

void WidgetChartView::setAutoScalePadding(double ratio)
{
    if (!qIsFinite(ratio) || ratio < 0.0) {
        qCWarning(lcWidgetChartView) << "WidgetChartView::setAutoScalePadding: rejected negative "
                                        "or non-finite ratio:"
                                     << ratio;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_autoScalePadding, ratio)) {
        m_autoScalePadding = ratio;
        emit autoScalePaddingChanged();
        if (m_autoScaleX || m_autoScaleY) {
            applyAutoScale();
        }
    }
}

void WidgetChartView::setXRange(double min, double max)
{
    if (!qIsFinite(min) || !qIsFinite(max) || min >= max) {
        return;
    }
    if (m_autoScaleX) {
        m_autoScaleX = false;
        emit autoScaleXChanged();
    }
    bool changed = false;
    if (qgraphplot::fuzzyValuesDiffer(m_xMin, min)) {
        m_xMin = min;
        emit xMinChanged();
        changed = true;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_xMax, max)) {
        m_xMax = max;
        emit xMaxChanged();
        changed = true;
    }
    if (changed) {
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setYRange(double min, double max)
{
    if (!qIsFinite(min) || !qIsFinite(max) || min >= max) {
        return;
    }
    if (m_autoScaleY) {
        m_autoScaleY = false;
        emit autoScaleYChanged();
    }
    bool changed = false;
    if (qgraphplot::fuzzyValuesDiffer(m_yMin, min)) {
        m_yMin = min;
        emit yMinChanged();
        changed = true;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_yMax, max)) {
        m_yMax = max;
        emit yMaxChanged();
        changed = true;
    }
    if (changed) {
        emit transformChanged();
        update();
    }
}

void WidgetChartView::setZoomXEnabled(bool enabled)
{
    if (m_zoomXEnabled == enabled) {
        return;
    }
    m_zoomXEnabled = enabled;
    emit zoomXEnabledChanged();
}

void WidgetChartView::setZoomYEnabled(bool enabled)
{
    if (m_zoomYEnabled == enabled) {
        return;
    }
    m_zoomYEnabled = enabled;
    emit zoomYEnabledChanged();
}

void WidgetChartView::wheelEvent(QWheelEvent* event)
{
    const QRectF plot = plotArea();
    if (!plot.contains(event->position())) {
        QWidget::wheelEvent(event);
        return;
    }
    const double degrees = event->angleDelta().y() / 8.0;
    const double factor = std::pow(0.9, degrees / 15.0);  // zoom 10% per notch

    const QCoordinateTransform xform = coordinateTransform();
    const QPointF dataPos = xform.toData(event->position());

    if (m_zoomXEnabled) {
        const double span = m_xMax - m_xMin;
        const double newSpan = span * factor;
        const double t = (dataPos.x() - m_xMin) / span;
        setXRange(dataPos.x() - t * newSpan, dataPos.x() + (1.0 - t) * newSpan);
    }
    if (m_zoomYEnabled) {
        const double span = m_yMax - m_yMin;
        const double newSpan = span * factor;
        const double t = (dataPos.y() - m_yMin) / span;
        setYRange(dataPos.y() - t * newSpan, dataPos.y() + (1.0 - t) * newSpan);
    }
    event->accept();
}

void WidgetChartView::mousePressEvent(QMouseEvent* event)
{
    const QRectF plot = plotArea();
    if (!plot.contains(event->position())) {
        QWidget::mousePressEvent(event);
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_panning = true;
        m_panLastPixel = event->position();
        if (!m_rangeSaved) {
            m_savedXMin = m_xMin;
            m_savedXMax = m_xMax;
            m_savedYMin = m_yMin;
            m_savedYMax = m_yMax;
            m_rangeSaved = true;
        }
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else if (event->button() == Qt::RightButton) {
        m_rubberbanding = true;
        m_rubberbandOrigin = event->position();
        m_rubberbandCurrent = event->position();
        if (!m_rangeSaved) {
            m_savedXMin = m_xMin;
            m_savedXMax = m_xMax;
            m_savedYMin = m_yMin;
            m_savedYMax = m_yMax;
            m_rangeSaved = true;
        }
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void WidgetChartView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        const QCoordinateTransform xform = coordinateTransform();
        const QPointF prev = xform.toData(m_panLastPixel);
        const QPointF cur = xform.toData(event->position());
        const double dx = prev.x() - cur.x();
        const double dy = prev.y() - cur.y();
        if (m_zoomXEnabled) {
            setXRange(m_xMin + dx, m_xMax + dx);
        }
        if (m_zoomYEnabled) {
            setYRange(m_yMin + dy, m_yMax + dy);
        }
        m_panLastPixel = event->position();
        event->accept();
    } else if (m_rubberbanding) {
        m_rubberbandCurrent = event->position();
        update();
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void WidgetChartView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_panning && event->button() == Qt::LeftButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else if (m_rubberbanding && event->button() == Qt::RightButton) {
        m_rubberbanding = false;
        const QRectF plot = plotArea();
        const QRectF band =
            QRectF(m_rubberbandOrigin, m_rubberbandCurrent).normalized().intersected(plot);
        if (band.width() > 4 && band.height() > 4) {
            const QCoordinateTransform xform = coordinateTransform();
            const QPointF dataMin = xform.toData(band.bottomLeft());
            const QPointF dataMax = xform.toData(band.topRight());
            if (m_zoomXEnabled) {
                setXRange(dataMin.x(), dataMax.x());
            }
            if (m_zoomYEnabled) {
                setYRange(dataMin.y(), dataMax.y());
            }
        }
        update();
        event->accept();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
}

void WidgetChartView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_rangeSaved) {
        setXRange(m_savedXMin, m_savedXMax);
        setYRange(m_savedYMin, m_savedYMax);
        m_rangeSaved = false;
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
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
            disconnectAutoScaleModel(aSeries);
            applyAutoScale();
        });
    connectAutoScaleModel(aSeries);
    applyAutoScale();
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
    disconnectAutoScaleModel(aSeries);
    m_series.removeOne(aSeries);
    if (aSeries) {
        aSeries->setParent(nullptr);
    }
    applyAutoScale();
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
        disconnectAutoScaleModel(aSeries);
        if (aSeries) {
            aSeries->setParent(nullptr);
        }
        emit seriesRemoved(aSeries);
    }
    applyAutoScale();
    update();
}

void WidgetChartView::connectAutoScaleModel(QAbstractSeries* aSeries)
{
    if (!aSeries || m_autoScaleConnections.contains(aSeries)) {
        return;
    }
    QMetaObject::Connection modelConn =
        connect(aSeries, &QAbstractSeries::modelChanged, this, [this, aSeries]() {
            auto it = m_autoScaleConnections.find(aSeries);
            if (it == m_autoScaleConnections.end()) {
                return;
            }
            disconnect(it.value().second);
            it.value().second = QMetaObject::Connection();
            if (auto* model = aSeries->model()) {
                it.value().second = connect(model,
                                            &QAbstractSeriesModel::boundsChanged,
                                            this,
                                            &WidgetChartView::applyAutoScale,
                                            Qt::UniqueConnection);
            }
            applyAutoScale();
        });
    QMetaObject::Connection boundsConn;
    if (auto* model = aSeries->model()) {
        boundsConn = connect(model,
                             &QAbstractSeriesModel::boundsChanged,
                             this,
                             &WidgetChartView::applyAutoScale,
                             Qt::UniqueConnection);
    }
    m_autoScaleConnections.insert(aSeries, {modelConn, boundsConn});
}

void WidgetChartView::disconnectAutoScaleModel(QAbstractSeries* aSeries)
{
    auto it = m_autoScaleConnections.find(aSeries);
    if (it == m_autoScaleConnections.end()) {
        return;
    }
    disconnect(it.value().first);
    disconnect(it.value().second);
    m_autoScaleConnections.erase(it);
}

void WidgetChartView::applyAutoScale()
{
    if (!m_autoScaleX && !m_autoScaleY) {
        return;
    }
    const QRectF padded = QAutoScaler::computePaddedBounds(m_series, m_autoScalePadding);
    // computePaddedBounds guarantees min < max on both axes; write members
    // directly, bypassing manual-setter validation (see priority rule in
    // WidgetChartView.h).
    bool changed = false;
    if (m_autoScaleX) {
        if (qgraphplot::fuzzyValuesDiffer(m_xMin, padded.left())) {
            m_xMin = padded.left();
            emit xMinChanged();
            changed = true;
        }
        if (qgraphplot::fuzzyValuesDiffer(m_xMax, padded.right())) {
            m_xMax = padded.right();
            emit xMaxChanged();
            changed = true;
        }
    }
    if (m_autoScaleY) {
        if (qgraphplot::fuzzyValuesDiffer(m_yMin, padded.top())) {
            m_yMin = padded.top();
            emit yMinChanged();
            changed = true;
        }
        if (qgraphplot::fuzzyValuesDiffer(m_yMax, padded.bottom())) {
            m_yMax = padded.bottom();
            emit yMaxChanged();
            changed = true;
        }
    }
    if (changed) {
        emit transformChanged();
        update();
    }
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
    if (!m_theme && !m_rubberbanding) {
        return;
    }

    QPainter painter(this);
    if (m_theme) {
        painter.fillRect(rect(), m_theme->backgroundColor());
        painter.fillRect(coordinateTransform().pixelRect(), m_theme->plotAreaColor());
    }

    if (m_rubberbanding) {
        const QRectF band = QRectF(m_rubberbandOrigin, m_rubberbandCurrent).normalized();
        painter.save();
        painter.setPen(QPen(QColor(0, 120, 215, 200), 1, Qt::DashLine));
        painter.fillRect(band, QColor(0, 120, 215, 40));
        painter.drawRect(band);
        painter.restore();
    }
}

void WidgetChartView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    emit transformChanged();
}

}  // namespace qgraphplot
