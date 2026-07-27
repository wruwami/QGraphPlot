#ifndef QGRAPHPLOT_QMLAXIS_H
#define QGRAPHPLOT_QMLAXIS_H

#include <QtCore/QPointer>
#include <QtCore/QVariantList>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>

#include "transform/QScaleEngine.h"

namespace qgraphplot
{

class QmlChartView;

class QGRAPHPLOT_QML_EXPORT QmlAxis : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
        Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(int tickCount READ tickCount WRITE setTickCount NOTIFY tickCountChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY gridColorChanged)
    Q_PROPERTY(double lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(double gridWidth READ gridWidth WRITE setGridWidth NOTIFY gridWidthChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)
    Q_PROPERTY(QVariantList ticks READ ticks NOTIFY ticksChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

public:
    explicit QmlAxis(QQuickItem* parent = nullptr);
    ~QmlAxis() override = default;

    // Getters
    Qt::Orientation orientation() const noexcept { return m_orientation; }
    int tickCount() const noexcept { return m_tickCount; }
    QColor color() const noexcept { return m_color; }
    QColor gridColor() const noexcept { return m_gridColor; }

    //! Width of the axis baseline and its tick marks, in pixels.
    double lineWidth() const noexcept { return m_lineWidth; }

    //! Width of the grid lines, in pixels.
    double gridWidth() const noexcept { return m_gridWidth; }

    bool showGrid() const noexcept { return m_showGrid; }
    QVariantList ticks() const noexcept;
    [[nodiscard]] QString title() const noexcept { return m_title; }
    void setTitle(const QString& title);

    // Setters
    void setOrientation(Qt::Orientation orientation);
    void setTickCount(int count);
    void setColor(const QColor& color);
    void setGridColor(const QColor& gridColor);
    void setLineWidth(double lineWidth);
    void setGridWidth(double gridWidth);
    void setShowGrid(bool show);

signals:
    void orientationChanged();
    void tickCountChanged();
    void colorChanged();
    void gridColorChanged();
    void lineWidthChanged();
    void gridWidthChanged();
    void showGridChanged();
    void ticksChanged();
    void titleChanged();

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
    double m_lineWidth{1.0};
    double m_gridWidth{1.0};
    bool m_showGrid{true};

    std::vector<qgraphplot::QScaleEngine::TickInfo> m_tickInfos;

    QString m_title;

    // Per-instance (NOT static — see #34): the ChartView this axis is
    // currently connected to. QPointer so a disconnect() against an
    // already-destroyed chart (reparented away without going through
    // itemChange) is a safe no-op instead of dangling-pointer UB.
    QPointer<QmlChartView> m_previousChartView;

    // Track the axis range used to generate the current ticks, so we can
    // detect geometry-only transform changes (range unchanged, ticks
    // unchanged, but pixel positions different - see updateTicks()).
    double m_previousMin{0.0};
    double m_previousMax{0.0};
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_QMLAXIS_H
