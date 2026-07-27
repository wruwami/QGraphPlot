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

//! @file TestQmlAxis.cpp
//! @brief Unit tests for qgraphplot::QmlAxis's `title` property, added
//!        alongside the QmlChartView title (chart title rendering support).

#include <QtTest/QtTest>

#include "../src/qml_frontend/QmlAxis.h"

using qgraphplot::QmlAxis;

class TestQmlAxis : public QObject
{
    Q_OBJECT

private slots:
    void titleDefaultsToEmpty();
    void setTitleUpdatesValueAndEmits();
    void setTitleSameValueDoesNotEmit();
    void setTitleCanBeClearedBackToEmpty();
    void setTitleDoesNotAffectOtherProperties();
};

void TestQmlAxis::titleDefaultsToEmpty()
{
    QmlAxis axis;
    QCOMPARE(axis.title(), QString());
}

void TestQmlAxis::setTitleUpdatesValueAndEmits()
{
    QmlAxis axis;
    QSignalSpy titleSpy(&axis, &QmlAxis::titleChanged);

    axis.setTitle(QStringLiteral("Time (s)"));

    QCOMPARE(axis.title(), QStringLiteral("Time (s)"));
    QCOMPARE(titleSpy.count(), 1);
}

void TestQmlAxis::setTitleSameValueDoesNotEmit()
{
    QmlAxis axis;
    axis.setTitle(QStringLiteral("Amplitude"));

    QSignalSpy titleSpy(&axis, &QmlAxis::titleChanged);
    axis.setTitle(QStringLiteral("Amplitude"));  // redundant write

    QCOMPARE(axis.title(), QStringLiteral("Amplitude"));
    QCOMPARE(titleSpy.count(), 0);
}

void TestQmlAxis::setTitleCanBeClearedBackToEmpty()
{
    QmlAxis axis;
    axis.setTitle(QStringLiteral("X"));

    QSignalSpy titleSpy(&axis, &QmlAxis::titleChanged);
    axis.setTitle(QString());

    QCOMPARE(axis.title(), QString());
    QCOMPARE(titleSpy.count(), 1);
}

void TestQmlAxis::setTitleDoesNotAffectOtherProperties()
{
    // The title is purely cosmetic metadata; it must not perturb the axis's
    // other properties or force a tick recomputation (unlike orientation,
    // tickCount, and showGrid, which do — see QmlAxis::setTitle vs.
    // setShowGrid in QmlAxis.cpp).
    QmlAxis axis;
    QSignalSpy orientationSpy(&axis, &QmlAxis::orientationChanged);
    QSignalSpy tickCountSpy(&axis, &QmlAxis::tickCountChanged);
    QSignalSpy ticksSpy(&axis, &QmlAxis::ticksChanged);

    axis.setTitle(QStringLiteral("Value"));

    QCOMPARE(orientationSpy.count(), 0);
    QCOMPARE(tickCountSpy.count(), 0);
    QCOMPARE(ticksSpy.count(), 0);
    QCOMPARE(axis.orientation(), Qt::Horizontal);
    QCOMPARE(axis.tickCount(), 5);
}

QTEST_MAIN(TestQmlAxis)
#include "TestQmlAxis.moc"