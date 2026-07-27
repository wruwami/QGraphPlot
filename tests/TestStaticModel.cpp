// QGraphPlot High-performance Qt chart library
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

//! @file TestStaticModel.cpp
//! @brief Unit tests for qgraphplot::QStaticSeriesModel.
//!
//! Covers setPoints, append, appendBatch, replacePoint, clear, reserve,
//! bounds tracking, and signal emission (issue #62).

#include <limits>
#include <list>
#include <vector>

#include <QtTest/QtTest>

#include "../src/core/model/QStaticSeriesModel.h"

using namespace qgraphplot;

class TestStaticModel : public QObject
{
    Q_OBJECT

private slots:
    void constructEmpty();

    void setPointsFromEmpty();
    void setPointsReplacesExisting();
    void setPointsWithEmptyClears();

    void appendSingleGrowsSize();
    void appendSingleUpdatesBounds();
    void appendBatchGrowsSize();
    void appendBatchEmptyIsNoop();
    void appendRangeFromVector();
    void appendRangeFromList();
    void setPointsFromSelfSpan();
    void appendBatchFromSelfSpan();

    void replacePointUpdatesValue();
    void replacePointUpdatesBoundsOnShrink();
    void replacePointUpdatesBoundsOnExpand();
    void rejectNonFiniteSetPoints();
    void rejectNonFiniteAppendBatch();
    void rejectNonFiniteReplacePoint();

    void clearEmptiesModel();
    void clearAlreadyEmpty();

    void reserveDoesNotChangeCount();

    void pointAtReturnsCorrectValue();
    void pointsRangeReturnsContiguousSpan();
    void boundsCorrectAfterMultipleAppends();
    void boundsCorrectAfterSetPoints();

    void signalsOnSetPointsFromEmpty();
    void signalsOnSetPointsReplace();
    void signalsOnAppend();
    void signalsOnAppendBatch();
    void noSignalsOnEmptyAppend();
    void signalsOnReplacePoint();
    void signalsOnClearNonEmpty();
    void noSignalsOnClearEmpty();
    void boundsChangedOnReplacePointShrink();
    void noBoundsChangedOnReplacePointNoChange();
};

void TestStaticModel::constructEmpty()
{
    QStaticSeriesModel model;
    QCOMPARE(model.pointCount(), qsizetype(0));
    QVERIFY(model.bounds().isNull());
}

void TestStaticModel::setPointsFromEmpty()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(1.0, 2.0), QPointF(3.0, 4.0), QPointF(5.0, 6.0)};
    model.setPoints(pts);

    QCOMPARE(model.pointCount(), qsizetype(3));
    QCOMPARE(model.pointAt(0), QPointF(1.0, 2.0));
    QCOMPARE(model.pointAt(1), QPointF(3.0, 4.0));
    QCOMPARE(model.pointAt(2), QPointF(5.0, 6.0));
    QCOMPARE(model.bounds(), QRectF(1.0, 2.0, 4.0, 4.0));
}

void TestStaticModel::setPointsReplacesExisting()
{
    QStaticSeriesModel model;
    std::vector<QPointF> first = {QPointF(0.0, 0.0), QPointF(10.0, 10.0)};
    model.setPoints(first);
    QCOMPARE(model.pointCount(), qsizetype(2));

    std::vector<QPointF> second = {QPointF(1.0, 1.0)};
    model.setPoints(second);
    QCOMPARE(model.pointCount(), qsizetype(1));
    QCOMPARE(model.pointAt(0), QPointF(1.0, 1.0));
    QCOMPARE(model.bounds(), QRectF(1.0, 1.0, 0.0, 0.0));
}

void TestStaticModel::setPointsWithEmptyClears()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(1.0, 2.0)};
    model.setPoints(pts);
    QCOMPARE(model.pointCount(), qsizetype(1));

    model.setPoints(QSpan<const QPointF>());
    QCOMPARE(model.pointCount(), qsizetype(0));
    QVERIFY(model.bounds().isNull());
}

void TestStaticModel::appendSingleGrowsSize()
{
    QStaticSeriesModel model;
    model.append(QPointF(1.0, 10.0));
    model.append(QPointF(2.0, 20.0));
    QCOMPARE(model.pointCount(), qsizetype(2));
    QCOMPARE(model.pointAt(0), QPointF(1.0, 10.0));
    QCOMPARE(model.pointAt(1), QPointF(2.0, 20.0));
}

void TestStaticModel::appendSingleUpdatesBounds()
{
    QStaticSeriesModel model;
    model.append(QPointF(1.0, 10.0));
    model.append(QPointF(3.0, -5.0));
    QCOMPARE(model.bounds(), QRectF(1.0, -5.0, 2.0, 15.0));
}

void TestStaticModel::appendBatchGrowsSize()
{
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0)};
    QStaticSeriesModel model;
    model.appendBatch(pts);
    QCOMPARE(model.pointCount(), qsizetype(3));
    QCOMPARE(model.pointAt(2), QPointF(2.0, 2.0));
    QCOMPARE(model.bounds(), QRectF(0.0, 0.0, 2.0, 2.0));
}

void TestStaticModel::appendBatchEmptyIsNoop()
{
    QStaticSeriesModel model;
    model.appendBatch(QSpan<const QPointF>());
    QCOMPARE(model.pointCount(), qsizetype(0));
    QVERIFY(model.bounds().isNull());
}

void TestStaticModel::appendRangeFromVector()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(1.0, 2.0), QPointF(3.0, 4.0)};
    model.appendRange(pts);
    QCOMPARE(model.pointCount(), qsizetype(2));
    QCOMPARE(model.pointAt(0), QPointF(1.0, 2.0));
    QCOMPARE(model.pointAt(1), QPointF(3.0, 4.0));
}

void TestStaticModel::appendRangeFromList()
{
    QStaticSeriesModel model;
    std::list<QPointF> pts = {QPointF(-1.0, 1.0), QPointF(2.0, -2.0)};
    model.appendRange(pts);

    QCOMPARE(model.pointCount(), qsizetype(2));
    QCOMPARE(model.pointAt(0), QPointF(-1.0, 1.0));
    QCOMPARE(model.pointAt(1), QPointF(2.0, -2.0));
    QCOMPARE(model.bounds(), QRectF(-1.0, -2.0, 3.0, 3.0));
}

void TestStaticModel::setPointsFromSelfSpan()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(1.0, 1.0), QPointF(2.0, 2.0)};
    model.setPoints(pts);

    auto span = model.points(0, 1);
    model.setPoints(span);

    QCOMPARE(model.pointCount(), qsizetype(2));
    QCOMPARE(model.pointAt(0), QPointF(1.0, 1.0));
    QCOMPARE(model.pointAt(1), QPointF(2.0, 2.0));
    QCOMPARE(model.bounds(), QRectF(1.0, 1.0, 1.0, 1.0));
}

void TestStaticModel::appendBatchFromSelfSpan()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(1.0, 1.0), QPointF(2.0, 2.0)};
    model.setPoints(pts);

    auto span = model.points(0, 1);
    model.appendBatch(span);

    QCOMPARE(model.pointCount(), qsizetype(4));
    QCOMPARE(model.pointAt(2), QPointF(1.0, 1.0));
    QCOMPARE(model.pointAt(3), QPointF(2.0, 2.0));
    QCOMPARE(model.bounds(), QRectF(1.0, 1.0, 1.0, 1.0));
}

void TestStaticModel::replacePointUpdatesValue()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0)};
    model.setPoints(pts);

    model.replacePoint(1, QPointF(10.0, 10.0));
    QCOMPARE(model.pointAt(1), QPointF(10.0, 10.0));
    QCOMPARE(model.pointCount(), qsizetype(3));
}

void TestStaticModel::replacePointUpdatesBoundsOnShrink()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(10.0, 10.0)};
    model.setPoints(pts);
    QCOMPARE(model.bounds(), QRectF(0.0, 0.0, 10.0, 10.0));

    model.replacePoint(1, QPointF(5.0, 5.0));
    QCOMPARE(model.bounds(), QRectF(0.0, 0.0, 5.0, 5.0));
}

void TestStaticModel::replacePointUpdatesBoundsOnExpand()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(1.0, 1.0)};
    model.setPoints(pts);
    QCOMPARE(model.bounds(), QRectF(0.0, 0.0, 1.0, 1.0));

    model.replacePoint(1, QPointF(100.0, 100.0));
    QCOMPARE(model.bounds(), QRectF(0.0, 0.0, 100.0, 100.0));
}

void TestStaticModel::rejectNonFiniteSetPoints()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {
        QPointF(1.0, 2.0),
        QPointF(std::numeric_limits<qreal>::infinity(), 4.0),
    };

    QSignalSpy removedSpy(&model, &QStaticSeriesModel::pointsRemoved);
    QSignalSpy insertedSpy(&model, &QStaticSeriesModel::pointsInserted);
    QSignalSpy boundsSpy(&model, &QStaticSeriesModel::boundsChanged);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    QTest::ignoreMessage(QtWarningMsg, "QStaticSeriesModel::setPoints: points must be finite");
    model.setPoints(pts);

    QCOMPARE(model.pointCount(), qsizetype(0));
    QVERIFY(model.bounds().isNull());
    QCOMPARE(removedSpy.count(), 0);
    QCOMPARE(insertedSpy.count(), 0);
    QCOMPARE(boundsSpy.count(), 0);
    QCOMPARE(changedSpy.count(), 0);
}

void TestStaticModel::rejectNonFiniteAppendBatch()
{
    QStaticSeriesModel model;
    model.append(QPointF(0.0, 0.0));

    std::vector<QPointF> pts = {
        QPointF(1.0, 1.0),
        QPointF(std::numeric_limits<qreal>::quiet_NaN(), 2.0),
    };

    QSignalSpy insertedSpy(&model, &QStaticSeriesModel::pointsInserted);
    QSignalSpy boundsSpy(&model, &QStaticSeriesModel::boundsChanged);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    QTest::ignoreMessage(QtWarningMsg, "QStaticSeriesModel::appendBatch: points must be finite");
    model.appendBatch(pts);

    QCOMPARE(model.pointCount(), qsizetype(1));
    QCOMPARE(model.pointAt(0), QPointF(0.0, 0.0));
    QCOMPARE(model.bounds(), QRectF(0.0, 0.0, 0.0, 0.0));
    QCOMPARE(insertedSpy.count(), 0);
    QCOMPARE(boundsSpy.count(), 0);
    QCOMPARE(changedSpy.count(), 0);
}

void TestStaticModel::rejectNonFiniteReplacePoint()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(1.0, 1.0)};
    model.setPoints(pts);

    QSignalSpy dataChangedSpy(&model, &QStaticSeriesModel::dataChanged);
    QSignalSpy boundsSpy(&model, &QStaticSeriesModel::boundsChanged);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    QTest::ignoreMessage(QtWarningMsg, "QStaticSeriesModel::replacePoint: point must be finite");
    model.replacePoint(1, QPointF(2.0, std::numeric_limits<qreal>::infinity()));

    QCOMPARE(model.pointAt(1), QPointF(1.0, 1.0));
    QCOMPARE(model.bounds(), QRectF(0.0, 0.0, 1.0, 1.0));
    QCOMPARE(dataChangedSpy.count(), 0);
    QCOMPARE(boundsSpy.count(), 0);
    QCOMPARE(changedSpy.count(), 0);
}

void TestStaticModel::clearEmptiesModel()
{
    QStaticSeriesModel model;
    model.append(QPointF(1.0, 1.0));
    model.append(QPointF(2.0, 2.0));
    model.clear();
    QCOMPARE(model.pointCount(), qsizetype(0));
    QVERIFY(model.bounds().isNull());
}

void TestStaticModel::clearAlreadyEmpty()
{
    QStaticSeriesModel model;
    model.clear();
    QCOMPARE(model.pointCount(), qsizetype(0));
}

void TestStaticModel::reserveDoesNotChangeCount()
{
    QStaticSeriesModel model;
    model.reserve(1000);
    QCOMPARE(model.pointCount(), qsizetype(0));
    QVERIFY(model.bounds().isNull());
}

void TestStaticModel::pointAtReturnsCorrectValue()
{
    QStaticSeriesModel model;
    model.append(QPointF(10.0, 20.0));
    model.append(QPointF(30.0, 40.0));
    QCOMPARE(model.pointAt(0), QPointF(10.0, 20.0));
    QCOMPARE(model.pointAt(1), QPointF(30.0, 40.0));
}

void TestStaticModel::pointsRangeReturnsContiguousSpan()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {
        QPointF(0.0, 0.0), QPointF(1.0, 1.0), QPointF(2.0, 2.0), QPointF(3.0, 3.0)};
    model.setPoints(pts);

    auto span = model.points(1, 2);
    QCOMPARE(span.size(), qsizetype(2));
    QCOMPARE(span[0], QPointF(1.0, 1.0));
    QCOMPARE(span[1], QPointF(2.0, 2.0));
}

void TestStaticModel::boundsCorrectAfterMultipleAppends()
{
    QStaticSeriesModel model;
    model.append(QPointF(-5.0, 3.0));
    model.append(QPointF(10.0, -2.0));
    model.append(QPointF(0.0, 7.0));
    QCOMPARE(model.bounds(), QRectF(-5.0, -2.0, 15.0, 9.0));
}

void TestStaticModel::boundsCorrectAfterSetPoints()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(-1.0, -1.0), QPointF(1.0, 1.0), QPointF(0.0, 0.0)};
    model.setPoints(pts);
    QCOMPARE(model.bounds(), QRectF(-1.0, -1.0, 2.0, 2.0));
}

void TestStaticModel::signalsOnSetPointsFromEmpty()
{
    QStaticSeriesModel model;
    QSignalSpy removedSpy(&model, &QStaticSeriesModel::pointsRemoved);
    QSignalSpy insertedSpy(&model, &QStaticSeriesModel::pointsInserted);
    QSignalSpy boundsSpy(&model, &QStaticSeriesModel::boundsChanged);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    std::vector<QPointF> pts = {QPointF(1.0, 2.0), QPointF(3.0, 4.0)};
    model.setPoints(pts);

    QCOMPARE(removedSpy.count(), 0);
    QCOMPARE(insertedSpy.count(), 1);
    QCOMPARE(insertedSpy.takeFirst().at(0).toLongLong(), qsizetype(0));
    QCOMPARE(boundsSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toLongLong(), qsizetype(2));
}

void TestStaticModel::signalsOnSetPointsReplace()
{
    QStaticSeriesModel model;
    std::vector<QPointF> first = {QPointF(0.0, 0.0)};
    model.setPoints(first);

    QSignalSpy removedSpy(&model, &QStaticSeriesModel::pointsRemoved);
    QSignalSpy insertedSpy(&model, &QStaticSeriesModel::pointsInserted);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    std::vector<QPointF> second = {QPointF(1.0, 1.0), QPointF(2.0, 2.0)};
    model.setPoints(second);

    QCOMPARE(removedSpy.count(), 1);
    auto removedArgs = removedSpy.takeFirst();
    QCOMPARE(removedArgs.at(0).toLongLong(), qsizetype(0));
    QCOMPARE(removedArgs.at(1).toLongLong(), qsizetype(0));

    QCOMPARE(insertedSpy.count(), 1);
    auto insertedArgs = insertedSpy.takeFirst();
    QCOMPARE(insertedArgs.at(0).toLongLong(), qsizetype(0));
    QCOMPARE(insertedArgs.at(1).toLongLong(), qsizetype(1));

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toLongLong(), qsizetype(2));
}

void TestStaticModel::signalsOnAppend()
{
    QStaticSeriesModel model;
    QSignalSpy insertedSpy(&model, &QStaticSeriesModel::pointsInserted);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    model.append(QPointF(1.0, 1.0));

    QCOMPARE(insertedSpy.count(), 1);
    auto insertedArgs = insertedSpy.takeFirst();
    QCOMPARE(insertedArgs.at(0).toLongLong(), qsizetype(0));
    QCOMPARE(insertedArgs.at(1).toLongLong(), qsizetype(0));

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toLongLong(), qsizetype(1));
}

void TestStaticModel::signalsOnAppendBatch()
{
    QStaticSeriesModel model;
    model.append(QPointF(0.0, 0.0));

    QSignalSpy insertedSpy(&model, &QStaticSeriesModel::pointsInserted);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    std::vector<QPointF> pts = {QPointF(1.0, 1.0), QPointF(2.0, 2.0)};
    model.appendBatch(pts);

    QCOMPARE(insertedSpy.count(), 1);
    auto insertedArgs = insertedSpy.takeFirst();
    QCOMPARE(insertedArgs.at(0).toLongLong(), qsizetype(1));
    QCOMPARE(insertedArgs.at(1).toLongLong(), qsizetype(2));

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toLongLong(), qsizetype(3));
}

void TestStaticModel::noSignalsOnEmptyAppend()
{
    QStaticSeriesModel model;
    QSignalSpy insertedSpy(&model, &QStaticSeriesModel::pointsInserted);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    model.appendBatch(QSpan<const QPointF>());

    QCOMPARE(insertedSpy.count(), 0);
    QCOMPARE(changedSpy.count(), 0);
}

void TestStaticModel::signalsOnReplacePoint()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(1.0, 1.0)};
    model.setPoints(pts);

    QSignalSpy dataChangedSpy(&model, &QStaticSeriesModel::dataChanged);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    model.replacePoint(1, QPointF(5.0, 5.0));

    QCOMPARE(dataChangedSpy.count(), 1);
    auto args = dataChangedSpy.takeFirst();
    QCOMPARE(args.at(0).toLongLong(), qsizetype(1));
    QCOMPARE(args.at(1).toLongLong(), qsizetype(1));

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toLongLong(), qsizetype(2));
}

void TestStaticModel::signalsOnClearNonEmpty()
{
    QStaticSeriesModel model;
    model.append(QPointF(1.0, 1.0));
    model.append(QPointF(2.0, 2.0));
    model.append(QPointF(3.0, 3.0));

    QSignalSpy removedSpy(&model, &QStaticSeriesModel::pointsRemoved);
    QSignalSpy boundsSpy(&model, &QStaticSeriesModel::boundsChanged);
    QSignalSpy changedSpy(&model, &QStaticSeriesModel::modelChanged);

    model.clear();

    QCOMPARE(removedSpy.count(), 1);
    auto removedArgs = removedSpy.takeFirst();
    QCOMPARE(removedArgs.at(0).toLongLong(), qsizetype(0));
    QCOMPARE(removedArgs.at(1).toLongLong(), qsizetype(2));

    QCOMPARE(boundsSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toLongLong(), qsizetype(0));
}

void TestStaticModel::noSignalsOnClearEmpty()
{
    QStaticSeriesModel model;
    QSignalSpy removedSpy(&model, &QStaticSeriesModel::pointsRemoved);

    model.clear();

    QCOMPARE(removedSpy.count(), 0);
}

void TestStaticModel::boundsChangedOnReplacePointShrink()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(10.0, 10.0)};
    model.setPoints(pts);

    QSignalSpy boundsSpy(&model, &QStaticSeriesModel::boundsChanged);

    model.replacePoint(1, QPointF(5.0, 5.0));

    QCOMPARE(boundsSpy.count(), 1);
}

void TestStaticModel::noBoundsChangedOnReplacePointNoChange()
{
    QStaticSeriesModel model;
    std::vector<QPointF> pts = {QPointF(0.0, 0.0), QPointF(5.0, 5.0), QPointF(10.0, 10.0)};
    model.setPoints(pts);

    QSignalSpy boundsSpy(&model, &QStaticSeriesModel::boundsChanged);

    model.replacePoint(1, QPointF(6.0, 6.0));

    QCOMPARE(boundsSpy.count(), 0);
}

QTEST_GUILESS_MAIN(TestStaticModel)
#include "TestStaticModel.moc"
