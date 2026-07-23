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

#include "../src/core/transform/QCoordinateTransform.h"

using namespace qgraphplot;

class TestCoordinate : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void testLinearTransform();
    void testLinearInverseTransform();
    void testLogTransform();
    void testLogInverseTransform();
    void testLogFallbackOnInvalidBounds();
    void testDegenerateBoundsSafety();
};

void TestCoordinate::initTestCase() {
}

void TestCoordinate::testLinearTransform() {
    // Data: X from 0.0 to 10.0, Y from 50.0 to 150.0
    // Screen: X from 100.0 to 500.0 (width 400), Y from 50.0 to 250.0 (height 200)
    QRectF dataBounds(0.0, 50.0, 10.0, 100.0);
    QRectF pixelRect(100.0, 50.0, 400.0, 200.0);

    QCoordinateTransform trans(dataBounds, pixelRect, false, false);

    // Test X mapping
    // X=0 -> 100, X=5 -> 300, X=10 -> 500
    // Test Y mapping (inverted)
    // Y=50 -> 250 (bottom), Y=100 -> 150 (middle), Y=150 -> 50 (top)
    QPointF p1 = trans.toPixel(QPointF(0.0, 50.0));
    QCOMPARE(p1.x(), 100.0);
    QCOMPARE(p1.y(), 250.0);

    QPointF p2 = trans.toPixel(QPointF(5.0, 100.0));
    QCOMPARE(p2.x(), 300.0);
    QCOMPARE(p2.y(), 150.0);

    QPointF p3 = trans.toPixel(QPointF(10.0, 150.0));
    QCOMPARE(p3.x(), 500.0);
    QCOMPARE(p3.y(), 50.0);
}

void TestCoordinate::testLinearInverseTransform() {
    QRectF dataBounds(0.0, 50.0, 10.0, 100.0);
    QRectF pixelRect(100.0, 50.0, 400.0, 200.0);

    QCoordinateTransform trans(dataBounds, pixelRect, false, false);

    QPointF d1 = trans.toData(QPointF(100.0, 250.0));
    QCOMPARE(d1.x(), 0.0);
    QCOMPARE(d1.y(), 50.0);

    QPointF d2 = trans.toData(QPointF(300.0, 150.0));
    QCOMPARE(d2.x(), 5.0);
    QCOMPARE(d2.y(), 100.0);

    QPointF d3 = trans.toData(QPointF(500.0, 50.0));
    QCOMPARE(d3.x(), 10.0);
    QCOMPARE(d3.y(), 150.0);
}

void TestCoordinate::testLogTransform() {
    // Log scale. Bounds must be positive.
    // Data: X from 10.0 to 1000.0 (log: 1 to 3), Y from 1.0 to 100.0 (log: 0 to 2)
    // Screen: X from 0.0 to 200.0, Y from 0.0 to 100.0
    QRectF dataBounds(10.0, 1.0, 990.0, 99.0);
    QRectF pixelRect(0.0, 0.0, 200.0, 100.0);

    QCoordinateTransform trans(dataBounds, pixelRect, true, true);
    QVERIFY(trans.xLog());
    QVERIFY(trans.yLog());

    // X: log10(10)=1 -> pixel 0.0; log10(100)=2 -> pixel 100.0; log10(1000)=3 -> pixel 200.0
    // Y: log10(1)=0 -> pixel 100.0; log10(10)=1 -> pixel 50.0; log10(100)=2 -> pixel 0.0
    QPointF p1 = trans.toPixel(QPointF(10.0, 1.0));
    QCOMPARE(p1.x(), 0.0);
    QCOMPARE(p1.y(), 100.0);

    QPointF p2 = trans.toPixel(QPointF(100.0, 10.0));
    QCOMPARE(p2.x(), 100.0);
    QCOMPARE(p2.y(), 50.0);

    QPointF p3 = trans.toPixel(QPointF(1000.0, 100.0));
    QCOMPARE(p3.x(), 200.0);
    QCOMPARE(p3.y(), 0.0);
}

void TestCoordinate::testLogInverseTransform() {
    QRectF dataBounds(10.0, 1.0, 990.0, 99.0);
    QRectF pixelRect(0.0, 0.0, 200.0, 100.0);

    QCoordinateTransform trans(dataBounds, pixelRect, true, true);

    QPointF d1 = trans.toData(QPointF(0.0, 100.0));
    QCOMPARE(d1.x(), 10.0);
    QCOMPARE(d1.y(), 1.0);

    QPointF d2 = trans.toData(QPointF(100.0, 50.0));
    QVERIFY(std::abs(d2.x() - 100.0) < 1e-9);
    QVERIFY(std::abs(d2.y() - 10.0) < 1e-9);

    QPointF d3 = trans.toData(QPointF(200.0, 0.0));
    QVERIFY(std::abs(d3.x() - 1000.0) < 1e-9);
    QVERIFY(std::abs(d3.y() - 100.0) < 1e-9);
}

void TestCoordinate::testLogFallbackOnInvalidBounds() {
    // Log scale enabled but bounds contain non-positive values.
    // Must print warnings and fall back to linear scale (AI.md §3.3).
    QRectF dataBounds(-10.0, 0.0, 20.0, 10.0);
    QRectF pixelRect(0.0, 0.0, 200.0, 100.0);

    QTest::ignoreMessage(QtWarningMsg, "QCoordinateTransform: X log scale requires positive bounds. Falling back to linear.");
    QTest::ignoreMessage(QtWarningMsg, "QCoordinateTransform: Y log scale requires positive bounds. Falling back to linear.");

    QCoordinateTransform trans(dataBounds, pixelRect, true, true);
    QVERIFY(!trans.xLog());
    QVERIFY(!trans.yLog());
}

void TestCoordinate::testDegenerateBoundsSafety() {
    // Degenerate range where width/height is 0.
    // Must map everything safely to center to avoid division by zero (NaN/Inf).
    QRectF dataBounds(5.0, 5.0, 0.0, 0.0);
    QRectF pixelRect(100.0, 50.0, 400.0, 200.0);

    QCoordinateTransform trans(dataBounds, pixelRect, false, false);
    QPointF p = trans.toPixel(QPointF(5.0, 5.0));

    QCOMPARE(p.x(), 300.0);
    QCOMPARE(p.y(), 150.0);
}

QTEST_GUILESS_MAIN(TestCoordinate)
#include "TestCoordinate.moc"
