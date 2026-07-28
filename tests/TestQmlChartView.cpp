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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopedPointer>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlProperty>
#include <QtQuick/QQuickItem>
#include <QtTest/QtTest>

#include "../src/core/model/QStaticSeriesModel.h"
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

QString qmlImportPath()
{
    return QDir::cleanPath(
        QDir(QCoreApplication::applicationDirPath()).filePath("../../src/qml_frontend"));
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

//! Builds a regex that matches the fixed, literal prefix of a qCWarning
//! message. The setters append the rejected value after the prefix via
//! QDebug's stream operator, so exact-string matching would be brittle.
QRegularExpression warningPrefix(const QString& prefix)
{
    return QRegularExpression(QRegularExpression::escape(prefix));
}

//! Finds the first descendant of @p root that exposes a "text" property
//! (i.e. a QtQuick Text item) whose current value equals @p text.
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

    // ── Title property (chart title rendering) ──────────────────
    void titleDefaultIsEmpty();
    void setTitleAcceptsValue();
    void setTitleEmitsSignal();
    void setTitleRedundantWriteDoesNotEmit();

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
    void reparentedLineSeriesMovesBetweenCharts();
    void reparentedScatterSeriesMovesBetweenCharts();
    void clearSeriesOnDestructorIsSafe();

    // ── Axis auto-range (issue #63) ─────────────────────────────
    void autoScaleDefaultsAreOff();
    void autoScalePaddingDefaultIsFivePercent();
    void autoScalePaddingRejectsNegativeAndNonFinite();
    void autoScaleXRecomputesOnEnable();
    void autoScaleYRecomputesOnEnable();
    void autoScaleXFollowsBoundsChanged();
    void autoScaleXRecomputesOnAddSeries();
    void autoScaleRespectsPaddingRatio();
    void autoScaleXHandlesEmptyModel();
    void autoScaleXHandlesSinglePoint();

    // ── Zoom / pan API (issue #64) ───────────────────────────────
    void zoomXEnabledDefaultIsTrue();
    void zoomYEnabledDefaultIsTrue();
    void setZoomXEnabledToggle();
    void setZoomYEnabledToggle();
    void setXRangeAcceptsValidRange();
    void setXRangeRejectsInvalidRange();
    void setXRangeDisablesAutoScaleX();
    void setXRangeEmitsTransformChanged();
    void setYRangeAcceptsValidRange();
    void setYRangeRejectsInvalidRange();
    void setYRangeDisablesAutoScaleY();
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
// Title property (chart title rendering, issue #87)
// ════════════════════════════════════════════════════════════════

void TestQmlChartView::titleDefaultIsEmpty()
{
    QmlChartView view;
    QCOMPARE(view.title(), QString());
}

void TestQmlChartView::setTitleAcceptsValue()
{
    QmlChartView view;
    view.setTitle(QStringLiteral("My Chart"));
    QCOMPARE(view.title(), QStringLiteral("My Chart"));
}

void TestQmlChartView::setTitleEmitsSignal()
{
    QmlChartView view;
    QSignalSpy spy(&view, &QmlChartView::titleChanged);
    view.setTitle(QStringLiteral("Sensor Data"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(view.title(), QStringLiteral("Sensor Data"));
}

void TestQmlChartView::setTitleRedundantWriteDoesNotEmit()
{
    QmlChartView view;
    view.setTitle(QStringLiteral("Fixed Title"));

    QSignalSpy spy(&view, &QmlChartView::titleChanged);
    view.setTitle(QStringLiteral("Fixed Title"));  // same value — no-op
    QCOMPARE(spy.count(), 0);
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
    QQmlEngine engine;
    QString error;
    QScopedPointer<QObject> root(createQmlObject(
        QByteArrayLiteral("import QtQuick\nimport QGraphPlot\nItem { QmlChartView { "
                          "objectName: \"view\"; QmlLineSeries { objectName: \"series\" } } }"),
        engine,
        &error));
    QVERIFY2(root, qPrintable(error));

    auto* view = root->findChild<QmlChartView*>(QStringLiteral("view"));
    auto* wrapper = root->findChild<QmlLineSeries*>(QStringLiteral("series"));
    QVERIFY(view);
    QVERIFY(wrapper);

    QTRY_COMPARE(view->seriesCount(), 1);
    QCOMPARE(view->seriesAt(0), wrapper->coreSeries());
}

void TestQmlChartView::declarativeLineAndScatterChildrenBothRegistered()
{
    QQmlEngine engine;
    QString error;
    QScopedPointer<QObject> root(createQmlObject(
        QByteArrayLiteral("import QtQuick\nimport QGraphPlot\nItem { QmlChartView { "
                          "objectName: \"view\"; QmlLineSeries { objectName: \"line\" } "
                          "QmlScatterSeries { objectName: \"scatter\" } } }"),
        engine,
        &error));
    QVERIFY2(root, qPrintable(error));

    auto* view = root->findChild<QmlChartView*>(QStringLiteral("view"));
    auto* line = root->findChild<QmlLineSeries*>(QStringLiteral("line"));
    auto* scatter = root->findChild<QmlScatterSeries*>(QStringLiteral("scatter"));
    QVERIFY(view);
    QVERIFY(line);
    QVERIFY(scatter);

    QTRY_COMPARE(view->seriesCount(), 2);
    QCOMPARE(view->seriesAt(0), line->coreSeries());
    QCOMPARE(view->seriesAt(1), scatter->coreSeries());

    // Core type identities are preserved through the collection.
    QCOMPARE(view->seriesAt(0)->type(), qgraphplot::SeriesType::Line);
    QCOMPARE(view->seriesAt(1)->type(), qgraphplot::SeriesType::Scatter);
}

void TestQmlChartView::reparentedLineSeriesMovesBetweenCharts()
{
    QQmlEngine engine;
    QString error;
    QScopedPointer<QObject> root(createQmlObject(
        QByteArrayLiteral("import QtQuick\nimport QGraphPlot\nItem { QmlChartView { "
                          "objectName: \"sourceView\"; QmlLineSeries { objectName: \"line\" } } "
                          "QmlChartView { objectName: \"targetView\" } }"),
        engine,
        &error));
    QVERIFY2(root, qPrintable(error));

    auto* sourceView = root->findChild<QmlChartView*>(QStringLiteral("sourceView"));
    auto* targetView = root->findChild<QmlChartView*>(QStringLiteral("targetView"));
    auto* wrapper = root->findChild<QmlLineSeries*>(QStringLiteral("line"));
    QVERIFY(sourceView);
    QVERIFY(targetView);
    QVERIFY(wrapper);

    QSignalSpy sourceRemovedSpy(sourceView, &QmlChartView::seriesRemoved);
    QSignalSpy targetAddedSpy(targetView, &QmlChartView::seriesAdded);
    QTRY_COMPARE(sourceView->seriesCount(), 1);
    QCOMPARE(sourceView->seriesAt(0), wrapper->coreSeries());

    wrapper->setParentItem(targetView);

    QTRY_COMPARE(sourceView->seriesCount(), 0);
    QTRY_COMPARE(targetView->seriesCount(), 1);
    QCOMPARE(targetView->seriesAt(0), wrapper->coreSeries());
    QCOMPARE(sourceRemovedSpy.count(), 1);
    QCOMPARE(targetAddedSpy.count(), 1);
}

void TestQmlChartView::reparentedScatterSeriesMovesBetweenCharts()
{
    QQmlEngine engine;
    QString error;
    QScopedPointer<QObject> root(createQmlObject(
        QByteArrayLiteral(
            "import QtQuick\nimport QGraphPlot\nItem { QmlChartView { "
            "objectName: \"sourceView\"; QmlScatterSeries { objectName: \"scatter\" } } "
            "QmlChartView { objectName: \"targetView\" } }"),
        engine,
        &error));
    QVERIFY2(root, qPrintable(error));

    auto* sourceView = root->findChild<QmlChartView*>(QStringLiteral("sourceView"));
    auto* targetView = root->findChild<QmlChartView*>(QStringLiteral("targetView"));
    auto* wrapper = root->findChild<QmlScatterSeries*>(QStringLiteral("scatter"));
    QVERIFY(sourceView);
    QVERIFY(targetView);
    QVERIFY(wrapper);

    QSignalSpy sourceRemovedSpy(sourceView, &QmlChartView::seriesRemoved);
    QSignalSpy targetAddedSpy(targetView, &QmlChartView::seriesAdded);
    QTRY_COMPARE(sourceView->seriesCount(), 1);
    QCOMPARE(sourceView->seriesAt(0), wrapper->coreSeries());

    wrapper->setParentItem(targetView);

    QTRY_COMPARE(sourceView->seriesCount(), 0);
    QTRY_COMPARE(targetView->seriesCount(), 1);
    QCOMPARE(targetView->seriesAt(0), wrapper->coreSeries());
    QCOMPARE(sourceRemovedSpy.count(), 1);
    QCOMPARE(targetAddedSpy.count(), 1);
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

// ════════════════════════════════════════════════════════════════
// Axis auto-range (issue #63)
//
// QmlChartView and WidgetChartView share the same auto-scale contract:
// autoScaleX/autoScaleY (bool, default false) + autoScalePadding (double,
// default 0.05). The math lives in qgraphplot::QAutoScaler; here we
// verify the QML frontend wires the signals correctly (AI.md §3.1).
// ════════════════════════════════════════════════════════════════

namespace
{
//! Builds a QLineSeries whose backing static model holds @p points, parented
//! to @p owner so it cleans up with the owner (no manual delete needed).
QLineSeries* makeSeriesWith(QObject& owner, const QList<QPointF>& points)
{
    auto* model = new qgraphplot::QStaticSeriesModel(&owner);
    model->setPoints(QSpan<const QPointF>(points.data(), points.size()));
    auto* series = new QLineSeries(&owner);
    series->setModel(model);
    return series;
}
}  // namespace

void TestQmlChartView::autoScaleDefaultsAreOff()
{
    QmlChartView view;
    QCOMPARE(view.autoScaleX(), false);
    QCOMPARE(view.autoScaleY(), false);
}

void TestQmlChartView::autoScalePaddingDefaultIsFivePercent()
{
    QmlChartView view;
    QCOMPARE(view.autoScalePadding(), 0.05);
}

void TestQmlChartView::autoScalePaddingRejectsNegativeAndNonFinite()
{
    QmlChartView view;
    QSignalSpy spy(&view, &QmlChartView::autoScalePaddingChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "QmlChartView::setAutoScalePadding: rejected negative or non-finite ratio:"));

    for (const double invalid : {-0.01,
                                 -1.0,
                                 std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity(),
                                 -std::numeric_limits<double>::infinity()}) {
        QTest::ignoreMessage(QtWarningMsg, pattern);
        view.setAutoScalePadding(invalid);
    }
    QCOMPARE(view.autoScalePadding(), 0.05);  // unchanged
    QCOMPARE(spy.count(), 0);
}

void TestQmlChartView::autoScaleXRecomputesOnEnable()
{
    QmlChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0.0, -1.0), QPointF(10.0, 1.0)});
    view.addSeries(s);

    QSignalSpy xMinSpy(&view, &QmlChartView::xMinChanged);
    QSignalSpy xMaxSpy(&view, &QmlChartView::xMaxChanged);
    QSignalSpy yMinSpy(&view, &QmlChartView::yMinChanged);

    view.setAutoScaleX(true);

    // Bounds x∈[0, 10], padding 0.05 → ±0.5 → [-0.5, 10.5].
    QCOMPARE(view.xMin(), -0.5);
    QCOMPARE(view.xMax(), 10.5);
    QVERIFY(xMinSpy.count() >= 1);
    QVERIFY(xMaxSpy.count() >= 1);
    // autoScaleX only — Y must be untouched.
    QCOMPARE(view.yMin(), 0.0);
    QCOMPARE(yMinSpy.count(), 0);
}

void TestQmlChartView::autoScaleYRecomputesOnEnable()
{
    QmlChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0.0, -1.0), QPointF(10.0, 1.0)});
    view.addSeries(s);

    QSignalSpy yMinSpy(&view, &QmlChartView::yMinChanged);
    QSignalSpy yMaxSpy(&view, &QmlChartView::yMaxChanged);
    QSignalSpy xMinSpy(&view, &QmlChartView::xMinChanged);

    view.setAutoScaleY(true);

    // Bounds y∈[-1, 1], padding 0.05 → ±0.1 → [-1.1, 1.1].
    QCOMPARE(view.yMin(), -1.1);
    QCOMPARE(view.yMax(), 1.1);
    QVERIFY(yMinSpy.count() >= 1);
    QVERIFY(yMaxSpy.count() >= 1);
    QCOMPARE(view.xMin(), 0.0);  // X untouched
    QCOMPARE(xMinSpy.count(), 0);
}

void TestQmlChartView::autoScaleXFollowsBoundsChanged()
{
    QmlChartView view;
    QObject owner;
    QList<QPointF> points{QPointF(0.0, 0.0), QPointF(2.0, 1.0)};
    auto* model = new qgraphplot::QStaticSeriesModel(&owner);
    model->setPoints(QSpan<const QPointF>(points.data(), points.size()));
    auto* s = new QLineSeries(&owner);
    s->setModel(model);
    view.addSeries(s);
    view.setAutoScaleX(true);

    // Initial auto range: x∈[0, 2], padding 0.05 → ±0.1 → [-0.1, 2.1].
    QCOMPARE(view.xMin(), -0.1);
    QCOMPARE(view.xMax(), 2.1);

    // Append a point far to the right — model emits boundsChanged, and the
    // view recomputes the auto-range synchronously off that direct
    // same-thread connection (signal-driven, no per-frame polling), so the
    // new range is already in effect by the time appendBatch() returns.
    // Exactly one emission also guards against duplicate boundsChanged
    // connections that would re-enter the model (regression test for the
    // QStaticSeriesModel mutex deadlock fixed alongside this test).
    QSignalSpy xMaxSpy(&view, &QmlChartView::xMaxChanged);
    QList<QPointF> extra{QPointF(20.0, 1.0)};
    model->appendBatch(QSpan<const QPointF>(extra.data(), extra.size()));
    QCOMPARE(xMaxSpy.count(), 1);
    QCOMPARE(view.xMax(), 21.0);  // padding 0.05 of span 20 = 1.0
}

void TestQmlChartView::autoScaleXRecomputesOnAddSeries()
{
    QmlChartView view;
    QObject owner;
    view.setAutoScaleX(true);
    // No series yet → fallback bounds QRectF(0,0,1,1), padded to [-0.05, 1.05].
    QCOMPARE(view.xMin(), -0.05);
    QCOMPARE(view.xMax(), 1.05);

    QSignalSpy xMinSpy(&view, &QmlChartView::xMinChanged);
    auto* s = makeSeriesWith(owner, {QPointF(0.0, 0.0), QPointF(4.0, 1.0)});
    view.addSeries(s);
    // Adding a series triggers an immediate recompute.
    QCOMPARE(view.xMin(), -0.2);  // padding 0.05 of 4 = 0.2
    QCOMPARE(view.xMax(), 4.2);
    QVERIFY(xMinSpy.count() >= 1);
}

void TestQmlChartView::autoScaleRespectsPaddingRatio()
{
    QmlChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0.0, 0.0), QPointF(10.0, 1.0)});
    view.addSeries(s);
    view.setAutoScalePadding(0.1);
    view.setAutoScaleX(true);
    // padding 0.1 of span 10 = 1.0 on each side.
    QCOMPARE(view.xMin(), -1.0);
    QCOMPARE(view.xMax(), 11.0);
}

void TestQmlChartView::autoScaleXHandlesEmptyModel()
{
    QmlChartView view;
    QObject owner;
    // Series with an empty model → contributes nothing; auto-range must not
    // crash and must use QAutoScaler's padded fallback bounds.
    auto* emptyModel = new qgraphplot::QStaticSeriesModel(&owner);
    auto* s = new QLineSeries(&owner);
    s->setModel(emptyModel);
    view.addSeries(s);

    QSignalSpy xMinSpy(&view, &QmlChartView::xMinChanged);
    view.setAutoScaleX(true);
    // Fallback bounds QRectF(0,0,1,1): padded by 0.05*1=0.05 → [-0.05, 1.05].
    QCOMPARE(view.xMin(), -0.05);
    QCOMPARE(view.xMax(), 1.05);
    QVERIFY(xMinSpy.count() >= 1);
}

void TestQmlChartView::autoScaleXHandlesSinglePoint()
{
    QmlChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(5.0, 7.0)});  // zero-width bounds
    view.addSeries(s);
    view.setAutoScaleX(true);
    // Single x coordinate → expanded by unit on each side.
    QCOMPARE(view.xMin(), 4.0);
    QCOMPARE(view.xMax(), 6.0);
}

// ════════════════════════════════════════════════════════════════
// Zoom / pan API (issue #64)
//
// setXRange / setYRange: atomic dual-setter that validates the pair and
// disables autoScale for the axis. zoomXEnabled / zoomYEnabled: flags
// that control whether wheel/drag gestures affect the axis (API parity
// with WidgetChartView, AI.md §3.1).
// ════════════════════════════════════════════════════════════════

void TestQmlChartView::zoomXEnabledDefaultIsTrue()
{
    QmlChartView view;
    QCOMPARE(view.zoomXEnabled(), true);
}

void TestQmlChartView::zoomYEnabledDefaultIsTrue()
{
    QmlChartView view;
    QCOMPARE(view.zoomYEnabled(), true);
}

void TestQmlChartView::setZoomXEnabledToggle()
{
    QmlChartView view;
    QSignalSpy spy(&view, &QmlChartView::zoomXEnabledChanged);
    view.setZoomXEnabled(false);
    QCOMPARE(view.zoomXEnabled(), false);
    QCOMPARE(spy.count(), 1);
    view.setZoomXEnabled(false);  // no-op
    QCOMPARE(spy.count(), 1);
    view.setZoomXEnabled(true);
    QCOMPARE(view.zoomXEnabled(), true);
    QCOMPARE(spy.count(), 2);
}

void TestQmlChartView::setZoomYEnabledToggle()
{
    QmlChartView view;
    QSignalSpy spy(&view, &QmlChartView::zoomYEnabledChanged);
    view.setZoomYEnabled(false);
    QCOMPARE(view.zoomYEnabled(), false);
    QCOMPARE(spy.count(), 1);
    view.setZoomYEnabled(false);  // no-op
    QCOMPARE(spy.count(), 1);
}

void TestQmlChartView::setXRangeAcceptsValidRange()
{
    QmlChartView view;
    QSignalSpy xMinSpy(&view, &QmlChartView::xMinChanged);
    QSignalSpy xMaxSpy(&view, &QmlChartView::xMaxChanged);

    view.setXRange(-5.0, 15.0);

    QCOMPARE(view.xMin(), -5.0);
    QCOMPARE(view.xMax(), 15.0);
    QCOMPARE(xMinSpy.count(), 1);
    QCOMPARE(xMaxSpy.count(), 1);
}

void TestQmlChartView::setXRangeRejectsInvalidRange()
{
    QmlChartView view;
    QSignalSpy spy(&view, &QmlChartView::xMinChanged);

    view.setXRange(5.0, -5.0);  // inverted
    view.setXRange(3.0, 3.0);   // equal
    view.setXRange(std::numeric_limits<double>::quiet_NaN(), 10.0);
    view.setXRange(0.0, std::numeric_limits<double>::infinity());

    QCOMPARE(view.xMin(), 0.0);  // defaults unchanged
    QCOMPARE(view.xMax(), 10.0);
    QCOMPARE(spy.count(), 0);
}

void TestQmlChartView::setXRangeDisablesAutoScaleX()
{
    QmlChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0.0, 0.0), QPointF(4.0, 1.0)});
    view.addSeries(s);
    view.setAutoScaleX(true);
    QCOMPARE(view.autoScaleX(), true);

    QSignalSpy autoSpy(&view, &QmlChartView::autoScaleXChanged);
    view.setXRange(1.0, 3.0);

    QCOMPARE(view.autoScaleX(), false);
    QCOMPARE(autoSpy.count(), 1);
    QCOMPARE(view.xMin(), 1.0);
    QCOMPARE(view.xMax(), 3.0);
}

void TestQmlChartView::setXRangeEmitsTransformChanged()
{
    QmlChartView view;
    QSignalSpy spy(&view, &QmlChartView::transformChanged);
    view.setXRange(2.0, 8.0);
    QVERIFY(spy.count() >= 1);
}

void TestQmlChartView::setYRangeAcceptsValidRange()
{
    QmlChartView view;
    QSignalSpy yMinSpy(&view, &QmlChartView::yMinChanged);
    QSignalSpy yMaxSpy(&view, &QmlChartView::yMaxChanged);

    view.setYRange(-2.0, 2.0);

    QCOMPARE(view.yMin(), -2.0);
    QCOMPARE(view.yMax(), 2.0);
    QCOMPARE(yMinSpy.count(), 1);
    QCOMPARE(yMaxSpy.count(), 1);
}

void TestQmlChartView::setYRangeRejectsInvalidRange()
{
    QmlChartView view;
    QSignalSpy spy(&view, &QmlChartView::yMinChanged);

    view.setYRange(5.0, -5.0);  // inverted
    view.setYRange(2.0, 2.0);   // equal

    QCOMPARE(view.yMin(), 0.0);  // defaults unchanged
    QCOMPARE(view.yMax(), 10.0);
    QCOMPARE(spy.count(), 0);
}

void TestQmlChartView::setYRangeDisablesAutoScaleY()
{
    QmlChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0.0, 0.0), QPointF(1.0, 4.0)});
    view.addSeries(s);
    view.setAutoScaleY(true);
    QCOMPARE(view.autoScaleY(), true);

    QSignalSpy autoSpy(&view, &QmlChartView::autoScaleYChanged);
    view.setYRange(-1.0, 5.0);

    QCOMPARE(view.autoScaleY(), false);
    QCOMPARE(autoSpy.count(), 1);
    QCOMPARE(view.yMin(), -1.0);
    QCOMPARE(view.yMax(), 5.0);
}

QTEST_MAIN(TestQmlChartView)
#include "TestQmlChartView.moc"
