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

//! @file TestTheme.cpp
//! @brief Unit tests for QGraphPlotTheme presets/validation and the shared
//!        series style properties on QAbstractSeries (Phase 0.9).

#include <limits>

#include <QtTest/QtTest>

#include "series/QAbstractSeries.h"
#include "theme/QGraphPlotTheme.h"

using qgraphplot::QAbstractSeries;
using qgraphplot::QGraphPlotTheme;
using qgraphplot::SeriesType;

namespace
{

//! Minimal concrete series — QAbstractSeries is abstract on type() only.
class StyleTestSeries : public QAbstractSeries
{
public:
    using QAbstractSeries::QAbstractSeries;

    [[nodiscard]] SeriesType type() const override { return SeriesType::Line; }
};

}  // namespace

class TestTheme : public QObject
{
    Q_OBJECT

private slots:
    void defaultsToLightPreset();
    void presetsDifferAndAreReported();
    void invalidWidthsAndSizesAreRejected();
    void invalidColorsAndEmptyPaletteAreRejected();
    void seriesColorWrapsAroundPalette();
    void themeChangedFiresOncePerChange();
    void seriesLineWidthIsValidated();
    void seriesDashPatternIsValidated();
};

void TestTheme::defaultsToLightPreset()
{
    const QGraphPlotTheme theme;
    QCOMPARE(theme.preset(), QGraphPlotTheme::Preset::Light);
    QVERIFY(theme.backgroundColor().isValid());
    QVERIFY(theme.gridWidth() > 0.0);
    QVERIFY(!theme.seriesPalette().isEmpty());
}

void TestTheme::presetsDifferAndAreReported()
{
    QGraphPlotTheme theme;
    const QColor lightBackground = theme.backgroundColor();

    QSignalSpy presetSpy(&theme, &QGraphPlotTheme::presetChanged);
    theme.applyPreset(QGraphPlotTheme::Preset::Dark);

    QCOMPARE(presetSpy.count(), 1);
    QCOMPARE(theme.preset(), QGraphPlotTheme::Preset::Dark);
    QVERIFY(theme.backgroundColor() != lightBackground);
    // Dark canvas must be darker than its text, otherwise labels vanish.
    QVERIFY(theme.backgroundColor().lightness() < theme.textColor().lightness());

    theme.applyPreset(QGraphPlotTheme::Preset::Scientific);
    QCOMPARE(theme.preset(), QGraphPlotTheme::Preset::Scientific);
    QCOMPARE(theme.axisColor(), QColor(Qt::black));

    // Re-applying the same preset is a no-op signal-wise.
    presetSpy.clear();
    theme.applyPreset(QGraphPlotTheme::Preset::Scientific);
    QCOMPARE(presetSpy.count(), 0);
}

void TestTheme::invalidWidthsAndSizesAreRejected()
{
    QGraphPlotTheme theme;
    const double gridWidth = theme.gridWidth();
    const int fontPixelSize = theme.fontPixelSize();

    for (const double invalid : {0.0,
                                 -1.0,
                                 std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity()}) {
        QTest::ignoreMessage(QtWarningMsg,
                             "QGraphPlotTheme::setGridWidth: width must be finite and > 0");
        theme.setGridWidth(invalid);
        QCOMPARE(theme.gridWidth(), gridWidth);
    }

    QTest::ignoreMessage(QtWarningMsg,
                         "QGraphPlotTheme::setAxisWidth: width must be finite and > 0");
    theme.setAxisWidth(-2.0);
    QTest::ignoreMessage(QtWarningMsg,
                         "QGraphPlotTheme::setSeriesLineWidth: width must be finite and > 0");
    theme.setSeriesLineWidth(0.0);
    QTest::ignoreMessage(QtWarningMsg, "QGraphPlotTheme::setFontPixelSize: pixelSize must be > 0");
    theme.setFontPixelSize(0);
    QCOMPARE(theme.fontPixelSize(), fontPixelSize);

    theme.setGridWidth(3.5);
    QCOMPARE(theme.gridWidth(), 3.5);
}

void TestTheme::invalidColorsAndEmptyPaletteAreRejected()
{
    QGraphPlotTheme theme;
    const QColor background = theme.backgroundColor();
    const QList<QColor> palette = theme.seriesPalette();

    QTest::ignoreMessage(QtWarningMsg,
                         "QGraphPlotTheme::setBackgroundColor: invalid color ignored");
    theme.setBackgroundColor(QColor());
    QCOMPARE(theme.backgroundColor(), background);

    QTest::ignoreMessage(QtWarningMsg, "QGraphPlotTheme::setSeriesPalette: empty palette ignored");
    theme.setSeriesPalette({});
    QCOMPARE(theme.seriesPalette(), palette);

    QTest::ignoreMessage(QtWarningMsg,
                         "QGraphPlotTheme::setSeriesPalette: palette contains an invalid color");
    theme.setSeriesPalette({QColor(Qt::red), QColor()});
    QCOMPARE(theme.seriesPalette(), palette);

    theme.setSeriesPalette({QColor(Qt::red), QColor(Qt::green)});
    QCOMPARE(theme.seriesPalette().size(), 2);
}

void TestTheme::seriesColorWrapsAroundPalette()
{
    QGraphPlotTheme theme;
    theme.setSeriesPalette({QColor(Qt::red), QColor(Qt::green), QColor(Qt::blue)});

    QCOMPARE(theme.seriesColor(0), QColor(Qt::red));
    QCOMPARE(theme.seriesColor(3), QColor(Qt::red));
    QCOMPARE(theme.seriesColor(4), QColor(Qt::green));
    QCOMPARE(theme.seriesColor(-1), QColor(Qt::blue));
}

void TestTheme::themeChangedFiresOncePerChange()
{
    QGraphPlotTheme theme;
    QSignalSpy changedSpy(&theme, &QGraphPlotTheme::themeChanged);

    theme.setGridColor(QColor(Qt::magenta));
    QCOMPARE(changedSpy.count(), 1);

    // Setting the same value again must not emit.
    theme.setGridColor(QColor(Qt::magenta));
    QCOMPARE(changedSpy.count(), 1);
}

void TestTheme::seriesLineWidthIsValidated()
{
    StyleTestSeries series;
    QCOMPARE(series.lineWidth(), 2.0);

    QSignalSpy widthSpy(&series, &QAbstractSeries::lineWidthChanged);
    series.setLineWidth(4.0);
    QCOMPARE(series.lineWidth(), 4.0);
    QCOMPARE(widthSpy.count(), 1);

    QTest::ignoreMessage(QtWarningMsg,
                         "QAbstractSeries::setLineWidth: lineWidth must be finite and > 0");
    series.setLineWidth(0.0);
    QCOMPARE(series.lineWidth(), 4.0);
    QCOMPARE(widthSpy.count(), 1);
}

void TestTheme::seriesDashPatternIsValidated()
{
    StyleTestSeries series;
    QVERIFY(series.dashPattern().isEmpty());

    QSignalSpy dashSpy(&series, &QAbstractSeries::dashPatternChanged);
    series.setDashPattern({6.0, 4.0});
    QCOMPARE(series.dashPattern(), QList<qreal>({6.0, 4.0}));
    QCOMPARE(dashSpy.count(), 1);

    // Odd entry count → QPen cannot represent it.
    QTest::ignoreMessage(QtWarningMsg,
                         "QAbstractSeries::setDashPattern: pattern must have an even number of "
                         "finite, positive entries");
    series.setDashPattern({6.0, 4.0, 2.0});
    QCOMPARE(series.dashPattern(), QList<qreal>({6.0, 4.0}));

    // Zero-length entries would make dash tessellation spin forever.
    QTest::ignoreMessage(QtWarningMsg,
                         "QAbstractSeries::setDashPattern: pattern must have an even number of "
                         "finite, positive entries");
    series.setDashPattern({6.0, 0.0});
    QCOMPARE(series.dashPattern(), QList<qreal>({6.0, 4.0}));

    QVERIFY(QAbstractSeries::isValidDashPattern({}));
    QVERIFY(!QAbstractSeries::isValidDashPattern({1.0, std::numeric_limits<qreal>::quiet_NaN()}));

    // Back to solid.
    series.setDashPattern({});
    QVERIFY(series.dashPattern().isEmpty());
}

QTEST_MAIN(TestTheme)
#include "TestTheme.moc"
