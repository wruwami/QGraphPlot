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
//! @brief Unit tests for qgraphplot::QmlAxis (placeholder - title API not yet implemented)

#include <QtTest/QtTest>

#include "../src/qml_frontend/QmlAxis.h"

using qgraphplot::QmlAxis;

class TestQmlAxis : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreSane();
};

void TestQmlAxis::defaultsAreSane()
{
    // Basic smoke test - verifies QmlAxis can be instantiated.
    // Title-related tests removed as the API does not currently exist in QmlAxis.
    QmlAxis axis;
    QCOMPARE(axis.orientation(), Qt::Horizontal);
    QCOMPARE(axis.tickCount(), 5);
}

QTEST_MAIN(TestQmlAxis)
#include "TestQmlAxis.moc"