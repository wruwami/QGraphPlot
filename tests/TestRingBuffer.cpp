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

//! @file TestRingBuffer.cpp
//! @brief Unit tests for qgraphplot::QRingBufferSeriesModel.
//!
//! Covers append, overflow eviction, range queries, bounds tracking, clear,
//! and signal emission. Companion to Phase 0.2 (issue #3).

#include <QtTest/QtTest>

#include "../src/core/model/QRingBufferSeriesModel.h"

#include <vector>

using namespace qgraphplot;

class TestRingBuffer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // ─ Construction ────────────────────────────────────────
    void constructValid();
    void constructInvalidCapacityClamps();

    // ─ Append (single) ─────────────────────────────────────
    void appendSingleGrowsSize();
    void appendSingleUpdatesBounds();

    // ─ Append (batch) ──────────────────────────────────────
    void appendBatchGrowsSize();
    void appendBatchEmptyIsNoop();

    // ─ Overflow / eviction ─────────────────────────────────
    void overflowEvictsOldest();
    void appendLargerThanCapacityKeepsTail();

    // ─ Queries ─────────────────────────────────────────────
    void pointAtReturnsLogicalOrder();
    void pointsRangeNonWrapping();
    void boundsAfterEvictionRecomputed();

    // ─ Clear ───────────────────────────────────────────────
    void clearEmptiesBuffer();
    void clearPreservesCapacity();

    // ─ Signals ─────────────────────────────────────────────
    void signalsOnAppend();
    void signalsOnOverflow();
    void noSignalsOnEmptyAppend();
};

void TestRingBuffer::initTestCase() {
    // Sanity: nothing global to set up.
}

// ─────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::constructValid() {
    QRingBufferSeriesModel rb(8);
    QCOMPARE(rb.capacity(), qsizetype(8));
    QCOMPARE(rb.pointCount(), qsizetype(0));
    QCOMPARE(rb.threadSafety(), ThreadSafety::Disabled);
    QVERIFY(rb.bounds().isNull());
}

void TestRingBuffer::constructInvalidCapacityClamps() {
    // AI.md §3.3: invalid input is reported and clamped, not silently UB.
    QTest::ignoreMessage(QtWarningMsg,
        "QRingBufferSeriesModel: capacity must be > 0; clamped to 1");
    QRingBufferSeriesModel rb(0);
    QCOMPARE(rb.capacity(), qsizetype(1));
}

// ─────────────────────────────────────────────────────────────
// Append (single)
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::appendSingleGrowsSize() {
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(1.0, 10.0));
    rb.append(QPointF(2.0, 20.0));
    QCOMPARE(rb.pointCount(), qsizetype(2));
    QCOMPARE(rb.pointAt(0), QPointF(1.0, 10.0));
    QCOMPARE(rb.pointAt(1), QPointF(2.0, 20.0));
}

void TestRingBuffer::appendSingleUpdatesBounds() {
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(1.0, 10.0));
    rb.append(QPointF(3.0, -5.0));
    QCOMPARE(rb.bounds(), QRectF(1.0, -5.0, 2.0, 15.0));
}

// ─────────────────────────────────────────────────────────────
// Append (batch)
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::appendBatchGrowsSize() {
    std::vector<QPointF> pts = {
        QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0)
    };
    QRingBufferSeriesModel rb(8);
    rb.appendRange(pts);
    QCOMPARE(rb.pointCount(), qsizetype(3));
    QCOMPARE(rb.pointAt(2), QPointF(2.0, 2.0));
}

void TestRingBuffer::appendBatchEmptyIsNoop() {
    QRingBufferSeriesModel rb(8);
    rb.appendBatch(QSpan<const QPointF>());
    QCOMPARE(rb.pointCount(), qsizetype(0));
    QVERIFY(rb.bounds().isNull());
}

// ─────────────────────────────────────────────────────────────
// Overflow / eviction
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::overflowEvictsOldest() {
    std::vector<QPointF> first = {
        QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0)
    };
    QRingBufferSeriesModel rb(3);
    rb.appendRange(first);
    rb.append(QPointF(3.0, 3.0));  // overflow → evicts (0,0)

    QCOMPARE(rb.pointCount(), qsizetype(3));
    QCOMPARE(rb.pointAt(0), QPointF(1.0, 1.0));  // oldest remaining
    QCOMPARE(rb.pointAt(2), QPointF(3.0, 3.0));  // newest
}

void TestRingBuffer::appendLargerThanCapacityKeepsTail() {
    std::vector<QPointF> pts = {
        QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0),
        QPointF(3.0, 3.0), QPointF(4.0, 4.0)
    };
    QRingBufferSeriesModel rb(3);
    rb.appendRange(pts);  // batch > capacity

    QCOMPARE(rb.pointCount(), qsizetype(3));
    QCOMPARE(rb.pointAt(0), QPointF(2.0, 2.0));  // only last 3 retained
    QCOMPARE(rb.pointAt(2), QPointF(4.0, 4.0));
}

// ─────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::pointAtReturnsLogicalOrder() {
    QRingBufferSeriesModel rb(3);
    rb.append(QPointF(10.0, 0.0));
    rb.append(QPointF(20.0, 0.0));
    rb.append(QPointF(30.0, 0.0));
    rb.append(QPointF(40.0, 0.0));  // evicts (10,0)

    // Logical order = oldest-first = [20, 30, 40]
    QCOMPARE(rb.pointAt(0).x(), 20.0);
    QCOMPARE(rb.pointAt(1).x(), 30.0);
    QCOMPARE(rb.pointAt(2).x(), 40.0);
}

void TestRingBuffer::pointsRangeNonWrapping() {
    std::vector<QPointF> pts = {
        QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0), QPointF(3.0, 3.0)
    };
    QRingBufferSeriesModel rb(8);
    rb.appendRange(pts);

    auto span = rb.points(1, 2);
    QCOMPARE(span.size(), qsizetype(2));
    QCOMPARE(span[0], QPointF(1.0, 1.0));
    QCOMPARE(span[1], QPointF(2.0, 2.0));
}

void TestRingBuffer::boundsAfterEvictionRecomputed() {
    QRingBufferSeriesModel rb(3);
    rb.append(QPointF(-100.0, 0.0));  // x_min
    rb.append(QPointF(0.0, 0.0));
    rb.append(QPointF(100.0, 0.0));   // x_max
    QCOMPARE(rb.bounds().left(), -100.0);
    QCOMPARE(rb.bounds().right(), 100.0);

    rb.append(QPointF(50.0, 0.0));  // evicts (-100, 0)
    // After eviction bounds must be recomputed, x_min now 0.
    QCOMPARE(rb.bounds().left(), 0.0);
    QCOMPARE(rb.bounds().right(), 100.0);
}

// ─────────────────────────────────────────────────────────────
// Clear
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::clearEmptiesBuffer() {
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(1.0, 1.0));
    rb.append(QPointF(2.0, 2.0));
    rb.clear();
    QCOMPARE(rb.pointCount(), qsizetype(0));
    QVERIFY(rb.bounds().isNull());
}

void TestRingBuffer::clearPreservesCapacity() {
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(1.0, 1.0));
    rb.clear();
    QCOMPARE(rb.capacity(), qsizetype(4));
}

// ─────────────────────────────────────────────────────────────
// Signals
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::signalsOnAppend() {
    QRingBufferSeriesModel rb(8);
    QSignalSpy insertedSpy(&rb, &QRingBufferSeriesModel::pointsInserted);
    QSignalSpy changedSpy(&rb, &QRingBufferSeriesModel::modelChanged);

    rb.append(QPointF(1.0, 1.0));

    QCOMPARE(insertedSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toLongLong(), qsizetype(1));
}

void TestRingBuffer::signalsOnOverflow() {
    QRingBufferSeriesModel rb(2);
    rb.append(QPointF(1.0, 1.0));
    rb.append(QPointF(2.0, 2.0));

    QSignalSpy removedSpy(&rb, &QRingBufferSeriesModel::pointsRemoved);
    QSignalSpy insertedSpy(&rb, &QRingBufferSeriesModel::pointsInserted);

    rb.append(QPointF(3.0, 3.0));  // evicts oldest

    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(insertedSpy.count(), 1);
    auto removedArgs = removedSpy.takeFirst();
    QCOMPARE(removedArgs.at(0).toLongLong(), qsizetype(0));  // first removed index
    QCOMPARE(removedArgs.at(1).toLongLong(), qsizetype(0));  // last removed index
}

void TestRingBuffer::noSignalsOnEmptyAppend() {
    QRingBufferSeriesModel rb(8);
    QSignalSpy insertedSpy(&rb, &QRingBufferSeriesModel::pointsInserted);
    QSignalSpy changedSpy(&rb, &QRingBufferSeriesModel::modelChanged);

    rb.appendBatch(QSpan<const QPointF>());

    QCOMPARE(insertedSpy.count(), 0);
    QCOMPARE(changedSpy.count(), 0);
}

QTEST_GUILESS_MAIN(TestRingBuffer)
#include "TestRingBuffer.moc"
