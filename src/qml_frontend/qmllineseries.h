#ifndef QMLLINESERIES_H
#define QMLLINESERIES_H

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>

#include "model/QAbstractSeriesModel.h"

namespace qgraphplot
{

class QmlChartView;

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

    // Getters
    qgraphplot::QAbstractSeriesModel* model() const noexcept { return m_model; }
    QColor color() const noexcept { return m_color; }
    QString name() const noexcept { return m_name; }

    //! Stroke width in device-independent pixels.
    double lineWidth() const noexcept { return m_lineWidth; }

    //! Alternating dash/gap lengths in units of lineWidth (QPen semantics).
    //! Empty means a solid line. Validated by
    //! qgraphplot::QAbstractSeries::isValidDashPattern() so the QML and
    //! Widget series accept exactly the same patterns (AI.md §3.1).
    QList<qreal> dashPattern() const { return m_dashPattern; }

    // Setters
    void setModel(qgraphplot::QAbstractSeriesModel* model);
    void setColor(const QColor& color);
    void setName(const QString& name);
    void setLineWidth(double lineWidth);
    void setDashPattern(const QList<qreal>& dashPattern);

signals:
    void modelChanged();
    void colorChanged();
    void nameChanged();
    void lineWidthChanged();
    void dashPatternChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private slots:
    void handleModelReset();
    void handleDataChanged();

private:
    void connectModelSignals();
    void disconnectModelSignals();
    void connectChartViewSignals();

    qgraphplot::QAbstractSeriesModel* m_model{nullptr};
    QColor m_color{Qt::blue};
    QString m_name;
    double m_lineWidth{2.0};
    QList<qreal> m_dashPattern;

    // Per-instance (NOT static — see #34): the ChartView this series is
    // currently connected to. QPointer so a disconnect() against an
    // already-destroyed chart (reparented away without going through
    // itemChange) is a safe no-op instead of dangling-pointer UB.
    QPointer<QmlChartView> m_previousChartView;
};

}  // namespace qgraphplot

#endif  // QMLLINESERIES_H
