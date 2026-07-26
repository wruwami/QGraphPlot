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
#include <QtTest/QtTest>

#include "../src/widget_frontend/WidgetChartView.h"

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

    for (const double invalid : {std::numeric_limits<double>::quiet_NaN(), 10.0, 12.0}) {
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

    for (const double invalid : {std::numeric_limits<double>::quiet_NaN(), 0.0, -1.0}) {
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

QTEST_MAIN(TestWidgetChartView)
#include "TestWidgetChartView.moc"