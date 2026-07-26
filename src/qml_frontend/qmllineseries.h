#ifndef QMLLINESERIES_H
#define QMLLINESERIES_H

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>

#include "model/QAbstractSeriesModel.h"
#include "series/QLineSeries.h"

namespace qgraphplot
{

class QmlChartView;

//! QML line-series renderer.
//!
//! This is the QML/Scene-Graph frontend for a connected-polyline series.
//! It must derive from QQuickItem to participate in the scene graph, so it
//! cannot also subclass the core QLineSeries (QObject diamond is
//! forbidden). Instead it composes one: every Q_PROPERTY below delegates
//! its READ/WRITE to the owned @ref m_series, and every NOTIFY signal is
//! forwarded from it. As a result validation, change-guarding, defaults
//! and signals live in exactly one place — QAbstractSeries — satisfying
//! AI.md §3.1 (parity) and §3.2 (no duplicated logic). See issue #57.
//!
//! @sa QLineSeries, QAbstractSeries
class QmlLineSeries : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
        qgraphplot::QAbstractSeriesModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(double lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(
        QList<qreal> dashPattern READ dashPattern WRITE setDashPattern NOTIFY dashPatternChanged)

public:
    explicit QmlLineSeries(QQuickItem* parent = nullptr);
    ~QmlLineSeries() override;

    // Getters — delegate to the core series so there is a single source of
    // truth for every property value.
    qgraphplot::QAbstractSeriesModel* model() const noexcept { return m_series->model(); }
    QColor color() const noexcept { return m_series->color(); }
    QString name() const noexcept { return m_series->name(); }

    //! Stroke width in device-independent pixels. Validated by the core
    //! series (finite, > 0) so the QML and Widget frontends reject exactly
    //! the same inputs (AI.md §3.1).
    double lineWidth() const noexcept { return m_series->lineWidth(); }

    //! Alternating dash/gap lengths in units of lineWidth (QPen semantics).
    //! Empty means a solid line. Validated by the core series so the QML
    //! and Widget series accept exactly the same patterns (AI.md §3.1).
    QList<qreal> dashPattern() const noexcept { return m_series->dashPattern(); }

    // Setters — delegate to the core series, which performs validation and
    // emits the corresponding NOTIFY signal (forwarded below).
    void setModel(qgraphplot::QAbstractSeriesModel* model) { m_series->setModel(model); }
    void setColor(const QColor& color) { m_series->setColor(color); }
    void setName(const QString& name) { m_series->setName(name); }
    void setLineWidth(double lineWidth) { m_series->setLineWidth(lineWidth); }
    void setDashPattern(const QList<qreal>& dashPattern) { m_series->setDashPattern(dashPattern); }

signals:
    // Forwarded from the core series. Declared parameter-less for backward
    // compatibility with existing QML handlers; Qt allows connecting a
    // signal that carries arguments to one that takes fewer/none.
    void modelChanged();
    void colorChanged();
    void nameChanged();
    void lineWidthChanged();
    void dashPatternChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private slots:
    void handleDataChanged();

private:
    void connectForwardingSignals();
    void connectModelSignals();
    void disconnectModelSignals();
    void connectChartViewSignals();

    //! Core property backend — owned via Qt parent/child (this), so it is
    //! destroyed with the QmlLineSeries. All property state lives here.
    QLineSeries* const m_series;

    // Per-instance (NOT static — see #34): the ChartView this series is
    // currently connected to. QPointer so a disconnect() against an
    // already-destroyed chart (reparented away without going through
    // itemChange) is a safe no-op instead of dangling-pointer UB.
    QPointer<QmlChartView> m_previousChartView;
};

}  // namespace qgraphplot

#endif  // QMLLINESERIES_H
