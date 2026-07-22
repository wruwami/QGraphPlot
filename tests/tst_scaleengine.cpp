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

#include <QtTest/QtTest>

#include "../src/core/transform/qscaleengine.h"

using namespace qgraphplot;

class TestScaleEngine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void testLinearTicksSimple();
    void testLinearTicksFloating();
    void testLogTicksWideSpan();
    void testLogTicksNarrowSpan();
    void testLogTicksFallbackOnInvalidBounds();
    void testDegenerateRangeTicks();
    void testLabelFormatting();
};

void TestScaleEngine::initTestCase() {
}

void TestScaleEngine::testLinearTicksSimple() {
    // Range [0.0, 10.0], 5 ticks target.
    // Heckbert should select step = 2.0 or 2.5
    // Ticks generated: 0, 2, 4, 6, 8, 10
    auto ticks = QScaleEngine::calculateTicks(0.0, 10.0, 5, false);

    QVERIFY(ticks.size() >= 4);
    QCOMPARE(ticks.front().position, 0.0);
    QCOMPARE(ticks.back().position, 10.0);

    // Verify labels
    QCOMPARE(ticks.front().label, QString("0"));
    QCOMPARE(ticks.back().label, QString("10"));
}

void TestScaleEngine::testLinearTicksFloating() {
    // Range [1.2, 8.7], 4 ticks target.
    auto ticks = QScaleEngine::calculateTicks(1.2, 8.7, 4, false);

    QVERIFY(!ticks.empty());
    for (const auto& tick : ticks) {
        QVERIFY(tick.position >= 1.2 && tick.position <= 8.7);
    }
}

void TestScaleEngine::testLogTicksWideSpan() {
    // Range [10.0, 1000.0] (log logMin=1, logMax=3), target 3.
    // Wide span should generate 10^1 (10), 10^2 (100), 10^3 (1000)
    auto ticks = QScaleEngine::calculateTicks(10.0, 1000.0, 3, true);

    QCOMPARE(ticks.size(), size_t(3));
    QCOMPARE(ticks[0].position, 10.0);
    QCOMPARE(ticks[0].label, QString("10"));
    QCOMPARE(ticks[1].position, 100.0);
    QCOMPARE(ticks[1].label, QString("100"));
    QCOMPARE(ticks[2].position, 1000.0);
    QCOMPARE(ticks[2].label, QString("1000"));
}

void TestScaleEngine::testLogTicksNarrowSpan() {
    // Range [10.0, 50.0] (log logMin=1, logMax=1.69), target 4.
    // Narrow span should generate intermediate nice ticks in log space.
    auto ticks = QScaleEngine::calculateTicks(10.0, 50.0, 4, true);

    QVERIFY(ticks.size() >= 2);
    for (const auto& tick : ticks) {
        QVERIFY(tick.position >= 10.0 && tick.position <= 50.0);
    }
}

void TestScaleEngine::testLogTicksFallbackOnInvalidBounds() {
    // Log scale requested but range contains negative numbers.
    // Warnings should be printed, and falls back to linear ticks.
    QTest::ignoreMessage(QtWarningMsg, "QScaleEngine: Log scale requires strictly positive range. Falling back to linear.");
    auto ticks = QScaleEngine::calculateTicks(-10.0, 10.0, 5, true);

    QVERIFY(!ticks.empty());
    // Fallback: ticks are linear. First tick should be <= -10.0 (or whatever Heckbert selects).
    // Let's just check that it generated ticks and they cover the range.
    QVERIFY(ticks.front().position <= -10.0);
    QVERIFY(ticks.back().position >= 10.0);
}

void TestScaleEngine::testDegenerateRangeTicks() {
    // Range [5.0, 5.0]
    auto ticks = QScaleEngine::calculateTicks(5.0, 5.0, 5, false);

    QCOMPARE(ticks.size(), size_t(1));
    QCOMPARE(ticks[0].position, 5.0);
    QCOMPARE(ticks[0].label, QString("5"));
}

void TestScaleEngine::testLabelFormatting() {
    // Verify decimal place calculations
    QCOMPARE(QScaleEngine::formatLabel(1.2345, 0.1), QString("1.2"));
    QCOMPARE(QScaleEngine::formatLabel(1.2345, 0.01), QString("1.23"));
    QCOMPARE(QScaleEngine::formatLabel(1.2346, 0.001), QString("1.235")); // Rounding check
    QCOMPARE(QScaleEngine::formatLabel(100.0, 10.0), QString("100"));
    QCOMPARE(QScaleEngine::formatLabel(-0.0000000000000001, 1.0), QString("0")); // Sign normalization check
}

QTEST_GUILESS_MAIN(TestScaleEngine)
#include "tst_scaleengine.moc"
