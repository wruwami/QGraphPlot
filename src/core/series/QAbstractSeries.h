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

//! @file QAbstractSeries.h
//! @brief Abstract base class for all chart series (line, scatter, bar, ...).
//!
//! QAbstractSeries is the property contract for a series — it references the
//! data model (which it does not own), and owns the color, name, and visibility,
//! but it deliberately does NOT know how to render itself. Concrete renderers live in the
//! frontends:
//!
//!   - QML frontend:    QmlLineSeries, QmlScatterSeries, ...
//!   - Widget frontend: WidgetLineSeries, WidgetScatterSeries, ...
//!
//! This is the Interface-First strategy (QGraphPlot_MVP_Plan.md): the core
//! defines the contract, each frontend plugs in its own rendering on top.
//! As a result the same QAbstractSeries API surface is visible from both
//! QML (via Q_PROPERTY) and C++ (via getters/setters), giving the two
//! frontends identical observable behavior (AI.md §3.1 parity).

#ifndef QGRAPHPLOT_ABSTRACTSERIES_H
#define QGRAPHPLOT_ABSTRACTSERIES_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QColor>

#include "../model/QAbstractSeriesModel.h"
#include "../qgraphplot_global.h"

namespace qgraphplot
{

//! Identifies what kind of series a subclass renders.
//!
//! Defined once here (AI.md §3.2 — no magic numbers / strings duplicated
//! elsewhere). Frontends switch on this to dispatch to their per-type
//! renderer. New series types add a value here and consume it via the
//! enum; numeric values are NOT to be hardcoded.
enum class SeriesType : unsigned char
{
    Line,     //!< Connected polyline (QmlLineSeries / WidgetLineSeries)
    Scatter,  //!< Discrete markers per point
    Area,     //!< Filled region between a line and a baseline
    Bar,      //!< Rectangular bars
    Pie,      //!< Pie / donut slices (future)
};

//! @brief Property-only base class for all chart series.
//!
//! Subclassing is the extension point for new chart types. Subclasses
//! MUST override type() to return their SeriesType. Rendering methods
//! are intentionally absent — they belong to the frontend layer.
//!
//! Threading: like all QObject subclasses in this library, instances
//! are affined to the thread that creates them. GUI-thread access is
//! the norm; the render thread reads only via the model's signals
//! (QGraphPlot_MVP_Plan.md § 60fps 설계).
class QGRAPHPLOT_EXPORT QAbstractSeries : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
        qgraphplot::QAbstractSeriesModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)

public:
    //! Constructs an empty series. @p parent follows Qt ownership (RAII,
    //! AI.md §1.4) — passing a QChartView / QmlChartView as parent is the
    //! usual choice so the series is destroyed with its view.
    explicit QAbstractSeries(QObject* parent = nullptr);

    //! Polymorphic base class → virtual destructor (AI.md §1.4).
    ~QAbstractSeries() override;

    QAbstractSeries(const QAbstractSeries&) = delete;
    QAbstractSeries& operator=(const QAbstractSeries&) = delete;
    QAbstractSeries(QAbstractSeries&&) = delete;
    QAbstractSeries& operator=(QAbstractSeries&&) = delete;

    // ── Identity ─────────────────────────────────────────────

    //! What kind of series this is. Subclasses return their own SeriesType.
    [[nodiscard]] virtual SeriesType type() const = 0;

    // ── Properties ───────────────────────────────────────────

    //! The data source backing this series. May be null (empty series).
    [[nodiscard]] QAbstractSeriesModel* model() const { return m_model; }
    void setModel(QAbstractSeriesModel* model);

    //! Stroke / fill color used by the renderer.
    [[nodiscard]] QColor color() const { return m_color; }
    void setColor(QColor color);

    //! Display name (used by legends).
    [[nodiscard]] QString name() const { return m_name; }
    void setName(QString name);

    //! Whether the series should be rendered. Invisible series still
    //! participate in axis autoscaling (their bounds count) — renderers
    //! simply skip drawing them.
    [[nodiscard]] bool isVisible() const { return m_visible; }
    void setVisible(bool visible);

Q_SIGNALS:
    void modelChanged(QAbstractSeriesModel* model);
    void colorChanged(QColor color);
    void nameChanged(QString name);
    void visibleChanged(bool visible);

private:
    QAbstractSeriesModel* m_model = nullptr;
    QMetaObject::Connection m_modelDestroyedConnection;
    QColor m_color = Qt::blue;  //!< Member-initialized (AI.md §1.4)
    QString m_name;
    bool m_visible = true;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_ABSTRACTSERIES_H
