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
//! @brief Unit tests for qgraphplot::QmlAxis

#include <QtTest/QtTest>

#include "../src/qml_frontend/QmlAxis.h"

using qgraphplot::QmlAxis;

class TestQmlAxis : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreSane();

    void titleDefaultIsEmpty();
    void setTitleAcceptsValue();
    void setTitleEmitsSignal();
    void setTitleRedundantWriteDoesNotEmit();
};

void TestQmlAxis::defaultsAreSane()
{
    QmlAxis axis;
    QCOMPARE(axis.orientation(), Qt::Horizontal);
    QCOMPARE(axis.tickCount(), 5);
}

void TestQmlAxis::titleDefaultIsEmpty()
{
    QmlAxis axis;
    QCOMPARE(axis.title(), QString());
}

void TestQmlAxis::setTitleAcceptsValue()
{
    QmlAxis axis;
    axis.setTitle(QStringLiteral("X Axis"));
    QCOMPARE(axis.title(), QStringLiteral("X Axis"));
}

void TestQmlAxis::setTitleEmitsSignal()
{
    QmlAxis axis;
    QSignalSpy spy(&axis, &QmlAxis::titleChanged);
    axis.setTitle(QStringLiteral("Time (s)"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(axis.title(), QStringLiteral("Time (s)"));
}

void TestQmlAxis::setTitleRedundantWriteDoesNotEmit()
{
    QmlAxis axis;
    axis.setTitle(QStringLiteral("Voltage"));

    QSignalSpy spy(&axis, &QmlAxis::titleChanged);
    axis.setTitle(QStringLiteral("Voltage"));  // same value — no-op
    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(TestQmlAxis)
#include "TestQmlAxis.moc"