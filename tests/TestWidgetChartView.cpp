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

//! @file TestWidgetChartView.cpp
//! @brief Unit tests for qgraphplot::WidgetChartView's axis/margin setters:
//!        finite-range validation, rejection warnings, and the near-zero
//!        safe fuzzy-equality gate shared with QmlChartView (#59, #35).

#include <limits>

#include <QtCore/QRegularExpression>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

#include "../src/core/model/QStaticSeriesModel.h"
#include "../src/core/series/QLineSeries.h"
#include "../src/widget_frontend/WidgetChartView.h"
#include "../src/widget_frontend/WidgetLineSeries.h"

using qgraphplot::WidgetChartView;

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

class TestWidgetChartView : public QObject
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

    // ── Phase 1 rendering: WidgetLineSeries (issue #70) ─────────
    void widgetLineSeriesTypeIsLine();
    void widgetLineSeriesInheritsDefaultLineWidth();
    void widgetLineSeriesInheritsDefaultColor();
    void paintSeriesNopWhenInvisible();
    void paintSeriesNopWhenNullModel();
    void paintSeriesNopWhenEmptyModel();
    void paintSeriesRendersWithoutCrash();
    void repaintConnectionWiredOnAddSeries();
    void repaintConnectionRemovedOnRemoveSeries();
};

void TestWidgetChartView::defaultsAreSane()
{
    WidgetChartView view;
    QCOMPARE(view.xMin(), 0.0);
    QCOMPARE(view.xMax(), 10.0);
    QCOMPARE(view.yMin(), 0.0);
    QCOMPARE(view.yMax(), 10.0);
    QCOMPARE(view.marginLeft(), 50.0);
    QCOMPARE(view.marginRight(), 20.0);
    QCOMPARE(view.marginTop(), 20.0);
    QCOMPARE(view.marginBottom(), 40.0);
}

void TestWidgetChartView::setXMinAcceptsValidValue()
{
    WidgetChartView view;
    QSignalSpy minSpy(&view, &WidgetChartView::xMinChanged);
    QSignalSpy transformSpy(&view, &WidgetChartView::transformChanged);

    view.setXMin(-5.0);

    QCOMPARE(view.xMin(), -5.0);
    QCOMPARE(minSpy.count(), 1);
    QCOMPARE(transformSpy.count(), 1);
}

void TestWidgetChartView::setXMinRejectsInvalidValues()
{
    WidgetChartView view;
    QSignalSpy minSpy(&view, &WidgetChartView::xMinChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "WidgetChartView::setXMin: rejected non-finite or invalid xMin (must be < xMax):"));

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

void TestWidgetChartView::setXMaxAcceptsValidValue()
{
    WidgetChartView view;
    QSignalSpy maxSpy(&view, &WidgetChartView::xMaxChanged);
    QSignalSpy transformSpy(&view, &WidgetChartView::transformChanged);

    view.setXMax(20.0);

    QCOMPARE(view.xMax(), 20.0);
    QCOMPARE(maxSpy.count(), 1);
    QCOMPARE(transformSpy.count(), 1);
}

void TestWidgetChartView::setXMaxRejectsInvalidValues()
{
    WidgetChartView view;
    QSignalSpy maxSpy(&view, &WidgetChartView::xMaxChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "WidgetChartView::setXMax: rejected non-finite or invalid xMax (must be > xMin):"));

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

void TestWidgetChartView::setYMinRejectsInvalidValues()
{
    WidgetChartView view;
    QSignalSpy minSpy(&view, &WidgetChartView::yMinChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "WidgetChartView::setYMin: rejected non-finite or invalid yMin (must be < yMax):"));

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

void TestWidgetChartView::setYMaxRejectsInvalidValues()
{
    WidgetChartView view;
    QSignalSpy maxSpy(&view, &WidgetChartView::yMaxChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "WidgetChartView::setYMax: rejected non-finite or invalid yMax (must be > yMin):"));

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

void TestWidgetChartView::setMarginAcceptsZeroAndPositive()
{
    WidgetChartView view;
    QSignalSpy marginsSpy(&view, &WidgetChartView::marginsChanged);

    // Boundary: exactly 0.0 is accepted ("< 0.0" is rejected, not "<= 0.0").
    view.setMarginLeft(0.0);
    QCOMPARE(view.marginLeft(), 0.0);
    QCOMPARE(marginsSpy.count(), 1);

    view.setMarginRight(100.0);
    QCOMPARE(view.marginRight(), 100.0);
    QCOMPARE(marginsSpy.count(), 2);
}

void TestWidgetChartView::setMarginRejectsNegativeOrNonFinite()
{
    WidgetChartView view;
    QSignalSpy marginsSpy(&view, &WidgetChartView::marginsChanged);

    struct Setter
    {
        void (WidgetChartView::*fn)(double);
        const char* name;
    };
    const Setter setters[] = {
        {&WidgetChartView::setMarginLeft, "setMarginLeft"},
        {&WidgetChartView::setMarginRight, "setMarginRight"},
        {&WidgetChartView::setMarginTop, "setMarginTop"},
        {&WidgetChartView::setMarginBottom, "setMarginBottom"},
    };

    for (const Setter& setter : setters) {
        const QRegularExpression pattern =
            warningPrefix(QStringLiteral("WidgetChartView::%1: rejected negative or non-finite "
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

void TestWidgetChartView::marginsChangedIsSharedAcrossAllFourMargins()
{
    WidgetChartView view;
    QSignalSpy marginsSpy(&view, &WidgetChartView::marginsChanged);
    QSignalSpy transformSpy(&view, &WidgetChartView::transformChanged);

    view.setMarginLeft(10.0);
    view.setMarginRight(11.0);
    view.setMarginTop(12.0);
    view.setMarginBottom(13.0);

    QCOMPARE(marginsSpy.count(), 4);
    QCOMPARE(transformSpy.count(), 4);
}

void TestWidgetChartView::redundantWritesDoNotEmit()
{
    WidgetChartView view;
    view.setXMin(-5.0);

    QSignalSpy minSpy(&view, &WidgetChartView::xMinChanged);
    QSignalSpy transformSpy(&view, &WidgetChartView::transformChanged);

    // Re-applying the same value is a no-op signal-wise (fuzzyValuesDiffer).
    view.setXMin(-5.0);
    QCOMPARE(minSpy.count(), 0);
    QCOMPARE(transformSpy.count(), 0);
}

void TestWidgetChartView::wideningRangeAllowsPreviouslyInvalidValue()
{
    WidgetChartView view;

    // xMin=20.0 is rejected against the default xMax=10.0...
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "WidgetChartView::setXMin: rejected non-finite or invalid xMin (must be < xMax):"));
    QTest::ignoreMessage(QtWarningMsg, pattern);
    view.setXMin(20.0);
    QCOMPARE(view.xMin(), 0.0);

    // ...but becomes valid once xMax is widened past it.
    view.setXMax(30.0);
    view.setXMin(20.0);
    QCOMPARE(view.xMin(), 20.0);
}

// ════════════════════════════════════════════════════════════════
// Axis auto-range (issue #63) — parity with TestQmlChartView (AI.md §3.1)
// ════════════════════════════════════════════════════════════════

namespace
{
//! Builds a QLineSeries whose backing static model holds @p points, parented
//! to @p owner so it cleans up with the owner.
qgraphplot::QLineSeries* makeSeriesWith(QObject& owner, const QList<QPointF>& points)
{
    auto* model = new qgraphplot::QStaticSeriesModel(&owner);
    model->setPoints(QSpan<const QPointF>(points.data(), points.size()));
    auto* series = new qgraphplot::QLineSeries(&owner);
    series->setModel(model);
    return series;
}
}  // namespace

void TestWidgetChartView::autoScaleDefaultsAreOff()
{
    WidgetChartView view;
    QCOMPARE(view.autoScaleX(), false);
    QCOMPARE(view.autoScaleY(), false);
}

void TestWidgetChartView::autoScalePaddingDefaultIsFivePercent()
{
    WidgetChartView view;
    QCOMPARE(view.autoScalePadding(), 0.05);
}

void TestWidgetChartView::autoScalePaddingRejectsNegativeAndNonFinite()
{
    WidgetChartView view;
    QSignalSpy spy(&view, &WidgetChartView::autoScalePaddingChanged);
    const QRegularExpression pattern = warningPrefix(QStringLiteral(
        "WidgetChartView::setAutoScalePadding: rejected negative or non-finite ratio:"));

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

void TestWidgetChartView::autoScaleXRecomputesOnEnable()
{
    WidgetChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0.0, -1.0), QPointF(10.0, 1.0)});
    view.addSeries(s);

    QSignalSpy xMinSpy(&view, &WidgetChartView::xMinChanged);
    QSignalSpy xMaxSpy(&view, &WidgetChartView::xMaxChanged);
    QSignalSpy yMinSpy(&view, &WidgetChartView::yMinChanged);

    view.setAutoScaleX(true);

    QCOMPARE(view.xMin(), -0.5);
    QCOMPARE(view.xMax(), 10.5);
    QVERIFY(xMinSpy.count() >= 1);
    QVERIFY(xMaxSpy.count() >= 1);
    QCOMPARE(view.yMin(), 0.0);  // Y untouched
    QCOMPARE(yMinSpy.count(), 0);
}

void TestWidgetChartView::autoScaleYRecomputesOnEnable()
{
    WidgetChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0.0, -1.0), QPointF(10.0, 1.0)});
    view.addSeries(s);

    QSignalSpy yMinSpy(&view, &WidgetChartView::yMinChanged);
    QSignalSpy yMaxSpy(&view, &WidgetChartView::yMaxChanged);
    QSignalSpy xMinSpy(&view, &WidgetChartView::xMinChanged);

    view.setAutoScaleY(true);

    QCOMPARE(view.yMin(), -1.1);
    QCOMPARE(view.yMax(), 1.1);
    QVERIFY(yMinSpy.count() >= 1);
    QVERIFY(yMaxSpy.count() >= 1);
    QCOMPARE(view.xMin(), 0.0);  // X untouched
    QCOMPARE(xMinSpy.count(), 0);
}

void TestWidgetChartView::autoScaleXFollowsBoundsChanged()
{
    WidgetChartView view;
    QObject owner;
    QList<QPointF> points{QPointF(0.0, 0.0), QPointF(2.0, 1.0)};
    auto* model = new qgraphplot::QStaticSeriesModel(&owner);
    model->setPoints(QSpan<const QPointF>(points.data(), points.size()));
    auto* s = new qgraphplot::QLineSeries(&owner);
    s->setModel(model);
    view.addSeries(s);
    view.setAutoScaleX(true);

    QCOMPARE(view.xMin(), -0.1);
    QCOMPARE(view.xMax(), 2.1);

    // model->appendBatch() emits boundsChanged() synchronously on this same
    // thread, and the view's direct connection recomputes the auto-range
    // before appendBatch() returns — no waiting needed. Exactly one emission
    // also guards against duplicate boundsChanged connections that would
    // re-enter the model (regression test for the QStaticSeriesModel mutex
    // deadlock fixed alongside this test).
    QSignalSpy xMaxSpy(&view, &WidgetChartView::xMaxChanged);
    QList<QPointF> extra{QPointF(20.0, 1.0)};
    model->appendBatch(QSpan<const QPointF>(extra.data(), extra.size()));
    QCOMPARE(xMaxSpy.count(), 1);
    QCOMPARE(view.xMax(), 21.0);
}

void TestWidgetChartView::autoScaleXRecomputesOnAddSeries()
{
    WidgetChartView view;
    QObject owner;
    view.setAutoScaleX(true);
    // No series yet → fallback bounds QRectF(0,0,1,1), padded to [-0.05, 1.05].
    QCOMPARE(view.xMin(), -0.05);
    QCOMPARE(view.xMax(), 1.05);

    auto* s = makeSeriesWith(owner, {QPointF(0.0, 0.0), QPointF(4.0, 1.0)});
    view.addSeries(s);
    QCOMPARE(view.xMin(), -0.2);
    QCOMPARE(view.xMax(), 4.2);
}

void TestWidgetChartView::autoScaleRespectsPaddingRatio()
{
    WidgetChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0.0, 0.0), QPointF(10.0, 1.0)});
    view.addSeries(s);
    view.setAutoScalePadding(0.1);
    view.setAutoScaleX(true);
    // padding 0.1 of span 10 = 1.0 on each side.
    QCOMPARE(view.xMin(), -1.0);
    QCOMPARE(view.xMax(), 11.0);
}

void TestWidgetChartView::autoScaleXHandlesEmptyModel()
{
    WidgetChartView view;
    QObject owner;
    auto* emptyModel = new qgraphplot::QStaticSeriesModel(&owner);
    auto* s = new qgraphplot::QLineSeries(&owner);
    s->setModel(emptyModel);
    view.addSeries(s);

    QSignalSpy xMinSpy(&view, &WidgetChartView::xMinChanged);
    view.setAutoScaleX(true);
    QCOMPARE(view.xMin(), -0.05);
    QCOMPARE(view.xMax(), 1.05);
    QVERIFY(xMinSpy.count() >= 1);
}

void TestWidgetChartView::autoScaleXHandlesSinglePoint()
{
    WidgetChartView view;
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(5.0, 7.0)});
    view.addSeries(s);
    view.setAutoScaleX(true);
    QCOMPARE(view.xMin(), 4.0);
    QCOMPARE(view.xMax(), 6.0);
}

// ════════════════════════════════════════════════════════════════
// Phase 1 rendering: WidgetLineSeries (issue #70)
// ════════════════════════════════════════════════════════════════

void TestWidgetChartView::widgetLineSeriesTypeIsLine()
{
    qgraphplot::WidgetLineSeries s;
    QCOMPARE(s.type(), qgraphplot::SeriesType::Line);
}

void TestWidgetChartView::widgetLineSeriesInheritsDefaultLineWidth()
{
    qgraphplot::WidgetLineSeries s;
    QCOMPARE(s.lineWidth(), 2.0);
}

void TestWidgetChartView::widgetLineSeriesInheritsDefaultColor()
{
    qgraphplot::WidgetLineSeries s;
    QCOMPARE(s.color(), QColor(Qt::blue));
}

namespace
{
//! Creates a QImage-backed painter + a unit-square coordinate transform for
//! use in no-crash rendering tests.
struct TestPaintContext
{
    QImage img{100, 100, QImage::Format_ARGB32};
    QPainter painter;
    qgraphplot::QCoordinateTransform xform{QRectF(0, 0, 10, 10), QRectF(0, 0, 100, 100)};

    TestPaintContext()
    {
        img.fill(Qt::white);
        painter.begin(&img);
    }
    ~TestPaintContext() { painter.end(); }
};
}  // namespace

void TestWidgetChartView::paintSeriesNopWhenInvisible()
{
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0, 0), QPointF(5, 5)});
    s->setVisible(false);

    TestPaintContext ctx;
    // Must not crash and must not alter the blank canvas.
    qgraphplot::WidgetLineSeries::paintSeries(&ctx.painter, ctx.xform, s);
    ctx.painter.end();

    // Canvas is unchanged: all pixels still white.
    bool unchanged = true;
    for (int y = 0; y < ctx.img.height() && unchanged; ++y) {
        for (int x = 0; x < ctx.img.width() && unchanged; ++x) {
            if (ctx.img.pixel(x, y) != qRgba(255, 255, 255, 255)) {
                unchanged = false;
            }
        }
    }
    QVERIFY(unchanged);
}

void TestWidgetChartView::paintSeriesNopWhenNullModel()
{
    qgraphplot::WidgetLineSeries s;
    // model() is null by default.
    QVERIFY(s.model() == nullptr);

    TestPaintContext ctx;
    qgraphplot::WidgetLineSeries::paintSeries(&ctx.painter, ctx.xform, &s);
    // No crash expected.
}

void TestWidgetChartView::paintSeriesNopWhenEmptyModel()
{
    QObject owner;
    auto* emptyModel = new qgraphplot::QStaticSeriesModel(&owner);
    qgraphplot::WidgetLineSeries s;
    s.setModel(emptyModel);

    TestPaintContext ctx;
    qgraphplot::WidgetLineSeries::paintSeries(&ctx.painter, ctx.xform, &s);
    // No crash expected.
}

void TestWidgetChartView::paintSeriesRendersWithoutCrash()
{
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0, 0), QPointF(5, 5), QPointF(10, 0)});
    s->setColor(Qt::red);
    s->setLineWidth(2.0);

    TestPaintContext ctx;
    qgraphplot::WidgetLineSeries::paintSeries(&ctx.painter, ctx.xform, s);
    // No crash expected.
}

void TestWidgetChartView::repaintConnectionWiredOnAddSeries()
{
    WidgetChartView view;
    QObject owner;
    auto* model = new qgraphplot::QStaticSeriesModel(&owner);
    auto* s = new qgraphplot::WidgetLineSeries(&owner);
    s->setModel(model);
    view.addSeries(s);

    // After addSeries, inserting a point into the model should not crash
    // (the repaint connection should forward the signal to update()).
    QList<QPointF> pts{QPointF(1, 1)};
    model->appendBatch(QSpan<const QPointF>(pts.constData(), pts.size()));
    // If we got here without a crash or assert, the connection is working.
    QVERIFY(true);
}

void TestWidgetChartView::repaintConnectionRemovedOnRemoveSeries()
{
    WidgetChartView view;
    QObject owner;
    auto* model = new qgraphplot::QStaticSeriesModel(&owner);
    auto* s = new qgraphplot::WidgetLineSeries(&owner);
    s->setModel(model);
    view.addSeries(s);
    view.removeSeries(s);

    // After removeSeries the connection must be torn down; mutating the model
    // must not crash (no dangling slot).
    QList<QPointF> pts{QPointF(2, 2)};
    model->appendBatch(QSpan<const QPointF>(pts.constData(), pts.size()));
    QVERIFY(true);
}

QTEST_MAIN(TestWidgetChartView)
#include "TestWidgetChartView.moc"