#ifndef QMLAXIS_H
#define QMLAXIS_H

#include <QtCore/QPointer>
#include <QtCore/QVariantList>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>

#include "transform/QScaleEngine.h"

class QmlChartView;

class QmlAxis : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
        Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(int tickCount READ tickCount WRITE setTickCount NOTIFY tickCountChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY gridColorChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)
    Q_PROPERTY(QVariantList ticks READ ticks NOTIFY ticksChanged)

public:
    explicit QmlAxis(QQuickItem* parent = nullptr);
    ~QmlAxis() override = default;

    // Getters
    Qt::Orientation orientation() const noexcept { return m_orientation; }
    int tickCount() const noexcept { return m_tickCount; }
    QColor color() const noexcept { return m_color; }
    QColor gridColor() const noexcept { return m_gridColor; }
    bool showGrid() const noexcept { return m_showGrid; }
    QVariantList ticks() const noexcept;

    // Setters
    void setOrientation(Qt::Orientation orientation);
    void setTickCount(int count);
    void setColor(const QColor& color);
    void setGridColor(const QColor& gridColor);
    void setShowGrid(bool show);

signals:
    void orientationChanged();
    void tickCountChanged();
    void colorChanged();
    void gridColorChanged();
    void showGridChanged();
    void ticksChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private slots:
    void handleTransformChanged();

private:
    void connectChartViewSignals();
    void updateTicks();

    Qt::Orientation m_orientation{Qt::Horizontal};
    int m_tickCount{5};
    QColor m_color{Qt::gray};
    QColor m_gridColor{0xE0, 0xE0, 0xE0};  // 아주 연한 회색
    bool m_showGrid{true};

    std::vector<qgraphplot::QScaleEngine::TickInfo> m_tickInfos;

    // Per-instance (NOT static — see #34): the ChartView this axis is
    // currently connected to. QPointer so a disconnect() against an
    // already-destroyed chart (reparented away without going through
    // itemChange) is a safe no-op instead of dangling-pointer UB.
    QPointer<QmlChartView> m_previousChartView;
};

#endif  // QMLAXIS_H
