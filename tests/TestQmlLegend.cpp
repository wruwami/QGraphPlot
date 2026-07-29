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

//! @file TestQmlLegend.cpp
//! @brief Unit tests for qgraphplot::QmlLegendItem and qgraphplot::QmlLegend
//!        (per-series legend entries auto-populated from a QmlChartView's
//!        series collection, issue #65), plus QML-level integration tests
//!        for Legend.qml's rendering/toggle behavior and position layout (#93).

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QScopedPointer>
#include <QtGui/QColor>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlProperty>
#include <QtQuick/QQuickItem>
#include <QtTest/QtTest>

#include "../src/core/series/QLineSeries.h"
#include "../src/qml_frontend/QmlChartView.h"
#include "../src/qml_frontend/QmlLegend.h"

using qgraphplot::QLineSeries;
using qgraphplot::QmlChartView;
using qgraphplot::QmlLegend;
using qgraphplot::QmlLegendItem;

namespace
{

QString qmlImportPath()
{
    // The QML plugin directory (QGraphPlot/) is staged next to the test binary
    // via CMake POST_BUILD (see tests/CMakeLists.txt).  QQmlEngine adds
    // applicationDirPath() to its import paths by default, but an explicit
    // addImportPath call here ensures the module is found regardless of how
    // the engine's default list is populated.
    return QCoreApplication::applicationDirPath();
}

QObject* createQmlObject(const QByteArray& source, QQmlEngine& engine, QString* errorString)
{
    engine.addImportPath(qmlImportPath());
    QQmlComponent component(&engine);
    component.setData(source, QUrl());
    QObject* object = component.create();
    if (!object && errorString) {
        *errorString = component.errorString();
    }
    return object;
}

//! Finds the first descendant of @p root exposing a "text" property (i.e. a
//! QtQuick Text item) whose current value equals @p text.
QQuickItem* findTextItem(QQuickItem* root, const QString& text)
{
    const auto children = root->findChildren<QQuickItem*>();
    for (QQuickItem* child : children) {
        QQmlProperty textProp(child, QStringLiteral("text"));
        if (textProp.isValid() && textProp.read().toString() == text) {
            return child;
        }
    }
    return nullptr;
}

//! Prefer the QML Legend.qml Text row (Rectangle marker sibling) over the
//! C++ visual-row label, so toggle/position tests exercise the public skin.
QQuickItem* findQmlLegendNameText(QQuickItem* legend, const QString& text)
{
    const auto children = legend->findChildren<QQuickItem*>();
    for (QQuickItem* child : children) {
        QQmlProperty textProp(child, QStringLiteral("text"));
        if (!textProp.isValid() || textProp.read().toString() != text) {
            continue;
        }
        QQuickItem* parent = child->parentItem();
        if (!parent) {
            continue;
        }
        // QML Row delegate: first child is a Rectangle with a "color" property
        // and typically a border (C++ marker is also colored but has no border).
        const auto siblings = parent->childItems();
        if (siblings.isEmpty()) {
            continue;
        }
        QQuickItem* marker = siblings.first();
        QQmlProperty borderProp(marker, QStringLiteral("border"));
        if (borderProp.isValid()) {
            return child;
        }
    }
    // Fallback: any matching text item (C++ visual row).
    return findTextItem(legend, text);
}

}  // namespace

class TestQmlLegend : public QObject
{
    Q_OBJECT

private slots:
    void legendItemDefaultStateIsEmpty();
    void legendItemSetSeriesPopulatesProperties();
    void legendItemForwardsNameColorVisibleChanges();
    void legendItemToggleFlipsSeriesVisibility();
    void legendItemToggleWithoutSeriesIsSafeNoOp();
    void legendItemSetSeriesSameValueIsNoOp();
    void legendItemSetSeriesNullResetsProperties();
    void legendItemSetSeriesDisconnectsPreviousSeries();
    void legendItemSurvivesSeriesDestruction();

    void legendDefaultsAreEmpty();
    void legendSetChartPopulatesItemsFromExistingSeries();
    void legendSetChartSameValueIsNoOp();
    void legendSetChartNullClearsItems();
    void legendTracksSeriesAddedToChart();
    void legendTracksSeriesRemovedFromChart();
    void legendSetPositionEmitsAndUpdates();
    void legendSetPositionSameValueIsNoOp();
    void legendReparentingPreservesItemConsistency();

    void legendQmlRendersSeriesNameAndColor();
    void legendQmlTogglingItemDimsRowDelegate();
    void legendQmlPositionBottomRightPlacesColumn();
};

void TestQmlLegend::legendItemDefaultStateIsEmpty()
{
    QmlLegendItem item;
    QCOMPARE(item.name(), QString());
    QVERIFY(!item.color().isValid());
    QCOMPARE(item.visible(), true);
    QCOMPARE(item.series(), nullptr);
}

void TestQmlLegend::legendItemSetSeriesPopulatesProperties()
{
    QLineSeries series;
    series.setName(QStringLiteral("Alpha"));
    series.setColor(QColor(Qt::red));
    series.setVisible(false);

    QmlLegendItem item;
    QSignalSpy nameSpy(&item, &QmlLegendItem::nameChanged);
    QSignalSpy colorSpy(&item, &QmlLegendItem::colorChanged);
    QSignalSpy visibleSpy(&item, &QmlLegendItem::visibleChanged);

    item.setSeries(&series);

    QCOMPARE(item.name(), QStringLiteral("Alpha"));
    QCOMPARE(item.color(), QColor(Qt::red));
    QCOMPARE(item.visible(), false);
    QCOMPARE(item.series(), &series);
    QCOMPARE(nameSpy.count(), 1);
    QCOMPARE(colorSpy.count(), 1);
    QCOMPARE(visibleSpy.count(), 1);
}

void TestQmlLegend::legendItemForwardsNameColorVisibleChanges()
{
    QLineSeries series;
    QmlLegendItem item;
    item.setSeries(&series);

    QSignalSpy nameSpy(&item, &QmlLegendItem::nameChanged);
    QSignalSpy colorSpy(&item, &QmlLegendItem::colorChanged);
    QSignalSpy visibleSpy(&item, &QmlLegendItem::visibleChanged);

    series.setName(QStringLiteral("Beta"));
    QCOMPARE(item.name(), QStringLiteral("Beta"));
    QCOMPARE(nameSpy.count(), 1);

    series.setColor(QColor(Qt::green));
    QCOMPARE(item.color(), QColor(Qt::green));
    QCOMPARE(colorSpy.count(), 1);

    series.setVisible(false);
    QCOMPARE(item.visible(), false);
    QCOMPARE(visibleSpy.count(), 1);
}

void TestQmlLegend::legendItemToggleFlipsSeriesVisibility()
{
    QLineSeries series;
    QmlLegendItem item;
    item.setSeries(&series);
    QVERIFY(item.visible());

    item.toggle();
    QVERIFY(!series.isVisible());
    QVERIFY(!item.visible());

    item.toggle();
    QVERIFY(series.isVisible());
    QVERIFY(item.visible());
}

void TestQmlLegend::legendItemToggleWithoutSeriesIsSafeNoOp()
{
    QmlLegendItem item;
    item.toggle();
    QCOMPARE(item.series(), nullptr);
    QCOMPARE(item.visible(), true);
}

void TestQmlLegend::legendItemSetSeriesSameValueIsNoOp()
{
    QLineSeries series;
    QmlLegendItem item;
    item.setSeries(&series);

    QSignalSpy nameSpy(&item, &QmlLegendItem::nameChanged);
    item.setSeries(&series);
    QCOMPARE(nameSpy.count(), 0);

    QmlLegendItem neverAssigned;
    QSignalSpy neverSpy(&neverAssigned, &QmlLegendItem::nameChanged);
    neverAssigned.setSeries(nullptr);
    QCOMPARE(neverSpy.count(), 0);
}

void TestQmlLegend::legendItemSetSeriesNullResetsProperties()
{
    QLineSeries series;
    series.setName(QStringLiteral("Gamma"));
    series.setColor(QColor(Qt::green));
    series.setVisible(false);

    QmlLegendItem item;
    item.setSeries(&series);

    QSignalSpy nameSpy(&item, &QmlLegendItem::nameChanged);
    QSignalSpy colorSpy(&item, &QmlLegendItem::colorChanged);
    QSignalSpy visibleSpy(&item, &QmlLegendItem::visibleChanged);

    item.setSeries(nullptr);

    QCOMPARE(item.name(), QString());
    QCOMPARE(item.color(), QColor(Qt::transparent));
    QCOMPARE(item.visible(), true);
    QCOMPARE(item.series(), nullptr);
    QCOMPARE(nameSpy.count(), 1);
    QCOMPARE(colorSpy.count(), 1);
    QCOMPARE(visibleSpy.count(), 1);
}

void TestQmlLegend::legendItemSetSeriesDisconnectsPreviousSeries()
{
    QLineSeries s1;
    s1.setName(QStringLiteral("First"));
    QLineSeries s2;
    s2.setName(QStringLiteral("Second"));

    QmlLegendItem item;
    item.setSeries(&s1);
    item.setSeries(&s2);
    QCOMPARE(item.name(), QStringLiteral("Second"));

    QSignalSpy nameSpy(&item, &QmlLegendItem::nameChanged);

    s1.setName(QStringLiteral("FirstChanged"));
    QCOMPARE(nameSpy.count(), 0);
    QCOMPARE(item.name(), QStringLiteral("Second"));

    s2.setName(QStringLiteral("SecondChanged"));
    QCOMPARE(nameSpy.count(), 1);
    QCOMPARE(item.name(), QStringLiteral("SecondChanged"));
}

void TestQmlLegend::legendItemSurvivesSeriesDestruction()
{
    auto* series = new QLineSeries();
    QmlLegendItem item;
    item.setSeries(series);
    QCOMPARE(item.series(), series);

    delete series;

    QCOMPARE(item.series(), nullptr);
    item.toggle();
}

void TestQmlLegend::legendDefaultsAreEmpty()
{
    QmlLegend legend;
    QCOMPARE(legend.chart(), nullptr);
    QVERIFY(legend.items().isEmpty());
    QCOMPARE(legend.position(), Qt::Alignment(Qt::AlignTop));
}

void TestQmlLegend::legendSetChartPopulatesItemsFromExistingSeries()
{
    QmlChartView chart;
    auto* s1 = new QLineSeries(&chart);
    s1->setName(QStringLiteral("A"));
    auto* s2 = new QLineSeries(&chart);
    s2->setName(QStringLiteral("B"));
    chart.addSeries(s1);
    chart.addSeries(s2);

    QmlLegend legend;
    QSignalSpy chartSpy(&legend, &QmlLegend::chartChanged);
    QSignalSpy itemsSpy(&legend, &QmlLegend::itemsChanged);

    legend.setChart(&chart);

    QCOMPARE(legend.chart(), &chart);
    QCOMPARE(legend.items().size(), 2);
    QCOMPARE(chartSpy.count(), 1);
    QCOMPARE(itemsSpy.count(), 1);

    auto* item0 = legend.items().at(0).value<QmlLegendItem*>();
    auto* item1 = legend.items().at(1).value<QmlLegendItem*>();
    QVERIFY(item0);
    QVERIFY(item1);
    QCOMPARE(item0->series(), s1);
    QCOMPARE(item1->series(), s2);
    QCOMPARE(item0->name(), QStringLiteral("A"));
    QCOMPARE(item1->name(), QStringLiteral("B"));
}

void TestQmlLegend::legendSetChartSameValueIsNoOp()
{
    QmlChartView chart;
    QmlLegend legend;
    legend.setChart(&chart);

    QSignalSpy chartSpy(&legend, &QmlLegend::chartChanged);
    legend.setChart(&chart);
    QCOMPARE(chartSpy.count(), 0);
}

void TestQmlLegend::legendSetChartNullClearsItems()
{
    QmlChartView chart;
    auto* s1 = new QLineSeries(&chart);
    chart.addSeries(s1);

    QmlLegend legend;
    legend.setChart(&chart);
    QCOMPARE(legend.items().size(), 1);

    QSignalSpy itemsSpy(&legend, &QmlLegend::itemsChanged);
    legend.setChart(nullptr);

    QCOMPARE(legend.chart(), nullptr);
    QVERIFY(legend.items().isEmpty());
    QCOMPARE(itemsSpy.count(), 1);
}

void TestQmlLegend::legendTracksSeriesAddedToChart()
{
    QmlChartView chart;
    QmlLegend legend;
    legend.setChart(&chart);
    QVERIFY(legend.items().isEmpty());

    QSignalSpy itemsSpy(&legend, &QmlLegend::itemsChanged);
    auto* s = new QLineSeries(&chart);
    s->setName(QStringLiteral("New"));
    chart.addSeries(s);

    QCOMPARE(legend.items().size(), 1);
    QVERIFY(itemsSpy.count() >= 1);
    auto* item = legend.items().at(0).value<QmlLegendItem*>();
    QVERIFY(item);
    QCOMPARE(item->series(), s);
    QCOMPARE(item->name(), QStringLiteral("New"));
}

void TestQmlLegend::legendTracksSeriesRemovedFromChart()
{
    QmlChartView chart;
    auto* s1 = new QLineSeries(&chart);
    auto* s2 = new QLineSeries(&chart);
    chart.addSeries(s1);
    chart.addSeries(s2);

    QmlLegend legend;
    legend.setChart(&chart);
    QCOMPARE(legend.items().size(), 2);

    chart.removeSeries(s1);

    QCOMPARE(legend.items().size(), 1);
    auto* remaining = legend.items().at(0).value<QmlLegendItem*>();
    QVERIFY(remaining);
    QCOMPARE(remaining->series(), s2);
}

void TestQmlLegend::legendSetPositionEmitsAndUpdates()
{
    QmlLegend legend;
    QSignalSpy positionSpy(&legend, &QmlLegend::positionChanged);

    legend.setPosition(Qt::AlignBottom | Qt::AlignRight);

    QCOMPARE(legend.position(), Qt::Alignment(Qt::AlignBottom | Qt::AlignRight));
    QCOMPARE(positionSpy.count(), 1);
}

void TestQmlLegend::legendSetPositionSameValueIsNoOp()
{
    QmlLegend legend;
    legend.setPosition(Qt::AlignBottom);

    QSignalSpy positionSpy(&legend, &QmlLegend::positionChanged);
    legend.setPosition(Qt::AlignBottom);
    QCOMPARE(positionSpy.count(), 0);
}

void TestQmlLegend::legendReparentingPreservesItemConsistency()
{
    QmlChartView chart;
    auto* s1 = new QLineSeries(&chart);
    chart.addSeries(s1);

    QmlLegend legend;
    legend.setChart(&chart);
    QCOMPARE(legend.items().size(), 1);

    QQuickItem newParent;
    legend.setParentItem(&newParent);

    auto* s2 = new QLineSeries(&chart);
    chart.addSeries(s2);

    QCOMPARE(legend.items().size(), 2);
    auto* item0 = legend.items().at(0).value<QmlLegendItem*>();
    auto* item1 = legend.items().at(1).value<QmlLegendItem*>();
    QVERIFY(item0);
    QVERIFY(item1);
    QCOMPARE(item0->series(), s1);
    QCOMPARE(item1->series(), s2);
}

void TestQmlLegend::legendQmlRendersSeriesNameAndColor()
{
    QQmlEngine engine;
    QString error;
    QScopedPointer<QObject> root(createQmlObject(
        QByteArrayLiteral(
            "import QtQuick\nimport QGraphPlot\nItem {\n"
            "  QmlChartView { id: chartView; objectName: \"chart\"\n"
            "    QmlLineSeries { objectName: \"series\"; name: \"Series A\"; color: \"red\" }\n"
            "  }\n"
            "  Legend { id: legendItem; objectName: \"legend\"; chart: chartView; width: 200; "
            "height: 100 }\n"
            "}\n"),
        engine,
        &error));
    QVERIFY2(root, qPrintable(error));

    auto* chart = root->findChild<QmlChartView*>(QStringLiteral("chart"));
    QVERIFY(chart);
    QTRY_COMPARE(chart->seriesCount(), 1);

    auto* legend = root->findChild<QmlLegend*>(QStringLiteral("legend"));
    QVERIFY(legend);
    QTRY_COMPARE(legend->items().size(), 1);
    QCOMPARE(legend->chart(), chart);

    QQuickItem* nameText = findQmlLegendNameText(legend, QStringLiteral("Series A"));
    QVERIFY2(nameText, "Legend.qml did not render the series name Text delegate");

    QQuickItem* rowDelegate = nameText->parentItem();
    QVERIFY(rowDelegate);
    const auto rowChildren = rowDelegate->childItems();
    QVERIFY(!rowChildren.isEmpty());
    QQuickItem* marker = rowChildren.first();
    QQmlProperty colorProp(marker, QStringLiteral("color"));
    QVERIFY(colorProp.isValid());
    QCOMPARE(colorProp.read().value<QColor>(), QColor(Qt::red));
}

void TestQmlLegend::legendQmlTogglingItemDimsRowDelegate()
{
    // Legend.qml uses opacity (not visible=false) so the row stays interactive.
    // Prefer the QML Text delegate so we do not assert on C++ visual rows.
    QQmlEngine engine;
    QString error;
    QScopedPointer<QObject> root(createQmlObject(
        QByteArrayLiteral(
            "import QtQuick\nimport QGraphPlot\nItem {\n"
            "  QmlChartView { id: chartView; objectName: \"chart\"\n"
            "    QmlLineSeries { objectName: \"series\"; name: \"Series A\" }\n"
            "  }\n"
            "  Legend { objectName: \"legend\"; chart: chartView; width: 200; height: 100 }\n"
            "}\n"),
        engine,
        &error));
    QVERIFY2(root, qPrintable(error));

    auto* chart = root->findChild<QmlChartView*>(QStringLiteral("chart"));
    QVERIFY(chart);
    QTRY_COMPARE(chart->seriesCount(), 1);

    auto* legend = root->findChild<QmlLegend*>(QStringLiteral("legend"));
    QVERIFY(legend);
    QTRY_COMPARE(legend->items().size(), 1);

    QQuickItem* nameText = findQmlLegendNameText(legend, QStringLiteral("Series A"));
    QVERIFY2(nameText, "Legend.qml did not render the series name Text delegate");
    QQuickItem* rowDelegate = nameText->parentItem();
    QVERIFY(rowDelegate);
    QQmlProperty rowVisible(rowDelegate, QStringLiteral("visible"));
    QVERIFY(rowVisible.isValid());
    QCOMPARE(rowVisible.read().toBool(), true);

    auto* legendItem = legend->items().at(0).value<QmlLegendItem*>();
    QVERIFY(legendItem);

    legendItem->toggle();

    QVERIFY(!chart->seriesAt(0)->isVisible());
    QTRY_VERIFY(rowVisible.read().toBool());
    QQmlProperty rowOpacity(rowDelegate, QStringLiteral("opacity"));
    QVERIFY(rowOpacity.isValid());
    QTRY_VERIFY(rowOpacity.read().toDouble() < 0.5);
}

void TestQmlLegend::legendQmlPositionBottomRightPlacesColumn()
{
    // issue #93: setting position must move the legend column on-screen.
    QQmlEngine engine;
    QString error;
    QScopedPointer<QObject> root(
        createQmlObject(QByteArrayLiteral("import QtQuick\nimport QGraphPlot\nItem {\n"
                                          "  width: 400; height: 300\n"
                                          "  QmlChartView { id: chartView; objectName: \"chart\"\n"
                                          "    QmlLineSeries { name: \"Series A\" }\n"
                                          "  }\n"
                                          "  Legend {\n"
                                          "    id: legendItem\n"
                                          "    objectName: \"legend\"\n"
                                          "    chart: chartView\n"
                                          "    anchors.fill: parent\n"
                                          "    position: Qt.AlignTop\n"
                                          "  }\n"
                                          "}\n"),
                        engine,
                        &error));
    QVERIFY2(root, qPrintable(error));

    auto* legend = root->findChild<QmlLegend*>(QStringLiteral("legend"));
    QVERIFY(legend);
    QTRY_COMPARE(legend->items().size(), 1);

    // Legend.qml registers the Column as objectName legendColumn.
    QQuickItem* column = nullptr;
    QTRY_VERIFY((column = legend->findChild<QQuickItem*>(QStringLiteral("legendColumn"))));

    // Default AlignTop → top-left of the legend bounds.
    QTRY_VERIFY(column->y() < 20.0);
    QTRY_VERIFY(column->x() < 20.0);

    legend->setPosition(Qt::AlignBottom | Qt::AlignRight);
    QCOMPARE(legend->position(), Qt::Alignment(Qt::AlignBottom | Qt::AlignRight));

    QTRY_VERIFY(column->y() + column->height() > legend->height() - 30.0);
    QTRY_VERIFY(column->x() + column->width() > legend->width() - 30.0);
}

QTEST_MAIN(TestQmlLegend)
#include "TestQmlLegend.moc"
