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

//! @file TestAbstractSeries.cpp
//! @brief Unit tests for qgraphplot::QAbstractSeries.
//!
//! Because QAbstractSeries::type() is pure virtual, the test fixture
//! defines a minimal TestSeries subclass that returns SeriesType::Line.
//! This keeps the tests focused on the property layer without dragging
//! in any rendering code.

#include <memory>
#include <type_traits>

#include <QtCore/QPointer>
#include <QtTest/QtTest>

#include "../src/core/model/QRingBufferSeriesModel.h"
#include "../src/core/series/QAbstractSeries.h"
#include "../src/core/series/QLineSeries.h"

using namespace qgraphplot;

namespace
{

//! Minimal concrete series for testing the abstract property layer.
class TestSeries final : public QAbstractSeries
{
    Q_OBJECT
public:
    explicit TestSeries(QObject* parent = nullptr) : QAbstractSeries(parent) {}
    SeriesType type() const override { return SeriesType::Line; }
};

}  // namespace

class TestAbstractSeriesFixture : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // ─ Defaults ───────────────────────────────────────────────
    void defaultColorIsBlue();
    void defaultVisibleIsTrue();
    void defaultModelIsNull();
    void defaultNameIsEmpty();

    // ─ type() dispatch ────────────────────────────────────────
    void typeReturnsLineForTestSubclass();

    // ─ QLineSeries concrete type (#57) ────────────────────────
    void qLineSeriesTypeReturnsLine();
    void qLineSeriesInheritsPropertyDefaults();
    void qLineSeriesColorSettersWork();

    // ─ Validation paths (single source of truth, #57) ────────
    void setLineWidthRejectsInvalid();
    void setLineWidthAcceptsValid();
    void setDashPatternRejectsOddCount();
    void setDashPatternRejectsNonPositive();
    void setDashPatternRejectsNonFinite();
    void setDashPatternAcceptsEmpty();

    // ─ Opacity validation (#71) ──────────────────────────────
    void setOpacityRejectsInvalid();
    void setOpacityAcceptsValid();
    void setOpacityDoesNotEmitOnSameValue();

    // ─ Setters emit signals only on change ───────────────────
    void setColorEmitsOnceOnRealChange();
    void setColorDoesNotEmitOnSameValue();
    void setNameEmitsOnChange();
    void setVisibleEmitsOnChange();

    // ─ Model ownership ────────────────────────────────────────
    void setModelUpdatesPointer();
    void setModelNullClears();

    // ─ Parent / child ownership (RAII) ───────────────────────
    void parentDeletesChild();

    // ─ Copy / move disabled ──────────────────────────────────
    void nonCopyable();
};

void TestAbstractSeriesFixture::initTestCase() {}

// ─────────────────────────────────────────────────────────────
// Defaults
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::defaultColorIsBlue()
{
    TestSeries s;
    QCOMPARE(s.color(), QColor(Qt::blue));
}

void TestAbstractSeriesFixture::defaultVisibleIsTrue()
{
    TestSeries s;
    QVERIFY(s.isVisible());
}

void TestAbstractSeriesFixture::defaultModelIsNull()
{
    TestSeries s;
    QVERIFY(s.model() == nullptr);
}

void TestAbstractSeriesFixture::defaultNameIsEmpty()
{
    TestSeries s;
    QVERIFY(s.name().isEmpty());
}

// ─────────────────────────────────────────────────────────────
// type() dispatch
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::typeReturnsLineForTestSubclass()
{
    TestSeries s;
    QCOMPARE(s.type(), SeriesType::Line);
}

// ─────────────────────────────────────────────────────────────
// QLineSeries concrete type (#57)
//
// QLineSeries is the single shipped concrete QAbstractSeries subclass in
// core and the composition backend used by QmlLineSeries. These tests
// verify it satisfies the abstract contract (identity + inherited
// property behavior) so the QML frontend can safely delegate to it.
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::qLineSeriesTypeReturnsLine()
{
    QLineSeries s;
    QCOMPARE(s.type(), SeriesType::Line);
}

void TestAbstractSeriesFixture::qLineSeriesInheritsPropertyDefaults()
{
    QLineSeries s;
    // Same defaults as QAbstractSeries — proving QLineSeries adds no
    // divergent property state of its own.
    QCOMPARE(s.color(), QColor(Qt::blue));
    QVERIFY(s.isVisible());
    QVERIFY(s.model() == nullptr);
    QVERIFY(s.name().isEmpty());
    QCOMPARE(s.lineWidth(), 2.0);
    QVERIFY(s.dashPattern().isEmpty());
    QCOMPARE(s.opacity(), 1.0);
}

void TestAbstractSeriesFixture::qLineSeriesColorSettersWork()
{
    QLineSeries s;
    QSignalSpy spy(&s, &QLineSeries::colorChanged);
    s.setColor(QColor(Qt::green));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.color(), QColor(Qt::green));
}

// ─────────────────────────────────────────────────────────────
// Validation paths — single source of truth (#57)
//
// QmlLineSeries now delegates lineWidth/dashPattern validation to the
// core QAbstractSeries setters, so these tests assert the exact behavior
// both frontends inherit. Covers the previously-untested validation gap.
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::setLineWidthRejectsInvalid()
{
    QLineSeries s;
    const double original = s.lineWidth();
    QSignalSpy spy(&s, &QLineSeries::lineWidthChanged);

    s.setLineWidth(0.0);      // non-positive
    s.setLineWidth(-1.0);     // negative
    s.setLineWidth(qQNaN());  // NaN
    s.setLineWidth(qInf());   // infinity

    // Rejected: value and signal count unchanged.
    QCOMPARE(s.lineWidth(), original);
    QCOMPARE(spy.count(), 0);
}

void TestAbstractSeriesFixture::setLineWidthAcceptsValid()
{
    QLineSeries s;
    QSignalSpy spy(&s, &QLineSeries::lineWidthChanged);
    s.setLineWidth(3.5);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.lineWidth(), 3.5);
}

void TestAbstractSeriesFixture::setDashPatternRejectsOddCount()
{
    QLineSeries s;
    QVERIFY(s.dashPattern().isEmpty());  // default empty (solid)
    QSignalSpy spy(&s, &QLineSeries::dashPatternChanged);
    s.setDashPattern({1.0, 2.0, 3.0});  // 3 entries — QPen requires even
    QCOMPARE(spy.count(), 0);
    QVERIFY(s.dashPattern().isEmpty());  // unchanged
}

void TestAbstractSeriesFixture::setDashPatternRejectsNonPositive()
{
    QLineSeries s;
    QSignalSpy spy(&s, &QLineSeries::dashPatternChanged);
    s.setDashPattern({1.0, 0.0});   // zero gap
    s.setDashPattern({1.0, -2.0});  // negative gap
    QCOMPARE(spy.count(), 0);
    QVERIFY(s.dashPattern().isEmpty());
}

void TestAbstractSeriesFixture::setDashPatternRejectsNonFinite()
{
    QLineSeries s;
    QSignalSpy spy(&s, &QLineSeries::dashPatternChanged);
    s.setDashPattern({1.0, qInf()});
    s.setDashPattern({qQNaN(), 2.0});
    QCOMPARE(spy.count(), 0);
    QVERIFY(s.dashPattern().isEmpty());
}

void TestAbstractSeriesFixture::setDashPatternAcceptsEmpty()
{
    QLineSeries s;
    s.setDashPattern({1.0, 2.0});
    QVERIFY(!s.dashPattern().isEmpty());
    QSignalSpy spy(&s, &QLineSeries::dashPatternChanged);
    s.setDashPattern({});  // empty = solid line, always valid
    QCOMPARE(spy.count(), 1);
    QVERIFY(s.dashPattern().isEmpty());
}

// ─────────────────────────────────────────────────────────────
// Opacity validation (#71)
//
// opacity must be finite and in [0.0, 1.0]; out-of-range / non-finite
// values are rejected (mirrors the lineWidth rejection idiom).
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::setOpacityRejectsInvalid()
{
    QLineSeries s;
    const double original = s.opacity();  // 1.0 default
    QSignalSpy spy(&s, &QLineSeries::opacityChanged);

    s.setOpacity(-0.1);     // below range
    s.setOpacity(1.1);      // above range
    s.setOpacity(2.0);      // well above range
    s.setOpacity(qQNaN());  // NaN
    s.setOpacity(qInf());   // infinity

    QCOMPARE(s.opacity(), original);
    QCOMPARE(spy.count(), 0);
}

void TestAbstractSeriesFixture::setOpacityAcceptsValid()
{
    QLineSeries s;
    QSignalSpy spy(&s, &QLineSeries::opacityChanged);
    s.setOpacity(0.5);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.opacity(), 0.5);
    // Boundaries are valid.
    s.setOpacity(0.0);
    QCOMPARE(s.opacity(), 0.0);
    s.setOpacity(1.0);
    QCOMPARE(s.opacity(), 1.0);
}

void TestAbstractSeriesFixture::setOpacityDoesNotEmitOnSameValue()
{
    QLineSeries s;
    s.setOpacity(0.5);
    QSignalSpy spy(&s, &QLineSeries::opacityChanged);
    s.setOpacity(0.5);  // identical — no-op
    QCOMPARE(spy.count(), 0);
    QCOMPARE(s.opacity(), 0.5);
}

// ─────────────────────────────────────────────────────────────
// Setters emit signals only on change
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::setColorEmitsOnceOnRealChange()
{
    TestSeries s;
    QSignalSpy spy(&s, &TestSeries::colorChanged);
    s.setColor(QColor(Qt::red));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.color(), QColor(Qt::red));
}

void TestAbstractSeriesFixture::setColorDoesNotEmitOnSameValue()
{
    TestSeries s;
    s.setColor(QColor(Qt::green));
    QSignalSpy spy(&s, &TestSeries::colorChanged);
    s.setColor(QColor(Qt::green));  // identical
    QCOMPARE(spy.count(), 0);
}

void TestAbstractSeriesFixture::setNameEmitsOnChange()
{
    TestSeries s;
    QSignalSpy spy(&s, &TestSeries::nameChanged);
    s.setName(QStringLiteral("temperature"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.name(), QStringLiteral("temperature"));
}

void TestAbstractSeriesFixture::setVisibleEmitsOnChange()
{
    TestSeries s;
    QVERIFY(s.isVisible());  // default
    QSignalSpy spy(&s, &TestSeries::visibleChanged);
    s.setVisible(false);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!s.isVisible());
}

// ─────────────────────────────────────────────────────────────
// Model ownership
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::setModelUpdatesPointer()
{
    TestSeries s;
    auto* model = new QRingBufferSeriesModel(16);
    QSignalSpy spy(&s, &TestSeries::modelChanged);
    s.setModel(model);
    QCOMPARE(s.model(), model);
    QCOMPARE(spy.count(), 1);

    delete model;  // explicit cleanup; series does NOT take ownership of model
    QVERIFY(s.model() == nullptr);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).value<QAbstractSeriesModel*>(), nullptr);
}

void TestAbstractSeriesFixture::setModelNullClears()
{
    TestSeries s;
    auto* model = new QRingBufferSeriesModel(16);
    s.setModel(model);
    s.setModel(nullptr);
    QVERIFY(s.model() == nullptr);
    delete model;
}

// ─────────────────────────────────────────────────────────────
// Parent / child ownership (RAII — AI.md §1.4)
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::parentDeletesChild()
{
    QPointer<TestSeries> seriesPointer;
    {
        QObject parent;
        auto* series = new TestSeries(&parent);
        seriesPointer = series;
        QCOMPARE(seriesPointer->parent(), &parent);
    }  // parent is destroyed here, deleting series
    QVERIFY(seriesPointer.isNull());
}

// ─────────────────────────────────────────────────────────────
// Copy / move disabled
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::nonCopyable()
{
    // Compile-time check: these lines would fail to compile if the
    // deleted functions were not present. We assert via QVERIFY so the
    // test runner has something to report; the real guard is the
    // compiler.
    QVERIFY(!std::is_copy_constructible<TestSeries>::value);
    QVERIFY(!std::is_copy_assignable<TestSeries>::value);
    QVERIFY(!std::is_move_constructible<TestSeries>::value);
    QVERIFY(!std::is_move_assignable<TestSeries>::value);
}

QTEST_GUILESS_MAIN(TestAbstractSeriesFixture)
#include "TestAbstractSeries.moc"
