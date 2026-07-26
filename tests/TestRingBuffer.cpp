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

#include <algorithm>
#include <atomic>
#include <deque>
#include <limits>
#include <thread>
#include <vector>

#include <QtTest/QtTest>

#include "../src/core/model/QRingBufferSeriesModel.h"

using namespace qgraphplot;

class TestRingBuffer : public QObject
{
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
    void pointsRangeWrapping();
    void boundsAfterEvictionRecomputed();

    // ─ Clear ───────────────────────────────────────────────
    void clearEmptiesBuffer();
    void clearPreservesCapacity();

    // ─ Signals ─────────────────────────────────────────────
    void signalsOnAppend();
    void signalsOnOverflow();
    void noSignalsOnEmptyAppend();
    void signalsOnClearNonEmpty();
    void noSignalsOnClearAlreadyEmpty();

    // ─ Thread safety (#33, #51) ────────────────────────────
    void threadSafeAppendFromMultipleThreads();
    void concurrentAppendAndRead();

    // ─ Incremental bounds (#37) ────────────────────────────
    void boundsIncrementalMatchesBruteForceUnderEviction();
    void boundsWithDuplicateExtremeValuesMatchesBruteForce();
    void boundsAllPointsIdenticalStaysZeroArea();
    void boundsWithCapacityOneAlwaysMatchesLatestPoint();
    void clearResetsIncrementalBoundsState();
    void boundsAfterEvictingBothMinAndMaxSimultaneously();
    void appendBatchDuplicateExtremesWithinSingleBatch();
    void boundsAfterBatchPartialEviction();
};

void TestRingBuffer::initTestCase()
{
    // Sanity: nothing global to set up.
}

// ─────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::constructValid()
{
    QRingBufferSeriesModel rb(8);
    QCOMPARE(rb.capacity(), qsizetype(8));
    QCOMPARE(rb.pointCount(), qsizetype(0));
    QCOMPARE(rb.threadSafety(), ThreadSafety::Disabled);
    QVERIFY(rb.bounds().isNull());
}

void TestRingBuffer::constructInvalidCapacityClamps()
{
    // AI.md §3.3: invalid input is reported and clamped, not silently UB.
    QTest::ignoreMessage(QtWarningMsg,
                         "QRingBufferSeriesModel: capacity must be > 0; clamped to 1");
    QRingBufferSeriesModel rb(0);
    QCOMPARE(rb.capacity(), qsizetype(1));
}

// ─────────────────────────────────────────────────────────────
// Append (single)
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::appendSingleGrowsSize()
{
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(1.0, 10.0));
    rb.append(QPointF(2.0, 20.0));
    QCOMPARE(rb.pointCount(), qsizetype(2));
    QCOMPARE(rb.pointAt(0), QPointF(1.0, 10.0));
    QCOMPARE(rb.pointAt(1), QPointF(2.0, 20.0));
}

void TestRingBuffer::appendSingleUpdatesBounds()
{
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(1.0, 10.0));
    rb.append(QPointF(3.0, -5.0));
    QCOMPARE(rb.bounds(), QRectF(1.0, -5.0, 2.0, 15.0));
}

// ─────────────────────────────────────────────────────────────
// Append (batch)
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::appendBatchGrowsSize()
{
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0)};
    QRingBufferSeriesModel rb(8);
    rb.appendRange(pts);
    QCOMPARE(rb.pointCount(), qsizetype(3));
    QCOMPARE(rb.pointAt(2), QPointF(2.0, 2.0));
}

void TestRingBuffer::appendBatchEmptyIsNoop()
{
    QRingBufferSeriesModel rb(8);
    rb.appendBatch(QSpan<const QPointF>());
    QCOMPARE(rb.pointCount(), qsizetype(0));
    QVERIFY(rb.bounds().isNull());
}

// ─────────────────────────────────────────────────────────────
// Overflow / eviction
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::overflowEvictsOldest()
{
    std::vector<QPointF> first = {QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0)};
    QRingBufferSeriesModel rb(3);
    rb.appendRange(first);
    rb.append(QPointF(3.0, 3.0));  // overflow → evicts (0,0)

    QCOMPARE(rb.pointCount(), qsizetype(3));
    QCOMPARE(rb.pointAt(0), QPointF(1.0, 1.0));  // oldest remaining
    QCOMPARE(rb.pointAt(2), QPointF(3.0, 3.0));  // newest
}

void TestRingBuffer::appendLargerThanCapacityKeepsTail()
{
    std::vector<QPointF> pts = {QPointF(0.0, 0.0),
                                QPointF(1.0, 1.0),
                                QPointF(2.0, 2.0),
                                QPointF(3.0, 3.0),
                                QPointF(4.0, 4.0)};
    QRingBufferSeriesModel rb(3);
    rb.appendRange(pts);  // batch > capacity

    QCOMPARE(rb.pointCount(), qsizetype(3));
    QCOMPARE(rb.pointAt(0), QPointF(2.0, 2.0));  // only last 3 retained
    QCOMPARE(rb.pointAt(2), QPointF(4.0, 4.0));
}

// ─────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::pointAtReturnsLogicalOrder()
{
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

void TestRingBuffer::pointsRangeNonWrapping()
{
    std::vector<QPointF> pts = {
        QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0), QPointF(3.0, 3.0)};
    QRingBufferSeriesModel rb(8);
    rb.appendRange(pts);

    auto span = rb.points(1, 2);
    QCOMPARE(span.size(), qsizetype(2));
    QCOMPARE(span[0], QPointF(1.0, 1.0));
    QCOMPARE(span[1], QPointF(2.0, 2.0));
}

void TestRingBuffer::pointsRangeWrapping()
{
    // Fill the ring exactly, then force one eviction so the logical range
    // [0, size-1] straddles the physical wrap boundary (regression test:
    // points() used to silently truncate this to a partial span).
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(0.0, 0.0));
    rb.append(QPointF(1.0, 1.0));
    rb.append(QPointF(2.0, 2.0));
    rb.append(QPointF(3.0, 3.0));
    rb.append(QPointF(4.0, 4.0));  // evicts (0,0); head now mid-buffer

    auto span = rb.points(0, rb.pointCount() - 1);
    QCOMPARE(span.size(), qsizetype(4));
    QCOMPARE(span[0], QPointF(1.0, 1.0));
    QCOMPARE(span[1], QPointF(2.0, 2.0));
    QCOMPARE(span[2], QPointF(3.0, 3.0));
    QCOMPARE(span[3], QPointF(4.0, 4.0));
}

void TestRingBuffer::boundsAfterEvictionRecomputed()
{
    QRingBufferSeriesModel rb(3);
    rb.append(QPointF(-100.0, 0.0));  // x_min
    rb.append(QPointF(0.0, 0.0));
    rb.append(QPointF(100.0, 0.0));  // x_max
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

void TestRingBuffer::clearEmptiesBuffer()
{
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(1.0, 1.0));
    rb.append(QPointF(2.0, 2.0));
    rb.clear();
    QCOMPARE(rb.pointCount(), qsizetype(0));
    QVERIFY(rb.bounds().isNull());
}

void TestRingBuffer::clearPreservesCapacity()
{
    QRingBufferSeriesModel rb(4);
    rb.append(QPointF(1.0, 1.0));
    rb.clear();
    QCOMPARE(rb.capacity(), qsizetype(4));
}

// ─────────────────────────────────────────────────────────────
// Signals
// ─────────────────────────────────────────────────────────────

void TestRingBuffer::signalsOnAppend()
{
    QRingBufferSeriesModel rb(8);
    QSignalSpy insertedSpy(&rb, &QRingBufferSeriesModel::pointsInserted);
    QSignalSpy changedSpy(&rb, &QRingBufferSeriesModel::modelChanged);

    rb.append(QPointF(1.0, 1.0));

    QCOMPARE(insertedSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toLongLong(), qsizetype(1));
}

void TestRingBuffer::signalsOnOverflow()
{
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

void TestRingBuffer::noSignalsOnEmptyAppend()
{
    QRingBufferSeriesModel rb(8);
    QSignalSpy insertedSpy(&rb, &QRingBufferSeriesModel::pointsInserted);
    QSignalSpy changedSpy(&rb, &QRingBufferSeriesModel::modelChanged);

    rb.appendBatch(QSpan<const QPointF>());

    QCOMPARE(insertedSpy.count(), 0);
    QCOMPARE(changedSpy.count(), 0);
}

void TestRingBuffer::signalsOnClearNonEmpty()
{
    // Regression test: clear() used to always emit pointsRemoved(0, 0)
    // regardless of how many points were actually held.
    QRingBufferSeriesModel rb(8);
    rb.append(QPointF(1.0, 1.0));
    rb.append(QPointF(2.0, 2.0));
    rb.append(QPointF(3.0, 3.0));

    QSignalSpy removedSpy(&rb, &QRingBufferSeriesModel::pointsRemoved);
    rb.clear();

    QCOMPARE(removedSpy.count(), 1);
    auto removedArgs = removedSpy.takeFirst();
    QCOMPARE(removedArgs.at(0).toLongLong(), qsizetype(0));
    QCOMPARE(removedArgs.at(1).toLongLong(), qsizetype(2));  // last of 3 points
}

void TestRingBuffer::noSignalsOnClearAlreadyEmpty()
{
    QRingBufferSeriesModel rb(8);
    QSignalSpy removedSpy(&rb, &QRingBufferSeriesModel::pointsRemoved);

    rb.clear();

    QCOMPARE(removedSpy.count(), 0);
}

void TestRingBuffer::threadSafeAppendFromMultipleThreads()
{
    // Regression test for #33: ThreadSafety::Enabled used to take no lock
    // at all, so concurrent appendBatch() calls raced on m_head/m_size/
    // m_buffer. With capacity large enough to avoid eviction, every point
    // from every thread must survive; a broken lock tends to corrupt
    // bookkeeping and lose points (or crash) well before this count is
    // reached in practice.
    constexpr int kThreadCount = 8;
    constexpr int kAppendsPerThread = 2000;

    QRingBufferSeriesModel rb(kThreadCount * kAppendsPerThread, ThreadSafety::Enabled);
    QCOMPARE(rb.threadSafety(), ThreadSafety::Enabled);

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (int t = 0; t < kThreadCount; ++t) {
        threads.emplace_back([&rb, t]() {
            for (int i = 0; i < kAppendsPerThread; ++i) {
                rb.append(QPointF(static_cast<double>(t), static_cast<double>(i)));
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    QCOMPARE(rb.pointCount(), static_cast<qsizetype>(kThreadCount * kAppendsPerThread));
}

void TestRingBuffer::concurrentAppendAndRead()
{
    // Fixes #51: Verify multi-threaded safety under concurrent reads and writes
    // when ThreadSafety::Enabled is specified. Producer threads append points
    // while Consumer threads continuously query pointCount(), bounds(), and pointAt().
    constexpr int kProducerCount = 4;
    constexpr int kConsumerCount = 4;
    constexpr int kOpsPerThread = 1500;

    QRingBufferSeriesModel rb(5000, ThreadSafety::Enabled);
    std::atomic<bool> producersDone{false};
    std::atomic<int> readErrors{0};

    std::vector<std::thread> producers;
    producers.reserve(kProducerCount);
    for (int t = 0; t < kProducerCount; ++t) {
        producers.emplace_back([&rb, t]() {
            for (int i = 0; i < kOpsPerThread; ++i) {
                rb.append(
                    QPointF(static_cast<double>(t * kOpsPerThread + i), static_cast<double>(i)));
            }
        });
    }

    std::vector<std::thread> consumers;
    consumers.reserve(kConsumerCount);
    for (int c = 0; c < kConsumerCount; ++c) {
        consumers.emplace_back([&rb, &producersDone, &readErrors]() {
            while (!producersDone.load(std::memory_order_relaxed)) {
                qsizetype count = rb.pointCount();
                if (count > 0) {
                    QPointF pt = rb.pointAt(count - 1);
                    if (!qIsFinite(pt.x()) || !qIsFinite(pt.y())) {
                        readErrors.fetch_add(1, std::memory_order_relaxed);
                    }
                    QRectF b = rb.bounds();
                    if (!b.isValid() && !b.isNull() && count > 1) {
                        readErrors.fetch_add(1, std::memory_order_relaxed);
                    }
                }
                std::this_thread::yield();
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }
    producersDone.store(true, std::memory_order_relaxed);

    for (auto& consumer : consumers) {
        consumer.join();
    }

    QCOMPARE(readErrors.load(), 0);
    QCOMPARE(rb.pointCount(), static_cast<qsizetype>(5000));
}

void TestRingBuffer::boundsIncrementalMatchesBruteForceUnderEviction()
{
    // Regression test for #37: cross-validates the O(1) incremental
    // min/max tracking (monotonic deques) against a brute-force scan over
    // a shadow FIFO, across many pushes and evictions -- including cases
    // where the evicted point was the current min or max, which is
    // exactly the scenario the old O(capacity) recomputeBounds() existed
    // to handle correctly.
    constexpr qsizetype kCapacity = 37;  // deliberately not a power of two
    QRingBufferSeriesModel rb(kCapacity);
    std::deque<QPointF> shadow;

    // Simple deterministic PRNG (LCG) instead of <random> so the test is
    // reproducible across platforms/toolchains without seeding concerns.
    quint32 state = 12345u;
    auto nextValue = [&state]() -> double {
        state = state * 1664525u + 1013904223u;
        return (static_cast<double>(state) /
                static_cast<double>(std::numeric_limits<quint32>::max())) *
                   200.0 -
               100.0;  // range roughly [-100, 100]
    };

    for (int i = 0; i < 500; ++i) {
        const QPointF pt(nextValue(), nextValue());
        rb.append(pt);

        shadow.push_back(pt);
        if (shadow.size() > static_cast<size_t>(kCapacity)) {
            shadow.pop_front();
        }

        qreal minX = shadow.front().x();
        qreal maxX = minX;
        qreal minY = shadow.front().y();
        qreal maxY = minY;
        for (const QPointF& p : shadow) {
            minX = std::min(minX, p.x());
            maxX = std::max(maxX, p.x());
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }
        const QRectF expected(minX, minY, maxX - minX, maxY - minY);
        QCOMPARE(rb.bounds(), expected);
    }
}

void TestRingBuffer::boundsWithDuplicateExtremeValuesMatchesBruteForce()
{
    // Small integer range forces frequent duplicate min/max values, which
    // stresses the >= / <= tie-breaking in pushMinCandidate()/
    // pushMaxCandidate(): a naive strict > / < comparison would keep a
    // stale duplicate around that is about to be evicted, instead of
    // discarding it in favor of the newer, equally-extreme point that will
    // remain live longer.
    constexpr qsizetype kCapacity = 5;
    QRingBufferSeriesModel rb(kCapacity);
    std::deque<QPointF> shadow;

    quint32 state = 987u;
    auto nextValue = [&state]() -> double {
        state = state * 1664525u + 1013904223u;
        return static_cast<double>(state % 7) - 3.0;  // integers in [-3, 3]
    };

    for (int i = 0; i < 300; ++i) {
        const QPointF pt(nextValue(), nextValue());
        rb.append(pt);

        shadow.push_back(pt);
        if (shadow.size() > static_cast<size_t>(kCapacity)) {
            shadow.pop_front();
        }

        qreal minX = shadow.front().x();
        qreal maxX = minX;
        qreal minY = shadow.front().y();
        qreal maxY = minY;
        for (const QPointF& p : shadow) {
            minX = std::min(minX, p.x());
            maxX = std::max(maxX, p.x());
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }
        const QRectF expected(minX, minY, maxX - minX, maxY - minY);
        QCOMPARE(rb.bounds(), expected);
    }
}

void TestRingBuffer::boundsAllPointsIdenticalStaysZeroArea()
{
    // Every candidate deque should collapse to a single repeated value
    // without ever mis-evicting it, keeping a stable zero-area rect.
    QRingBufferSeriesModel rb(4);
    for (int i = 0; i < 10; ++i) {
        rb.append(QPointF(7.0, -2.0));
    }
    QCOMPARE(rb.bounds(), QRectF(7.0, -2.0, 0.0, 0.0));
}

void TestRingBuffer::boundsWithCapacityOneAlwaysMatchesLatestPoint()
{
    // Capacity 1 means every append evicts the previous point, exercising
    // evictStaleCandidates() on every single call.
    QRingBufferSeriesModel rb(1);
    rb.append(QPointF(1.0, 1.0));
    QCOMPARE(rb.bounds(), QRectF(1.0, 1.0, 0.0, 0.0));

    rb.append(QPointF(-5.0, 20.0));
    QCOMPARE(rb.bounds(), QRectF(-5.0, 20.0, 0.0, 0.0));

    rb.append(QPointF(3.0, 3.0));
    QCOMPARE(rb.bounds(), QRectF(3.0, 3.0, 0.0, 0.0));
}

void TestRingBuffer::clearResetsIncrementalBoundsState()
{
    // Regression test: clear() must reset m_nextPushIndex and empty all
    // four candidate deques. If it didn't, stale pushIndex values from
    // before the clear could be misinterpreted against the push-index
    // sequence restarting at 0 after the buffer is refilled.
    QRingBufferSeriesModel rb(3);
    rb.append(QPointF(-100.0, -100.0));
    rb.append(QPointF(100.0, 100.0));
    rb.append(QPointF(0.0, 0.0));
    rb.append(QPointF(50.0, 50.0));  // triggers eviction, advances m_nextPushIndex

    rb.clear();
    QVERIFY(rb.bounds().isNull());

    rb.append(QPointF(1.0, 2.0));
    rb.append(QPointF(3.0, 4.0));
    QCOMPARE(rb.bounds(), QRectF(1.0, 2.0, 2.0, 2.0));
}

void TestRingBuffer::boundsAfterEvictingBothMinAndMaxSimultaneously()
{
    // The evicted point is simultaneously the current minX AND maxY
    // candidate, so a single eviction must pop the front of two different
    // deques in the same appendBatch() call.
    QRingBufferSeriesModel rb(2);
    rb.append(QPointF(-10.0, 100.0));  // minX and maxY candidate
    rb.append(QPointF(5.0, 5.0));
    QCOMPARE(rb.bounds(), QRectF(-10.0, 5.0, 15.0, 95.0));

    rb.append(QPointF(0.0, 0.0));  // evicts (-10, 100): both minX and maxY leaders
    QCOMPARE(rb.bounds(), QRectF(0.0, 0.0, 5.0, 5.0));
}

void TestRingBuffer::appendBatchDuplicateExtremesWithinSingleBatch()
{
    // Duplicate min/max values arriving together in ONE appendBatch() call
    // (as opposed to across separate append() calls) exercise the
    // per-batch push loop, where all four deques are updated for the whole
    // batch before any eviction is applied.
    std::vector<QPointF> pts = {QPointF(2.0, 2.0),
                                QPointF(2.0, 2.0),  // duplicate of running max so far
                                QPointF(-2.0, -2.0),
                                QPointF(-2.0, -2.0)};  // duplicate of new running min
    QRingBufferSeriesModel rb(8);
    rb.appendRange(pts);

    QCOMPARE(rb.pointCount(), qsizetype(4));
    QCOMPARE(rb.bounds(), QRectF(-2.0, -2.0, 4.0, 4.0));
}

void TestRingBuffer::boundsAfterBatchPartialEviction()
{
    // A single appendBatch() call whose size is smaller than capacity but
    // still forces eviction of previously-held points (evicted > 0 and
    // appended < capacity in the same call), including eviction of the
    // current min/max, which is a less common path than the
    // append-one-at-a-time regression tests above.
    QRingBufferSeriesModel rb(4);
    std::vector<QPointF> seed = {
        QPointF(-1.0, -1.0), QPointF(-2.0, -2.0), QPointF(10.0, 10.0), QPointF(3.0, 3.0)};
    rb.appendRange(seed);
    QCOMPARE(rb.bounds(), QRectF(-2.0, -2.0, 12.0, 12.0));

    // Remaining free space is 0, so this batch of 2 evicts the two oldest
    // points (-1,-1) and (-2,-2) -- including the current min.
    std::vector<QPointF> batch = {QPointF(0.0, 0.0), QPointF(1.0, 1.0)};
    rb.appendRange(batch);

    QCOMPARE(rb.pointCount(), qsizetype(4));
    // Remaining points: (10,10), (3,3), (0,0), (1,1).
    QCOMPARE(rb.bounds(), QRectF(0.0, 0.0, 10.0, 10.0));
}

QTEST_GUILESS_MAIN(TestRingBuffer)
#include "TestRingBuffer.moc"
