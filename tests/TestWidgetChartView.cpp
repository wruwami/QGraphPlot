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
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QWheelEvent>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

#include "../src/core/model/QStaticSeriesModel.h"
#include "../src/core/series/QLineSeries.h"
#include "../src/core/series/QScatterSeries.h"
#include "../src/core/theme/QGraphPlotTheme.h"
#include "../src/widget_frontend/WidgetChartView.h"
#include "../src/widget_frontend/WidgetLineSeries.h"
#include "../src/widget_frontend/WidgetScatterSeries.h"

using qgraphplot::QGraphPlotTheme;
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

    // ── WidgetScatterSeries (issue #101) ────────────────────────
    void widgetScatterSeriesTypeIsScatter();
    void widgetScatterSeriesDefaultMarkerSize();
    void widgetScatterSeriesDefaultMarkerShape();
    void paintScatterSeriesNopWhenInvisible();
    void paintScatterSeriesNopWhenNullModel();
    void paintScatterSeriesNopWhenEmptyModel();
    void paintScatterSeriesCircleRendersWithoutCrash();
    void paintScatterSeriesSquareRendersWithoutCrash();
    void paintScatterSeriesOpacityFoldedIntoAlpha();
    void paintEventWithScatterSeriesNoCrash();

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
    void repaintConnectionRewiredOnModelSwap();

    // ── Zoom / pan: range setters and zoom toggles (issue #64) ─────────────
    void zoomXEnabledDefaultIsTrue();
    void setZoomXEnabledToggles();
    void setZoomXEnabledRedundantWriteNoSignal();
    void setZoomYEnabledToggles();
    void setXRangeAtomicUpdate();
    void setXRangeInvalidRejected();
    void setXRangeDisablesAutoScale();
    void setYRangeAtomicUpdate();
    void setYRangeInvalidRejected();
    void setYRangeDisablesAutoScale();

    // ── Theme ────────────────────────────────────────────────────────────────
    void setThemeEmitsSignal();
    void setThemeRedundantWriteNoSignal();
    void setThemeNullClearsTheme();

    // ── clearSeries / resizeEvent ────────────────────────────────────────────
    void clearSeriesEmitsRemovedForEachSeries();
    void resizeEmitsTransformChanged();

    // ── Rendering (issue #70) ────────────────────────────────────────────────
    void paintEventNoCrash();
    void paintEventWithThemeNoCrash();
    void paintEventWithSeriesNoCrash();
    void paintEventWithRubberbandNoCrash();
    void paintSeriesWithDashPattern();

    // ── Mouse / wheel interaction (issue #64) ────────────────────────────────
    void wheelEventZoomsInside();
    void wheelEventOutsidePlotPassesThrough();
    void mousePressLeftButtonStartsPan();
    void mousePressRightButtonStartsRubberband();
    void mousePressOutsidePassesThrough();
    void mouseMoveWhilePanningShiftsRange();
    void mouseMoveWhileRubberbanding();
    void mouseReleaseEndsPan();
    void mouseReleaseRubberbandSmallBandNoZoom();
    void mouseReleaseRubberbandLargeBandAppliesZoom();
    void mouseDoubleClickResetsViewport();
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

    QImage img;
    {
        TestPaintContext ctx;
        // Must not crash and must not alter the blank canvas.
        qgraphplot::WidgetLineSeries::paintSeries(&ctx.painter, ctx.xform, s);
        img = ctx.img;
    }  // ctx destructor calls painter.end() here

    // Canvas is unchanged: all pixels still white.
    bool unchanged = true;
    for (int y = 0; y < img.height() && unchanged; ++y) {
        for (int x = 0; x < img.width() && unchanged; ++x) {
            if (img.pixel(x, y) != qRgba(255, 255, 255, 255)) {
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

    // Model must fire pointsInserted exactly once; the view's repaint slot runs
    // synchronously on the live series — verifies the connection is wired and
    // the slot executes without crashing while view still owns the series.
    QSignalSpy spy(model, &qgraphplot::QAbstractSeriesModel::pointsInserted);
    QList<QPointF> pts{QPointF(1, 1)};
    model->appendBatch(QSpan<const QPointF>(pts.constData(), pts.size()));
    QCOMPARE(spy.count(), 1);
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
    delete s;  // removeSeries() detached ownership from both view and owner.
}

void TestWidgetChartView::repaintConnectionRewiredOnModelSwap()
{
    WidgetChartView view;
    QObject owner;
    auto* model1 = new qgraphplot::QStaticSeriesModel(&owner);
    auto* model2 = new qgraphplot::QStaticSeriesModel(&owner);
    auto* s = new qgraphplot::QLineSeries(&owner);
    s->setModel(model1);
    view.addSeries(s);

    s->setModel(model2);  // triggers the modelChanged handler in connectRepaintModel

    // New model must be subscribed: pointsInserted fires without crash.
    QSignalSpy spy(model2, &qgraphplot::QAbstractSeriesModel::pointsInserted);
    QList<QPointF> pts{QPointF(3, 3)};
    model2->appendBatch(QSpan<const QPointF>(pts.constData(), pts.size()));
    QCOMPARE(spy.count(), 1);

    // Old model mutations must not crash (stale connections torn down).
    QList<QPointF> pts1{QPointF(9, 9)};
    model1->appendBatch(QSpan<const QPointF>(pts1.constData(), pts1.size()));
}

// ════════════════════════════════════════════════════════════════
// Zoom / pan: range setters and zoom toggles (issue #64)
// ════════════════════════════════════════════════════════════════

void TestWidgetChartView::zoomXEnabledDefaultIsTrue()
{
    WidgetChartView view;
    QCOMPARE(view.zoomXEnabled(), true);
    QCOMPARE(view.zoomYEnabled(), true);
}

void TestWidgetChartView::setZoomXEnabledToggles()
{
    WidgetChartView view;
    QSignalSpy spy(&view, &WidgetChartView::zoomXEnabledChanged);
    view.setZoomXEnabled(false);
    QCOMPARE(view.zoomXEnabled(), false);
    QCOMPARE(spy.count(), 1);
}

void TestWidgetChartView::setZoomXEnabledRedundantWriteNoSignal()
{
    WidgetChartView view;
    QSignalSpy spy(&view, &WidgetChartView::zoomXEnabledChanged);
    view.setZoomXEnabled(true);  // same as default
    QCOMPARE(spy.count(), 0);
}

void TestWidgetChartView::setZoomYEnabledToggles()
{
    WidgetChartView view;
    QSignalSpy spy(&view, &WidgetChartView::zoomYEnabledChanged);
    view.setZoomYEnabled(false);
    QCOMPARE(view.zoomYEnabled(), false);
    QCOMPARE(spy.count(), 1);
}

void TestWidgetChartView::setXRangeAtomicUpdate()
{
    WidgetChartView view;
    QSignalSpy xMinSpy(&view, &WidgetChartView::xMinChanged);
    QSignalSpy xMaxSpy(&view, &WidgetChartView::xMaxChanged);
    QSignalSpy transformSpy(&view, &WidgetChartView::transformChanged);

    view.setXRange(-5.0, 25.0);

    QCOMPARE(view.xMin(), -5.0);
    QCOMPARE(view.xMax(), 25.0);
    QVERIFY(xMinSpy.count() >= 1);
    QVERIFY(xMaxSpy.count() >= 1);
    QVERIFY(transformSpy.count() >= 1);
}

void TestWidgetChartView::setXRangeInvalidRejected()
{
    WidgetChartView view;
    QSignalSpy spy(&view, &WidgetChartView::xMinChanged);

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    view.setXRange(nan, 5.0);  // nan min
    view.setXRange(0.0, inf);  // inf max
    view.setXRange(5.0, 5.0);  // min == max
    view.setXRange(6.0, 5.0);  // min > max

    QCOMPARE(view.xMin(), 0.0);  // unchanged
    QCOMPARE(view.xMax(), 10.0);
    QCOMPARE(spy.count(), 0);
}

void TestWidgetChartView::setXRangeDisablesAutoScale()
{
    WidgetChartView view;
    view.setAutoScaleX(true);
    QCOMPARE(view.autoScaleX(), true);

    QSignalSpy autoSpy(&view, &WidgetChartView::autoScaleXChanged);
    view.setXRange(1.0, 9.0);

    QCOMPARE(view.autoScaleX(), false);
    QCOMPARE(autoSpy.count(), 1);
}

void TestWidgetChartView::setYRangeAtomicUpdate()
{
    WidgetChartView view;
    QSignalSpy yMinSpy(&view, &WidgetChartView::yMinChanged);
    QSignalSpy yMaxSpy(&view, &WidgetChartView::yMaxChanged);

    view.setYRange(-3.0, 15.0);

    QCOMPARE(view.yMin(), -3.0);
    QCOMPARE(view.yMax(), 15.0);
    QVERIFY(yMinSpy.count() >= 1);
    QVERIFY(yMaxSpy.count() >= 1);
}

void TestWidgetChartView::setYRangeInvalidRejected()
{
    WidgetChartView view;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    view.setYRange(nan, 5.0);  // nan min
    view.setYRange(0.0, inf);  // inf max
    view.setYRange(5.0, 5.0);  // min == max
    view.setYRange(6.0, 5.0);  // min > max

    QCOMPARE(view.yMin(), 0.0);  // unchanged
    QCOMPARE(view.yMax(), 10.0);
}

void TestWidgetChartView::setYRangeDisablesAutoScale()
{
    WidgetChartView view;
    view.setAutoScaleY(true);
    QSignalSpy spy(&view, &WidgetChartView::autoScaleYChanged);
    view.setYRange(-1.0, 11.0);
    QCOMPARE(view.autoScaleY(), false);
    QCOMPARE(spy.count(), 1);
}

// ════════════════════════════════════════════════════════════════
// Theme
// ════════════════════════════════════════════════════════════════

void TestWidgetChartView::setThemeEmitsSignal()
{
    WidgetChartView view;
    QGraphPlotTheme theme;
    QSignalSpy spy(&view, &WidgetChartView::themeChanged);

    view.setTheme(&theme);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(view.theme(), &theme);
}

void TestWidgetChartView::setThemeRedundantWriteNoSignal()
{
    WidgetChartView view;
    QGraphPlotTheme theme;
    view.setTheme(&theme);
    QSignalSpy spy(&view, &WidgetChartView::themeChanged);
    view.setTheme(&theme);  // same theme — no-op
    QCOMPARE(spy.count(), 0);
}

void TestWidgetChartView::setThemeNullClearsTheme()
{
    WidgetChartView view;
    QGraphPlotTheme theme;
    view.setTheme(&theme);
    QSignalSpy spy(&view, &WidgetChartView::themeChanged);
    view.setTheme(nullptr);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(view.theme(), nullptr);
}

// ════════════════════════════════════════════════════════════════
// clearSeries / resizeEvent
// ════════════════════════════════════════════════════════════════

void TestWidgetChartView::clearSeriesEmitsRemovedForEachSeries()
{
    WidgetChartView view;
    QObject owner;
    auto* s1 = makeSeriesWith(owner, {QPointF(0, 0)});
    auto* s2 = makeSeriesWith(owner, {QPointF(1, 1)});
    view.addSeries(s1);
    view.addSeries(s2);

    QSignalSpy spy(&view, &WidgetChartView::seriesRemoved);
    view.clearSeries();

    QCOMPARE(view.series().size(), 0);
    QCOMPARE(spy.count(), 2);
    delete s1;  // clearSeries() detached both from view and owner
    delete s2;
}

void TestWidgetChartView::resizeEmitsTransformChanged()
{
    WidgetChartView view;
    view.resize(400, 300);
    QSignalSpy spy(&view, &WidgetChartView::transformChanged);

    QResizeEvent ev(QSize(500, 400), QSize(400, 300));
    QApplication::sendEvent(&view, &ev);

    QCOMPARE(spy.count(), 1);
}

// ════════════════════════════════════════════════════════════════
// Rendering (issue #70)
// ════════════════════════════════════════════════════════════════

void TestWidgetChartView::paintEventNoCrash()
{
    WidgetChartView view;
    view.resize(400, 300);
    QImage img(400, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    view.render(&img);
}

void TestWidgetChartView::paintEventWithThemeNoCrash()
{
    WidgetChartView view;
    view.resize(400, 300);
    QGraphPlotTheme theme;
    theme.setFontFamily(QStringLiteral("Arial"));
    view.setTheme(&theme);
    QImage img(400, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    view.render(&img);
}

void TestWidgetChartView::paintEventWithSeriesNoCrash()
{
    WidgetChartView view;
    view.resize(400, 300);
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0, 0), QPointF(5, 5), QPointF(10, 0)});
    view.addSeries(s);
    QImage img(400, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    view.render(&img);
}

void TestWidgetChartView::paintEventWithRubberbandNoCrash()
{
    WidgetChartView view;
    view.resize(400, 300);

    QMouseEvent pressEv(QEvent::MouseButtonPress,
                        QPointF(100.0, 50.0),
                        QPointF(100.0, 50.0),
                        Qt::RightButton,
                        Qt::RightButton,
                        Qt::NoModifier);
    QApplication::sendEvent(&view, &pressEv);
    QMouseEvent moveEv(QEvent::MouseMove,
                       QPointF(250.0, 200.0),
                       QPointF(250.0, 200.0),
                       Qt::NoButton,
                       Qt::RightButton,
                       Qt::NoModifier);
    QApplication::sendEvent(&view, &moveEv);

    QImage img(400, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    view.render(&img);  // covers the m_rubberbanding branch in paintEvent
}

void TestWidgetChartView::paintSeriesWithDashPattern()
{
    QObject owner;
    auto* s = makeSeriesWith(owner, {QPointF(0, 0), QPointF(5, 5), QPointF(10, 0)});
    s->setDashPattern({4.0, 2.0});

    TestPaintContext ctx;
    qgraphplot::WidgetLineSeries::paintSeries(&ctx.painter, ctx.xform, s);
    // No crash expected; exercises the dash-pattern branch in paintSeries.
}

// ════════════════════════════════════════════════════════════════
// WidgetScatterSeries (issue #101)
// ════════════════════════════════════════════════════════════════

namespace
{
//! Builds a QScatterSeries whose backing static model holds @p points,
//! parented to @p owner so it cleans up with the owner.
qgraphplot::QScatterSeries* makeScatterWith(QObject& owner, const QList<QPointF>& points)
{
    auto* model = new qgraphplot::QStaticSeriesModel(&owner);
    model->setPoints(QSpan<const QPointF>(points.data(), points.size()));
    auto* series = new qgraphplot::QScatterSeries(&owner);
    series->setModel(model);
    return series;
}
}  // namespace

void TestWidgetChartView::widgetScatterSeriesTypeIsScatter()
{
    qgraphplot::WidgetScatterSeries s;
    QCOMPARE(s.type(), qgraphplot::SeriesType::Scatter);
}

void TestWidgetChartView::widgetScatterSeriesDefaultMarkerSize()
{
    qgraphplot::WidgetScatterSeries s;
    QCOMPARE(s.markerSize(), 8.0);
}

void TestWidgetChartView::widgetScatterSeriesDefaultMarkerShape()
{
    qgraphplot::WidgetScatterSeries s;
    QCOMPARE(s.markerShape(), qgraphplot::MarkerShape::Circle);
}

void TestWidgetChartView::paintScatterSeriesNopWhenInvisible()
{
    QObject owner;
    auto* s = makeScatterWith(owner, {QPointF(0, 0), QPointF(5, 5)});
    s->setVisible(false);

    QImage img;
    {
        TestPaintContext ctx;
        qgraphplot::WidgetScatterSeries::paintSeries(&ctx.painter, ctx.xform, s);
        img = ctx.img;
    }

    bool unchanged = true;
    for (int y = 0; y < img.height() && unchanged; ++y) {
        for (int x = 0; x < img.width() && unchanged; ++x) {
            if (img.pixel(x, y) != qRgba(255, 255, 255, 255)) {
                unchanged = false;
            }
        }
    }
    QVERIFY(unchanged);
}

void TestWidgetChartView::paintScatterSeriesNopWhenNullModel()
{
    qgraphplot::WidgetScatterSeries s;
    QVERIFY(s.model() == nullptr);

    TestPaintContext ctx;
    qgraphplot::WidgetScatterSeries::paintSeries(&ctx.painter, ctx.xform, &s);
    // No crash expected.
}

void TestWidgetChartView::paintScatterSeriesNopWhenEmptyModel()
{
    QObject owner;
    auto* emptyModel = new qgraphplot::QStaticSeriesModel(&owner);
    qgraphplot::WidgetScatterSeries s;
    s.setModel(emptyModel);

    TestPaintContext ctx;
    qgraphplot::WidgetScatterSeries::paintSeries(&ctx.painter, ctx.xform, &s);
    // No crash expected.
}

void TestWidgetChartView::paintScatterSeriesCircleRendersWithoutCrash()
{
    QObject owner;
    auto* s = makeScatterWith(owner, {QPointF(2, 2), QPointF(5, 5), QPointF(8, 3)});
    s->setColor(Qt::blue);
    s->setMarkerShape(qgraphplot::MarkerShape::Circle);
    s->setMarkerSize(10.0);

    TestPaintContext ctx;
    qgraphplot::WidgetScatterSeries::paintSeries(&ctx.painter, ctx.xform, s);
    // No crash expected; exercises Circle branch.
}

void TestWidgetChartView::paintScatterSeriesSquareRendersWithoutCrash()
{
    QObject owner;
    auto* s = makeScatterWith(owner, {QPointF(1, 1), QPointF(5, 5), QPointF(9, 9)});
    s->setColor(Qt::red);
    s->setMarkerShape(qgraphplot::MarkerShape::Square);
    s->setMarkerSize(8.0);

    TestPaintContext ctx;
    qgraphplot::WidgetScatterSeries::paintSeries(&ctx.painter, ctx.xform, s);
    // No crash expected; exercises Square branch.
}

void TestWidgetChartView::paintScatterSeriesOpacityFoldedIntoAlpha()
{
    // Render with opacity=0.0 → markers are transparent → canvas unchanged.
    QObject owner;
    auto* s = makeScatterWith(owner, {QPointF(5, 5)});
    s->setColor(Qt::red);
    s->setOpacity(0.0);

    QImage img;
    {
        TestPaintContext ctx;
        qgraphplot::WidgetScatterSeries::paintSeries(&ctx.painter, ctx.xform, s);
        img = ctx.img;
    }

    bool unchanged = true;
    for (int y = 0; y < img.height() && unchanged; ++y) {
        for (int x = 0; x < img.width() && unchanged; ++x) {
            if (img.pixel(x, y) != qRgba(255, 255, 255, 255)) {
                unchanged = false;
            }
        }
    }
    QVERIFY(unchanged);
}

void TestWidgetChartView::paintEventWithScatterSeriesNoCrash()
{
    WidgetChartView view;
    view.resize(400, 300);
    QObject owner;
    auto* s = makeScatterWith(owner, {QPointF(2, 2), QPointF(5, 5), QPointF(8, 3)});
    view.addSeries(s);
    QImage img(400, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    view.render(&img);
    // Verifies WidgetChartView::paintAllSeries() dispatches SeriesType::Scatter
    // without crashing (regression: before issue #101 this was a silent no-op).
}

// ════════════════════════════════════════════════════════════════
// Mouse / wheel interaction (issue #64)
// ════════════════════════════════════════════════════════════════

void TestWidgetChartView::wheelEventZoomsInside()
{
    WidgetChartView view;
    view.resize(400, 300);
    // Default: plot area = QRectF(50, 20, 330, 240); center ≈ (215, 140).
    const QPointF center(215.0, 140.0);
    QWheelEvent ev(center,
                   center,
                   QPoint(),
                   QPoint(0, 120),
                   Qt::NoButton,
                   Qt::NoModifier,
                   Qt::ScrollUpdate,
                   false);
    QApplication::sendEvent(&view, &ev);

    // One notch up (angleDelta.y=120) → factor = 0.9 → span shrinks by 10%.
    QVERIFY(view.xMax() - view.xMin() < 10.0);
}

void TestWidgetChartView::wheelEventOutsidePlotPassesThrough()
{
    WidgetChartView view;
    view.resize(400, 300);
    // x=10 is inside the left margin (marginLeft=50), so outside the plot.
    const QPointF outside(10.0, 140.0);
    QWheelEvent ev(outside,
                   outside,
                   QPoint(),
                   QPoint(0, 120),
                   Qt::NoButton,
                   Qt::NoModifier,
                   Qt::ScrollUpdate,
                   false);
    QApplication::sendEvent(&view, &ev);

    QCOMPARE(view.xMin(), 0.0);
    QCOMPARE(view.xMax(), 10.0);
}

void TestWidgetChartView::mousePressLeftButtonStartsPan()
{
    WidgetChartView view;
    view.resize(400, 300);
    QMouseEvent ev(QEvent::MouseButtonPress,
                   QPointF(215.0, 140.0),
                   QPointF(215.0, 140.0),
                   Qt::LeftButton,
                   Qt::LeftButton,
                   Qt::NoModifier);
    QApplication::sendEvent(&view, &ev);
    QCOMPARE(view.cursor().shape(), Qt::ClosedHandCursor);
}

void TestWidgetChartView::mousePressRightButtonStartsRubberband()
{
    WidgetChartView view;
    view.resize(400, 300);
    QMouseEvent ev(QEvent::MouseButtonPress,
                   QPointF(215.0, 140.0),
                   QPointF(215.0, 140.0),
                   Qt::RightButton,
                   Qt::RightButton,
                   Qt::NoModifier);
    QApplication::sendEvent(&view, &ev);
    // Rubberband started; range unchanged until release.
    QCOMPARE(view.xMin(), 0.0);
    QCOMPARE(view.xMax(), 10.0);
}

void TestWidgetChartView::mousePressOutsidePassesThrough()
{
    WidgetChartView view;
    view.resize(400, 300);
    // x=10 is in the left margin — outside the plot area.
    QMouseEvent ev(QEvent::MouseButtonPress,
                   QPointF(10.0, 140.0),
                   QPointF(10.0, 140.0),
                   Qt::LeftButton,
                   Qt::LeftButton,
                   Qt::NoModifier);
    QApplication::sendEvent(&view, &ev);
    QVERIFY(view.cursor().shape() != Qt::ClosedHandCursor);
}

void TestWidgetChartView::mouseMoveWhilePanningShiftsRange()
{
    WidgetChartView view;
    view.resize(400, 300);

    QMouseEvent pressEv(QEvent::MouseButtonPress,
                        QPointF(215.0, 140.0),
                        QPointF(215.0, 140.0),
                        Qt::LeftButton,
                        Qt::LeftButton,
                        Qt::NoModifier);
    QApplication::sendEvent(&view, &pressEv);

    const double prevXMin = view.xMin();

    QMouseEvent moveEv(QEvent::MouseMove,
                       QPointF(265.0, 140.0),
                       QPointF(265.0, 140.0),
                       Qt::NoButton,
                       Qt::LeftButton,
                       Qt::NoModifier);
    QApplication::sendEvent(&view, &moveEv);

    // Moving right pans the view left: xMin decreases.
    QVERIFY(view.xMin() < prevXMin);
}

void TestWidgetChartView::mouseMoveWhileRubberbanding()
{
    WidgetChartView view;
    view.resize(400, 300);

    QMouseEvent pressEv(QEvent::MouseButtonPress,
                        QPointF(100.0, 50.0),
                        QPointF(100.0, 50.0),
                        Qt::RightButton,
                        Qt::RightButton,
                        Qt::NoModifier);
    QApplication::sendEvent(&view, &pressEv);

    QMouseEvent moveEv(QEvent::MouseMove,
                       QPointF(250.0, 200.0),
                       QPointF(250.0, 200.0),
                       Qt::NoButton,
                       Qt::RightButton,
                       Qt::NoModifier);
    QApplication::sendEvent(&view, &moveEv);

    // Range is not yet changed — zoom only applies on button release.
    QCOMPARE(view.xMin(), 0.0);
    QCOMPARE(view.xMax(), 10.0);
}

void TestWidgetChartView::mouseReleaseEndsPan()
{
    WidgetChartView view;
    view.resize(400, 300);

    QMouseEvent pressEv(QEvent::MouseButtonPress,
                        QPointF(215.0, 140.0),
                        QPointF(215.0, 140.0),
                        Qt::LeftButton,
                        Qt::LeftButton,
                        Qt::NoModifier);
    QApplication::sendEvent(&view, &pressEv);
    QCOMPARE(view.cursor().shape(), Qt::ClosedHandCursor);

    QMouseEvent releaseEv(QEvent::MouseButtonRelease,
                          QPointF(215.0, 140.0),
                          QPointF(215.0, 140.0),
                          Qt::LeftButton,
                          Qt::NoButton,
                          Qt::NoModifier);
    QApplication::sendEvent(&view, &releaseEv);

    QCOMPARE(view.cursor().shape(), Qt::ArrowCursor);
}

void TestWidgetChartView::mouseReleaseRubberbandSmallBandNoZoom()
{
    WidgetChartView view;
    view.resize(400, 300);

    QMouseEvent pressEv(QEvent::MouseButtonPress,
                        QPointF(200.0, 140.0),
                        QPointF(200.0, 140.0),
                        Qt::RightButton,
                        Qt::RightButton,
                        Qt::NoModifier);
    QApplication::sendEvent(&view, &pressEv);

    QMouseEvent moveEv(QEvent::MouseMove,
                       QPointF(202.0, 142.0),
                       QPointF(202.0, 142.0),
                       Qt::NoButton,
                       Qt::RightButton,
                       Qt::NoModifier);
    QApplication::sendEvent(&view, &moveEv);

    QMouseEvent releaseEv(QEvent::MouseButtonRelease,
                          QPointF(202.0, 142.0),
                          QPointF(202.0, 142.0),
                          Qt::RightButton,
                          Qt::NoButton,
                          Qt::NoModifier);
    QApplication::sendEvent(&view, &releaseEv);

    // Band was only 2×2 pixels — too small to apply zoom.
    QCOMPARE(view.xMin(), 0.0);
    QCOMPARE(view.xMax(), 10.0);
}

void TestWidgetChartView::mouseReleaseRubberbandLargeBandAppliesZoom()
{
    WidgetChartView view;
    view.resize(400, 300);

    QMouseEvent pressEv(QEvent::MouseButtonPress,
                        QPointF(100.0, 50.0),
                        QPointF(100.0, 50.0),
                        Qt::RightButton,
                        Qt::RightButton,
                        Qt::NoModifier);
    QApplication::sendEvent(&view, &pressEv);

    QMouseEvent moveEv(QEvent::MouseMove,
                       QPointF(300.0, 200.0),
                       QPointF(300.0, 200.0),
                       Qt::NoButton,
                       Qt::RightButton,
                       Qt::NoModifier);
    QApplication::sendEvent(&view, &moveEv);

    QMouseEvent releaseEv(QEvent::MouseButtonRelease,
                          QPointF(300.0, 200.0),
                          QPointF(300.0, 200.0),
                          Qt::RightButton,
                          Qt::NoButton,
                          Qt::NoModifier);
    QApplication::sendEvent(&view, &releaseEv);

    // 200×150-pixel band is large enough to apply zoom; view zoomed in.
    QVERIFY(view.xMin() > 0.0);
    QVERIFY(view.xMax() < 10.0);
}

void TestWidgetChartView::mouseDoubleClickResetsViewport()
{
    WidgetChartView view;
    view.resize(400, 300);

    const double initXMin = view.xMin();
    const double initXMax = view.xMax();

    // Wheel zoom triggers saveViewportIfNeeded and changes the range.
    const QPointF center(215.0, 140.0);
    QWheelEvent wheelEv(center,
                        center,
                        QPoint(),
                        QPoint(0, 120),
                        Qt::NoButton,
                        Qt::NoModifier,
                        Qt::ScrollUpdate,
                        false);
    QApplication::sendEvent(&view, &wheelEv);
    QVERIFY(view.xMax() - view.xMin() < 10.0);

    // Double-click restores the pre-zoom viewport.
    QMouseEvent dblClickEv(QEvent::MouseButtonDblClick,
                           center,
                           center,
                           Qt::LeftButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QApplication::sendEvent(&view, &dblClickEv);

    QCOMPARE(view.xMin(), initXMin);
    QCOMPARE(view.xMax(), initXMax);
}

QTEST_MAIN(TestWidgetChartView)
#include "TestWidgetChartView.moc"
