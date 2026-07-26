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

//! @file TestAutoScaler.cpp
//! @brief Unit tests for qgraphplot::QAutoScaler — the shared axis auto-range
//!        helper consumed by both QmlChartView and WidgetChartView (#63).

#include <limits>
#include <QPointF>
#include <QRectF>
#include <QRegularExpression>

#include <QtTest/QtTest>

#include "../src/core/model/QStaticSeriesModel.h"
#include "../src/core/series/QLineSeries.h"
#include "../src/core/transform/QAutoScaler.h"

using qgraphplot::QAbstractSeries;
using qgraphplot::QAutoScaler;
using qgraphplot::QLineSeries;
using qgraphplot::QStaticSeriesModel;

namespace
{

//! Builds a series backed by a QStaticSeriesModel loaded with @p points.
//! Both objects are parented to @p owner for RAII cleanup.
QLineSeries* makeSeries(QObject& owner, const QList<QPointF>& points)
{
    auto* model = new QStaticSeriesModel(&owner);
    model->setPoints(QSpan<const QPointF>(points.data(), points.size()));
    auto* series = new QLineSeries(&owner);
    series->setModel(model);
    return series;
}

}  // namespace

class TestAutoScaler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void emptyListReturnsFallback();
    void nullSeriesAreSkipped();
    void nullModelSeriesAreSkipped();
    void emptyModelSeriesAreSkipped();
    void singleSeriesBoundsArePadded();
    void multipleSeriesAreUnioned();
    void zeroWidthSpanIsExpandedByUnit();
    void paddingZeroFitsExactly();
    void negativePaddingIsClamped();
    void nonFinitePaddingIsClamped();
    void paddingDefaultMatchesFivePercent();
};

void TestAutoScaler::initTestCase() {}

void TestAutoScaler::emptyListReturnsFallback()
{
    QList<QAbstractSeries*> empty;
    const QRectF r = QAutoScaler::computePaddedBounds(empty, 0.05);
    QCOMPARE(r, QRectF(0.0, 0.0, 1.0, 1.0));
}

void TestAutoScaler::nullSeriesAreSkipped()
{
    QList<QAbstractSeries*> series{nullptr, nullptr};
    const QRectF r = QAutoScaler::computePaddedBounds(series, 0.05);
    QCOMPARE(r, QRectF(0.0, 0.0, 1.0, 1.0));
}

void TestAutoScaler::nullModelSeriesAreSkipped()
{
    QObject owner;
    auto* noModel = new QLineSeries(&owner);  // model() is null by default
    QList<QAbstractSeries*> series{noModel};
    const QRectF r = QAutoScaler::computePaddedBounds(series, 0.05);
    QCOMPARE(r, QRectF(0.0, 0.0, 1.0, 1.0));
}

void TestAutoScaler::emptyModelSeriesAreSkipped()
{
    QObject owner;
    auto* emptyModelSeries = makeSeries(&owner, QList<QPointF>{});  // 0 points → skipped
    QList<QAbstractSeries*> series{emptyModelSeries};
    const QRectF r = QAutoScaler::computePaddedBounds(series, 0.05);
    QCOMPARE(r, QRectF(0.0, 0.0, 1.0, 1.0));
}

void TestAutoScaler::singleSeriesBoundsArePadded()
{
    QObject owner;
    // Bounds: x∈[0, 10], y∈[-1, 1]. Padding 0.1 → 1.0 on each side per axis.
    auto* s = makeSeries(&owner, {QPointF(0.0, -1.0), QPointF(10.0, 1.0)});
    const QRectF r = QAutoScaler::computePaddedBounds({s}, 0.1);
    QCOMPARE(r.left(), -1.0);
    QCOMPARE(r.right(), 11.0);
    QCOMPARE(r.top(), -1.2);
    QCOMPARE(r.bottom(), 1.2);
    QVERIFY(r.width() > 0.0);
    QVERIFY(r.height() > 0.0);
}

void TestAutoScaler::multipleSeriesAreUnioned()
{
    QObject owner;
    auto* a = makeSeries(&owner, {QPointF(0.0, 0.0), QPointF(2.0, 2.0)});
    auto* b = makeSeries(&owner, {QPointF(5.0, -3.0), QPointF(8.0, 4.0)});
    const QRectF r = QAutoScaler::computePaddedBounds({a, b}, 0.0);
    // Union without padding: x∈[0, 8], y∈[-3, 4].
    QCOMPARE(r.left(), 0.0);
    QCOMPARE(r.right(), 8.0);
    QCOMPARE(r.top(), -3.0);
    QCOMPARE(r.bottom(), 4.0);
}

void TestAutoScaler::zeroWidthSpanIsExpandedByUnit()
{
    QObject owner;
    // Single point → bounds width==0 AND height==0. Each axis expands by
    // 1.0 on each side regardless of padding (issue #63 edge case).
    auto* s = makeSeries(&owner, {QPointF(5.0, 7.0)});
    const QRectF r = QAutoScaler::computePaddedBounds({s}, 0.05);
    QCOMPARE(r.left(), 4.0);
    QCOMPARE(r.right(), 6.0);
    QCOMPARE(r.top(), 6.0);
    QCOMPARE(r.bottom(), 8.0);

    // Also true when padding is 0: the degenerate-axis fallback is mandatory.
    const QRectF r0 = QAutoScaler::computePaddedBounds({s}, 0.0);
    QCOMPARE(r0.left(), 4.0);
    QCOMPARE(r0.right(), 6.0);
}

void TestAutoScaler::paddingZeroFitsExactly()
{
    QObject owner;
    auto* s = makeSeries(&owner, {QPointF(0.0, 0.0), QPointF(10.0, 10.0)});
    const QRectF r = QAutoScaler::computePaddedBounds({s}, 0.0);
    // Exact fit, no padding — but still a valid (min < max) range.
    QCOMPARE(r.left(), 0.0);
    QCOMPARE(r.right(), 10.0);
    QCOMPARE(r.top(), 0.0);
    QCOMPARE(r.bottom(), 10.0);
    QVERIFY(r.width() > 0.0);
    QVERIFY(r.height() > 0.0);
}

void TestAutoScaler::negativePaddingIsClamped()
{
    QObject owner;
    auto* s = makeSeries(&owner, {QPointF(0.0, 0.0), QPointF(10.0, 10.0)});
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QRegularExpression::escape(
                             "QAutoScaler: clamping invalid padding ratio to 0.0:")));
    const QRectF r = QAutoScaler::computePaddedBounds({s}, -0.5);
    // Negative clamped to 0 → exact fit, same as paddingZeroFitsExactly.
    QCOMPARE(r, QRectF(0.0, 0.0, 10.0, 10.0));
}

void TestAutoScaler::nonFinitePaddingIsClamped()
{
    QObject owner;
    auto* s = makeSeries(&owner, {QPointF(0.0, 0.0), QPointF(10.0, 10.0)});
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QRegularExpression::escape(
                             "QAutoScaler: clamping invalid padding ratio to 0.0:")));
    const QRectF r = QAutoScaler::computePaddedBounds({s}, std::numeric_limits<double>::infinity());
    QCOMPARE(r, QRectF(0.0, 0.0, 10.0, 10.0));
}

void TestAutoScaler::paddingDefaultMatchesFivePercent()
{
    QObject owner;
    // Span 10 → 0.05 padding = 0.5 on each side.
    auto* s = makeSeries(&owner, {QPointF(0.0, 0.0), QPointF(10.0, 10.0)});
    const QRectF r = QAutoScaler::computePaddedBounds({s}, 0.05);
    QCOMPARE(r.left(), -0.5);
    QCOMPARE(r.right(), 10.5);
    QCOMPARE(r.top(), -0.5);
    QCOMPARE(r.bottom(), 10.5);
}

QTEST_GUILESS_MAIN(TestAutoScaler)
#include "TestAutoScaler.moc"
