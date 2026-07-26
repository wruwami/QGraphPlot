#ifndef QMLLINESERIES_H
#define QMLLINESERIES_H

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>

#include "model/QAbstractSeriesModel.h"

class QmlChartView;

class QmlLineSeries : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
        qgraphplot::QAbstractSeriesModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    explicit QmlLineSeries(QQuickItem* parent = nullptr);
    ~QmlLineSeries() override = default;

    // Getters
    qgraphplot::QAbstractSeriesModel* model() const noexcept { return m_model; }
    QColor color() const noexcept { return m_color; }
    QString name() const noexcept { return m_name; }

    // Setters
    void setModel(qgraphplot::QAbstractSeriesModel* model);
    void setColor(const QColor& color);
    void setName(const QString& name);

signals:
    void modelChanged();
    void colorChanged();
    void nameChanged();

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

    // Per-instance (NOT static — see #34): the ChartView this series is
    // currently connected to. QPointer so a disconnect() against an
    // already-destroyed chart (reparented away without going through
    // itemChange) is a safe no-op instead of dangling-pointer UB.
    QPointer<QmlChartView> m_previousChartView;
};

#endif  // QMLLINESERIES_H
