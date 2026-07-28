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

//! @file WidgetChartView.h
//! @brief QWidget-based chart view stub for the Widget frontend (Phase 0.8).

#ifndef WIDGETCHARTVIEW_H
#define WIDGETCHARTVIEW_H

#include <QtCore/QHash>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QPointF>
#include <QtWidgets/QWidget>

#include "series/QAbstractSeries.h"
#include "theme/QGraphPlotTheme.h"
#include "transform/QCoordinateTransform.h"

namespace qgraphplot
{

//! @brief QWidget 파생 차트 뷰 스텁.
//!
//! Phase 0.8에서는 렌더링 없이 API 표면과 빌드/링크 통합 지점만 검증.
//! QML 프론트엔드의 QmlChartView와 동일한 속성(xMin/xMax/yMin/yMax,
//! margin*) 및 시그널(transformChanged 등)을 노출한다 (AI.md §3.1 패리티).
//! 실제 그리기는 Phase 1에서 추가 예정.
class WidgetChartView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(double xMin READ xMin WRITE setXMin NOTIFY xMinChanged)
    Q_PROPERTY(double xMax READ xMax WRITE setXMax NOTIFY xMaxChanged)
    Q_PROPERTY(double yMin READ yMin WRITE setYMin NOTIFY yMinChanged)
    Q_PROPERTY(double yMax READ yMax WRITE setYMax NOTIFY yMaxChanged)

    Q_PROPERTY(double marginLeft READ marginLeft WRITE setMarginLeft NOTIFY marginsChanged)
    Q_PROPERTY(double marginRight READ marginRight WRITE setMarginRight NOTIFY marginsChanged)
    Q_PROPERTY(double marginTop READ marginTop WRITE setMarginTop NOTIFY marginsChanged)
    Q_PROPERTY(double marginBottom READ marginBottom WRITE setMarginBottom NOTIFY marginsChanged)

    // Axis auto-range (issue #63). Mirrors QmlChartView (AI.md §3.1 parity).
    Q_PROPERTY(bool autoScaleX READ autoScaleX WRITE setAutoScaleX NOTIFY autoScaleXChanged)
    Q_PROPERTY(bool autoScaleY READ autoScaleY WRITE setAutoScaleY NOTIFY autoScaleYChanged)
    Q_PROPERTY(double autoScalePadding READ autoScalePadding WRITE setAutoScalePadding NOTIFY
                   autoScalePaddingChanged)

    Q_PROPERTY(qgraphplot::QGraphPlotTheme* theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QRectF plotArea READ plotArea NOTIFY transformChanged)

    // ── Zoom / pan interaction (issue #64) ────────────────────────
    Q_PROPERTY(bool zoomXEnabled READ zoomXEnabled WRITE setZoomXEnabled NOTIFY zoomXEnabledChanged)
    Q_PROPERTY(bool zoomYEnabled READ zoomYEnabled WRITE setZoomYEnabled NOTIFY zoomYEnabledChanged)

public:
    explicit WidgetChartView(QWidget* parent = nullptr);
    ~WidgetChartView() override;

    WidgetChartView(const WidgetChartView&) = delete;
    WidgetChartView& operator=(const WidgetChartView&) = delete;
    WidgetChartView(WidgetChartView&&) = delete;
    WidgetChartView& operator=(WidgetChartView&&) = delete;

    // Getters
    [[nodiscard]] double xMin() const noexcept { return m_xMin; }
    [[nodiscard]] double xMax() const noexcept { return m_xMax; }
    [[nodiscard]] double yMin() const noexcept { return m_yMin; }
    [[nodiscard]] double yMax() const noexcept { return m_yMax; }

    [[nodiscard]] double marginLeft() const noexcept { return m_marginLeft; }
    [[nodiscard]] double marginRight() const noexcept { return m_marginRight; }
    [[nodiscard]] double marginTop() const noexcept { return m_marginTop; }
    [[nodiscard]] double marginBottom() const noexcept { return m_marginBottom; }

    //! @name Axis auto-range (issue #63)
    //! Same contract as QmlChartView (AI.md §3.1 parity): when on, the axis
    //! is recomputed from the union of all series' model bounds plus padding
    //! via the shared `qgraphplot::QAutoScaler`.
    //!
    //! @par Priority rule
    //! Manual `setXMin/setXMax/...` still store the value, but auto-scale
    //! overrides it on the next `boundsChanged`.
    //!@{
    [[nodiscard]] bool autoScaleX() const noexcept { return m_autoScaleX; }
    [[nodiscard]] bool autoScaleY() const noexcept { return m_autoScaleY; }
    [[nodiscard]] double autoScalePadding() const noexcept { return m_autoScalePadding; }
    //!@}

    //! Visual style, the very same type the QML frontend consumes (AI.md
    //! §3.1). Null (the default) keeps the widget's palette background.
    [[nodiscard]] QGraphPlotTheme* theme() const noexcept { return m_theme; }

    //! The plot rectangle in widget pixels (widget rect minus the margins).
    //! Matches QmlChartView's plotArea (AI.md §3.1).
    [[nodiscard]] QRectF plotArea() const noexcept { return coordinateTransform().pixelRect(); }

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

    void setTheme(QGraphPlotTheme* theme);

    // ── Zoom / pan (issue #64) ────────────────────────────────────
    [[nodiscard]] bool zoomXEnabled() const noexcept { return m_zoomXEnabled; }
    [[nodiscard]] bool zoomYEnabled() const noexcept { return m_zoomYEnabled; }
    void setZoomXEnabled(bool enabled);
    void setZoomYEnabled(bool enabled);

    //! Atomically update both X-axis bounds.  Disables autoScaleX.
    void setXRange(double min, double max);
    //! Atomically update both Y-axis bounds.  Disables autoScaleY.
    void setYRange(double min, double max);

    // Child series management (stub)
    void addSeries(QAbstractSeries* aSeries);
    void removeSeries(QAbstractSeries* aSeries);
    void clearSeries();
    [[nodiscard]] QList<QAbstractSeries*> series() const noexcept { return m_series; }

    // Coordinate helper
    [[nodiscard]] QCoordinateTransform coordinateTransform() const noexcept;
    [[nodiscard]] Q_INVOKABLE QPointF mapToPixel(double x, double y) const noexcept;

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

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    //! Recomputes axis bounds via QAutoScaler — same role as in QmlChartView.
    void applyAutoScale();
    //! (Re)subscribes to a series' model (and modelChanged) for auto-scale.
    void connectAutoScaleModel(QAbstractSeries* aSeries);
    //! Tears down the subscriptions wired by connectAutoScaleModel.
    void disconnectAutoScaleModel(QAbstractSeries* aSeries);

    // QPointer: the theme is typically owned by the application, not by the
    // view, so either object may be destroyed first.
    QPointer<QGraphPlotTheme> m_theme;

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

    QList<QAbstractSeries*> m_series;
    QHash<QAbstractSeries*, QMetaObject::Connection> m_seriesDestroyedConnections;
    // [modelChanged conn, current-model boundsChanged conn].
    QHash<QAbstractSeries*, QPair<QMetaObject::Connection, QMetaObject::Connection>>
        m_autoScaleConnections;

    bool m_zoomXEnabled{true};
    bool m_zoomYEnabled{true};

    // Pan state
    bool m_panning{false};
    QPointF m_panLastPixel;

    // Rubberband state (right-button drag → area zoom)
    bool m_rubberbanding{false};
    QPointF m_rubberbandOrigin;
    QPointF m_rubberbandCurrent;

    // Range saved before zoom starts (for double-click reset)
    double m_savedXMin{0.0};
    double m_savedXMax{10.0};
    double m_savedYMin{0.0};
    double m_savedYMax{10.0};
    bool m_rangeSaved{false};
};

}  // namespace qgraphplot

#endif  // WIDGETCHARTVIEW_H
