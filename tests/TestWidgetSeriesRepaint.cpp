// QGraphPlot — High-performance Qt chart library
//
// Licensed under the Apache License, Version 2.0.

#include <QtTest/QtTest>

#include "../src/widget_frontend/WidgetChartView.h"
#include "../src/widget_frontend/WidgetLineSeries.h"

namespace
{
class ObservableLineSeries : public qgraphplot::WidgetLineSeries
{
public:
    using qgraphplot::WidgetLineSeries::WidgetLineSeries;

    int colorReceiverCount() const { return receivers(SIGNAL(colorChanged(QColor))); }
    int visibleReceiverCount() const { return receivers(SIGNAL(visibleChanged(bool))); }
    int opacityReceiverCount() const { return receivers(SIGNAL(opacityChanged(double))); }
    int lineWidthReceiverCount() const { return receivers(SIGNAL(lineWidthChanged(double))); }
    int dashPatternReceiverCount() const { return receivers(SIGNAL(dashPatternChanged())); }
};
}  // namespace

class TestWidgetSeriesRepaint : public QObject
{
    Q_OBJECT

private slots:
    void addSeriesConnectsVisualPropertySignals();
    void removeSeriesDisconnectsVisualPropertySignals();
    void clearSeriesDisconnectsVisualPropertySignals();
};

void TestWidgetSeriesRepaint::addSeriesConnectsVisualPropertySignals()
{
    qgraphplot::WidgetChartView view;
    ObservableLineSeries series;

    const int colorBefore = series.colorReceiverCount();
    const int visibleBefore = series.visibleReceiverCount();
    const int opacityBefore = series.opacityReceiverCount();
    const int lineWidthBefore = series.lineWidthReceiverCount();
    const int dashBefore = series.dashPatternReceiverCount();

    view.addSeries(&series);

    QVERIFY(series.colorReceiverCount() > colorBefore);
    QVERIFY(series.visibleReceiverCount() > visibleBefore);
    QVERIFY(series.opacityReceiverCount() > opacityBefore);
    QVERIFY(series.lineWidthReceiverCount() > lineWidthBefore);
    QVERIFY(series.dashPatternReceiverCount() > dashBefore);
}

void TestWidgetSeriesRepaint::removeSeriesDisconnectsVisualPropertySignals()
{
    qgraphplot::WidgetChartView view;
    ObservableLineSeries series;
    view.addSeries(&series);

    const int colorConnected = series.colorReceiverCount();
    const int visibleConnected = series.visibleReceiverCount();
    const int opacityConnected = series.opacityReceiverCount();
    const int lineWidthConnected = series.lineWidthReceiverCount();
    const int dashConnected = series.dashPatternReceiverCount();

    view.removeSeries(&series);

    QVERIFY(series.colorReceiverCount() < colorConnected);
    QVERIFY(series.visibleReceiverCount() < visibleConnected);
    QVERIFY(series.opacityReceiverCount() < opacityConnected);
    QVERIFY(series.lineWidthReceiverCount() < lineWidthConnected);
    QVERIFY(series.dashPatternReceiverCount() < dashConnected);
}

void TestWidgetSeriesRepaint::clearSeriesDisconnectsVisualPropertySignals()
{
    qgraphplot::WidgetChartView view;
    ObservableLineSeries series;
    view.addSeries(&series);

    const int colorConnected = series.colorReceiverCount();
    const int visibleConnected = series.visibleReceiverCount();
    const int opacityConnected = series.opacityReceiverCount();
    const int lineWidthConnected = series.lineWidthReceiverCount();
    const int dashConnected = series.dashPatternReceiverCount();

    view.clearSeries();

    QVERIFY(series.colorReceiverCount() < colorConnected);
    QVERIFY(series.visibleReceiverCount() < visibleConnected);
    QVERIFY(series.opacityReceiverCount() < opacityConnected);
    QVERIFY(series.lineWidthReceiverCount() < lineWidthConnected);
    QVERIFY(series.dashPatternReceiverCount() < dashConnected);
}

QTEST_MAIN(TestWidgetSeriesRepaint)
#include "TestWidgetSeriesRepaint.moc"
