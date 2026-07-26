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

//! @file TestQmlChartView.cpp
//! @brief Unit tests for qgraphplot::QmlChartView's axis/margin setters and
//!        the series-collection API (add/remove/clear/enumerate, destroyed
//!        cleanup, and itemChange child registration): #58, #59, #35.

#include <limits>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtTest/QtTest>

#include "../src/core/series/QLineSeries.h"
#include "../src/qml_frontend/QmlChartView.h"
#include "../src/qml_frontend/QmlLineSeries.h"
#include "../src/qml_frontend/QmlScatterSeries.h"

using qgraphplot::QAbstractSeries;
using qgraphplot::QLineSeries;
using qgraphplot::QmlChartView;
using qgraphplot::QmlLineSeries;
using qgraphplot::QmlScatterSeries;

namespace
{

//! Builds a regex that matches the fixed, literal prefix of a qCWarning
//! message. The setters append the rejected value after the prefix via
//! QDebug's stream operator, so exact-string matching would be brittle.
QRegularExpression warningPrefix(const QString& prefix)
{
    return QRegularExpression(QRegularExpression::escape(prefix));
}

}  // namespace

class TestQmlChartView : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreSane();

    void setXMinAcceptsValidValue();
    void setXMinRejectsInvalidValues();
    void setXMaxAcceptsValidValue();
    void setXMaxRejectsInvalidValues();
    void setYMinRejectsInvalidValues();
    void setYMaxRejectsInvalidValues();

    void setMarginAcceptsZeroAndPositive();
    void setMarginRejectsNegativeOrNonFinite();
    void marginsChangedIsSharedAcrossAllFourMargins();

    void redundantWritesDoNotEmit();
    void wideningRangeAllowsPreviouslyInvalidValue();

    // ── Series collection (issue #58) ──────────────────────────
    void seriesCollectionStartsEmpty();
    void addSeriesAppendsAndEmits();
    void addSeriesIgnoresNullAndDuplicates();
    void removeSeriesRemovesAndEmits();
    void removeSeriesIgnoresUnknownSeries();
    void clearSeriesRemovesAllAndEmits();
    void seriesAtReturnsIndexedOrNull();
    void seriesCountReflectsCollection();
    void destroyedSeriesIsDroppedFromCollection();
    void declarativeChildRegistersCoreSeries();
    void declarativeLineAndScatterChildrenBothRegistered();
    void clearSeriesOnDestructorIsSafe();
};

void TestQmlChartView::defaultsAreSane()
{
    QmlChartView view;
    QCOMPARE(view.xMin(), 0.0);
    QCOMPARE(view.xMax(), 10.0);
    QCOMPARE(view.yMin(), 0.0);
    QCOMPARE(view.yMax(), 10.0);
    QCOMPARE(view.marginLeft(), 50.0);
    QCOMPARE(view.marginRight(), 20.0);
    QCOMPARE(view.marginTop(), 20.0);
    QCOMPARE(view.marginBottom(), 40.0);
}

void TestQmlChartView::setXMinAcceptsValidValue()
{
    QmlChartView view;
    QSignalSpy minSpy(&view, &QmlChartView::xMinChanged);
    QSignalSpy transformSpy(&view, &QmlChartView::transformChanged);

    view.setXMin(-5.0);

    QCOMPARE(view.xMin(), -5.0);
    QCOMPARE(minSpy.count(), 1);
    QCOMPARE(transformSpy.count(), 1);
}

void TestQmlChartView::setXMinRejectsInvalidValues()
{
    QmlChartView view;
    QSignalSpy minSpy(&view, &QmlChartView::xMinChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "QmlChartView::setXMin: rejected non-finite or invalid xMin (must be < xMax):"));

    for (const double invalid : {std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity(),
                                 -std::numeric_limits<double>::infinity(),
                                 10.0,     // == xMax: violates "must be < xMax"
                                 15.0}) {  // > xMax
        QTest::ignoreMessage(QtWarningMsg, pattern);
        view.setXMin(invalid);
        QCOMPARE(view.xMin(), 0.0);
    }
    QCOMPARE(minSpy.count(), 0);
}

void TestQmlChartView::setXMaxAcceptsValidValue()
{
    QmlChartView view;
    QSignalSpy maxSpy(&view, &QmlChartView::xMaxChanged);
    QSignalSpy transformSpy(&view, &QmlChartView::transformChanged);

    view.setXMax(20.0);

    QCOMPARE(view.xMax(), 20.0);
    QCOMPARE(maxSpy.count(), 1);
    QCOMPARE(transformSpy.count(), 1);
}

void TestQmlChartView::setXMaxRejectsInvalidValues()
{
    QmlChartView view;
    QSignalSpy maxSpy(&view, &QmlChartView::xMaxChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "QmlChartView::setXMax: rejected non-finite or invalid xMax (must be > xMin):"));

    for (const double invalid : {std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity(),
                                 0.0,      // == xMin: violates "must be > xMin"
                                 -5.0}) {  // < xMin
        QTest::ignoreMessage(QtWarningMsg, pattern);
        view.setXMax(invalid);
        QCOMPARE(view.xMax(), 10.0);
    }
    QCOMPARE(maxSpy.count(), 0);
}

void TestQmlChartView::setYMinRejectsInvalidValues()
{
    QmlChartView view;
    QSignalSpy minSpy(&view, &QmlChartView::yMinChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "QmlChartView::setYMin: rejected non-finite or invalid yMin (must be < yMax):"));

    for (const double invalid : {std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity(),
                                 -std::numeric_limits<double>::infinity(),
                                 10.0,     // == yMax: violates "must be < yMax"
                                 12.0}) {  // > yMax
        QTest::ignoreMessage(QtWarningMsg, pattern);
        view.setYMin(invalid);
        QCOMPARE(view.yMin(), 0.0);
    }
    QCOMPARE(minSpy.count(), 0);

    // A valid change still works after the rejections above.
    view.setYMin(2.0);
    QCOMPARE(view.yMin(), 2.0);
    QCOMPARE(minSpy.count(), 1);
}

void TestQmlChartView::setYMaxRejectsInvalidValues()
{
    QmlChartView view;
    QSignalSpy maxSpy(&view, &QmlChartView::yMaxChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "QmlChartView::setYMax: rejected non-finite or invalid yMax (must be > yMin):"));

    for (const double invalid : {std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity(),
                                 -std::numeric_limits<double>::infinity(),
                                 0.0,      // == yMin: violates "must be > yMin"
                                 -1.0}) {  // < yMin
        QTest::ignoreMessage(QtWarningMsg, pattern);
        view.setYMax(invalid);
        QCOMPARE(view.yMax(), 10.0);
    }
    QCOMPARE(maxSpy.count(), 0);
}

void TestQmlChartView::setMarginAcceptsZeroAndPositive()
{
    QmlChartView view;
    QSignalSpy marginsSpy(&view, &QmlChartView::marginsChanged);

    // Boundary: exactly 0.0 is accepted ("< 0.0" is rejected, not "<= 0.0").
    view.setMarginLeft(0.0);
    QCOMPARE(view.marginLeft(), 0.0);
    QCOMPARE(marginsSpy.count(), 1);

    view.setMarginRight(100.0);
    QCOMPARE(view.marginRight(), 100.0);
    QCOMPARE(marginsSpy.count(), 2);
}

void TestQmlChartView::setMarginRejectsNegativeOrNonFinite()
{
    QmlChartView view;
    QSignalSpy marginsSpy(&view, &QmlChartView::marginsChanged);

    struct Setter
    {
        void (QmlChartView::*fn)(double);
        const char* name;
    };
    const Setter setters[] = {
        {&QmlChartView::setMarginLeft, "setMarginLeft"},
        {&QmlChartView::setMarginRight, "setMarginRight"},
        {&QmlChartView::setMarginTop, "setMarginTop"},
        {&QmlChartView::setMarginBottom, "setMarginBottom"},
    };

    for (const Setter& setter : setters) {
        const QRegularExpression pattern =
            warningPrefix(QStringLiteral("QmlChartView::%1: rejected negative or non-finite "
                                         "margin:")
                              .arg(QString::fromLatin1(setter.name)));

        for (const double invalid : {-1.0,
                                     std::numeric_limits<double>::quiet_NaN(),
                                     std::numeric_limits<double>::infinity(),
                                     -std::numeric_limits<double>::infinity()}) {
            QTest::ignoreMessage(QtWarningMsg, pattern);
            (view.*setter.fn)(invalid);
        }
    }

    QCOMPARE(view.marginLeft(), 50.0);
    QCOMPARE(view.marginRight(), 20.0);
    QCOMPARE(view.marginTop(), 20.0);
    QCOMPARE(view.marginBottom(), 40.0);
    QCOMPARE(marginsSpy.count(), 0);
}

void TestQmlChartView::marginsChangedIsSharedAcrossAllFourMargins()
{
    QmlChartView view;
    QSignalSpy marginsSpy(&view, &QmlChartView::marginsChanged);
    QSignalSpy transformSpy(&view, &QmlChartView::transformChanged);

    view.setMarginLeft(10.0);
    view.setMarginRight(11.0);
    view.setMarginTop(12.0);
    view.setMarginBottom(13.0);

    QCOMPARE(marginsSpy.count(), 4);
    QCOMPARE(transformSpy.count(), 4);
}

void TestQmlChartView::redundantWritesDoNotEmit()
{
    QmlChartView view;
    view.setXMin(-5.0);

    QSignalSpy minSpy(&view, &QmlChartView::xMinChanged);
    QSignalSpy transformSpy(&view, &QmlChartView::transformChanged);

    // Re-applying the same value is a no-op signal-wise (fuzzyValuesDiffer).
    view.setXMin(-5.0);
    QCOMPARE(minSpy.count(), 0);
    QCOMPARE(transformSpy.count(), 0);
}

void TestQmlChartView::wideningRangeAllowsPreviouslyInvalidValue()
{
    QmlChartView view;

    // xMin=20.0 is rejected against the default xMax=10.0...
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "QmlChartView::setXMin: rejected non-finite or invalid xMin (must be < xMax):"));
    QTest::ignoreMessage(QtWarningMsg, pattern);
    view.setXMin(20.0);
    QCOMPARE(view.xMin(), 0.0);

    // ...but becomes valid once xMax is widened past it.
    view.setXMax(30.0);
    view.setXMin(20.0);
    QCOMPARE(view.xMin(), 20.0);
}

// ════════════════════════════════════════════════════════════════
// Series collection (issue #58)
//
// The QmlChartView tracks the *core* QAbstractSeries* objects composed by
// the QML wrappers (QmlLineSeries/QmlScatterSeries), mirroring the
// WidgetChartView API (AI.md §3.1). These tests cover add/remove/clear/
// enumerate, the seriesAdded/seriesRemoved signals, destroyed-cleanup, and
// automatic registration of declarative QML children via itemChange.
// ════════════════════════════════════════════════════════════════

void TestQmlChartView::seriesCollectionStartsEmpty()
{
    QmlChartView view;
    QCOMPARE(view.series().size(), 0);
    QCOMPARE(view.seriesCount(), 0);
    QVERIFY(view.series().isEmpty());
}

void TestQmlChartView::addSeriesAppendsAndEmits()
{
    QmlChartView view;
    QSignalSpy addedSpy(&view, &QmlChartView::seriesAdded);

    auto* core = new QLineSeries(&view);  // owned by view (QObject parent)
    view.addSeries(core);

    QCOMPARE(view.seriesCount(), 1);
    QCOMPARE(view.series().at(0), core);
    QCOMPARE(addedSpy.count(), 1);
    QCOMPARE(addedSpy.at(0).at(0).value<QAbstractSeries*>(), core);
}

void TestQmlChartView::addSeriesIgnoresNullAndDuplicates()
{
    QmlChartView view;
    QSignalSpy addedSpy(&view, &QmlChartView::seriesAdded);

    auto* core = new QLineSeries(&view);

    view.addSeries(nullptr);  // null is a no-op
    QCOMPARE(view.seriesCount(), 0);
    QCOMPARE(addedSpy.count(), 0);

    view.addSeries(core);
    QCOMPARE(view.seriesCount(), 1);
    QCOMPARE(addedSpy.count(), 1);

    view.addSeries(core);  // duplicate is a no-op
    QCOMPARE(view.seriesCount(), 1);
    QCOMPARE(addedSpy.count(), 1);
}

void TestQmlChartView::removeSeriesRemovesAndEmits()
{
    QmlChartView view;
    QSignalSpy removedSpy(&view, &QmlChartView::seriesRemoved);

    auto* core = new QLineSeries(&view);
    view.addSeries(core);
    QCOMPARE(view.seriesCount(), 1);

    view.removeSeries(core);

    QCOMPARE(view.seriesCount(), 0);
    QVERIFY(view.series().isEmpty());
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.at(0).at(0).value<QAbstractSeries*>(), core);
}

void TestQmlChartView::removeSeriesIgnoresUnknownSeries()
{
    QmlChartView view;
    QSignalSpy removedSpy(&view, &QmlChartView::seriesRemoved);

    auto* tracked = new QLineSeries(&view);
    auto* untracked = new QLineSeries(&view);
    view.addSeries(tracked);

    view.removeSeries(untracked);  // not in the collection

    QCOMPARE(view.seriesCount(), 1);
    QCOMPARE(view.series().at(0), tracked);
    QCOMPARE(removedSpy.count(), 0);
}

void TestQmlChartView::clearSeriesRemovesAllAndEmits()
{
    QmlChartView view;
    QSignalSpy removedSpy(&view, &QmlChartView::seriesRemoved);

    auto* a = new QLineSeries(&view);
    auto* b = new QLineSeries(&view);
    auto* c = new QLineSeries(&view);
    view.addSeries(a);
    view.addSeries(b);
    view.addSeries(c);
    QCOMPARE(view.seriesCount(), 3);

    view.clearSeries();

    QCOMPARE(view.seriesCount(), 0);
    QVERIFY(view.series().isEmpty());
    QCOMPARE(removedSpy.count(), 3);
}

void TestQmlChartView::seriesAtReturnsIndexedOrNull()
{
    QmlChartView view;

    QCOMPARE(view.seriesAt(0), nullptr);  // empty
    QCOMPARE(view.seriesAt(-1), nullptr);

    auto* a = new QLineSeries(&view);
    auto* b = new QLineSeries(&view);
    view.addSeries(a);
    view.addSeries(b);

    QCOMPARE(view.seriesAt(0), a);
    QCOMPARE(view.seriesAt(1), b);
    QCOMPARE(view.seriesAt(2), nullptr);  // out of range
}

void TestQmlChartView::seriesCountReflectsCollection()
{
    QmlChartView view;

    auto* a = new QLineSeries(&view);
    auto* b = new QLineSeries(&view);

    QCOMPARE(view.seriesCount(), 0);
    view.addSeries(a);
    QCOMPARE(view.seriesCount(), 1);
    view.addSeries(b);
    QCOMPARE(view.seriesCount(), 2);
    view.removeSeries(a);
    QCOMPARE(view.seriesCount(), 1);
    view.clearSeries();
    QCOMPARE(view.seriesCount(), 0);
}

void TestQmlChartView::destroyedSeriesIsDroppedFromCollection()
{
    QmlChartView view;
    QSignalSpy removedSpy(&view, &QmlChartView::seriesRemoved);

    // The core series is owned externally (here by `view` as QObject parent,
    // but in real QML usage by the wrapper item). When it is destroyed, the
    // chart must drop it from the collection WITHOUT emitting seriesRemoved
    // (the chart did not initiate the removal — parity with WidgetChartView,
    // whose destroyed-handler also only removes silently).
    auto* core = new QLineSeries(&view);
    view.addSeries(core);
    QCOMPARE(view.seriesCount(), 1);

    delete core;

    QCOMPARE(view.seriesCount(), 0);
    QVERIFY(view.series().isEmpty());
    // The destroyed-handler removes silently (no seriesRemoved emission),
    // matching WidgetChartView::addSeries's destroyed lambda.
    QCOMPARE(removedSpy.count(), 0);
}

void TestQmlChartView::declarativeChildRegistersCoreSeries()
{
    QmlChartView view;
    QSignalSpy addedSpy(&view, &QmlChartView::seriesAdded);

    // Reproduces the declarative QML pattern: a LineSeries added as a child
    // of the ChartView. Setting the parentItem triggers ItemChildAddedChange
    // inside QQuickItem, which QmlChartView::itemChange intercepts.
    auto* wrapper = new QmlLineSeries(&view);  // QObject parent = view
    wrapper->setParentItem(&view);             // QQuickItem child = view

    QCOMPARE(view.seriesCount(), 1);
    QCOMPARE(view.series().at(0), wrapper->coreSeries());
    QCOMPARE(addedSpy.count(), 1);
}

void TestQmlChartView::declarativeLineAndScatterChildrenBothRegistered()
{
    QmlChartView view;
    QSignalSpy addedSpy(&view, &QmlChartView::seriesAdded);

    auto* line = new QmlLineSeries(&view);
    line->setParentItem(&view);

    auto* scatter = new QmlScatterSeries(&view);
    scatter->setParentItem(&view);

    QCOMPARE(view.seriesCount(), 2);
    QCOMPARE(view.seriesAt(0), line->coreSeries());
    QCOMPARE(view.seriesAt(1), scatter->coreSeries());
    QCOMPARE(addedSpy.count(), 2);

    // Core type identities are preserved through the collection.
    QCOMPARE(view.seriesAt(0)->type(), qgraphplot::SeriesType::Line);
    QCOMPARE(view.seriesAt(1)->type(), qgraphplot::SeriesType::Scatter);
}

void TestQmlChartView::clearSeriesOnDestructorIsSafe()
{
    // A chart with tracked series must destroy cleanly (no use-after-free in
    // the destroyed connections). QPointer verifies the chart is gone.
    QPointer<QmlChartView> view(new QmlChartView);

    auto* a = new QLineSeries(view);
    auto* b = new QLineSeries(view);
    view->addSeries(a);
    view->addSeries(b);
    QCOMPARE(view->seriesCount(), 2);

    delete view;
    QVERIFY(view.isNull());
}

QTEST_MAIN(TestQmlChartView)
#include "TestQmlChartView.moc"