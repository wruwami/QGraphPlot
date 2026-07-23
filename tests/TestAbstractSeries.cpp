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

#include <QtTest/QtTest>

#include "../src/core/series/QAbstractSeries.h"
#include "../src/core/model/QRingBufferSeriesModel.h"

#include <memory>
#include <type_traits>
#include <QtCore/QPointer>

using namespace qgraphplot;

namespace {

//! Minimal concrete series for testing the abstract property layer.
class TestSeries final : public QAbstractSeries {
    Q_OBJECT
public:
    explicit TestSeries(QObject* parent = nullptr) : QAbstractSeries(parent) {}
    SeriesType type() const override { return SeriesType::Line; }
};

}  // namespace

class TestAbstractSeriesFixture : public QObject {
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

void TestAbstractSeriesFixture::defaultColorIsBlue() {
    TestSeries s;
    QCOMPARE(s.color(), QColor(Qt::blue));
}

void TestAbstractSeriesFixture::defaultVisibleIsTrue() {
    TestSeries s;
    QVERIFY(s.isVisible());
}

void TestAbstractSeriesFixture::defaultModelIsNull() {
    TestSeries s;
    QVERIFY(s.model() == nullptr);
}

void TestAbstractSeriesFixture::defaultNameIsEmpty() {
    TestSeries s;
    QVERIFY(s.name().isEmpty());
}

// ─────────────────────────────────────────────────────────────
// type() dispatch
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::typeReturnsLineForTestSubclass() {
    TestSeries s;
    QCOMPARE(s.type(), SeriesType::Line);
}

// ─────────────────────────────────────────────────────────────
// Setters emit signals only on change
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::setColorEmitsOnceOnRealChange() {
    TestSeries s;
    QSignalSpy spy(&s, &TestSeries::colorChanged);
    s.setColor(QColor(Qt::red));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.color(), QColor(Qt::red));
}

void TestAbstractSeriesFixture::setColorDoesNotEmitOnSameValue() {
    TestSeries s;
    s.setColor(QColor(Qt::green));
    QSignalSpy spy(&s, &TestSeries::colorChanged);
    s.setColor(QColor(Qt::green));  // identical
    QCOMPARE(spy.count(), 0);
}

void TestAbstractSeriesFixture::setNameEmitsOnChange() {
    TestSeries s;
    QSignalSpy spy(&s, &TestSeries::nameChanged);
    s.setName(QStringLiteral("temperature"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.name(), QStringLiteral("temperature"));
}

void TestAbstractSeriesFixture::setVisibleEmitsOnChange() {
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

void TestAbstractSeriesFixture::setModelUpdatesPointer() {
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

void TestAbstractSeriesFixture::setModelNullClears() {
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

void TestAbstractSeriesFixture::parentDeletesChild() {
    QPointer<TestSeries> seriesPointer;
    {
        QObject parent;
        auto* series = new TestSeries(&parent);
        seriesPointer = series;
        QCOMPARE(seriesPointer->parent(), &parent);
    } // parent is destroyed here, deleting series
    QVERIFY(seriesPointer.isNull());
}

// ─────────────────────────────────────────────────────────────
// Copy / move disabled
// ─────────────────────────────────────────────────────────────

void TestAbstractSeriesFixture::nonCopyable() {
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
