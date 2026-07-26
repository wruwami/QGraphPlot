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

//! @file QGraphPlotTheme.cpp
//! @brief Preset definitions and validating setters for QGraphPlotTheme.

#include "QGraphPlotTheme.h"

#include <array>
#include <cmath>

#include <QtCore/QtGlobal>

namespace qgraphplot
{

namespace
{

//! Widths and font sizes must be strictly positive and finite; a 0-width or
//! NaN pen silently renders nothing (AI.md §3.3).
bool isPositiveFinite(double value)
{
    return std::isfinite(value) && value > 0.0;
}

//! The concrete values behind each Preset. Bundled in one struct so adding a
//! preset means adding one table entry, not touching every setter.
struct PresetValues
{
    QRgb background;
    QRgb plotArea;
    QRgb grid;
    QRgb axis;
    QRgb text;
    double gridWidth;
    double axisWidth;
    double seriesLineWidth;
    const char* fontFamily;
    int fontPixelSize;
    std::array<QRgb, 5> palette;
};

PresetValues valuesFor(QGraphPlotTheme::Preset preset)
{
    switch (preset) {
    case QGraphPlotTheme::Preset::Dark:
        return {0xFF1E1E1E,
                0xFF252526,
                0xFF3A3A3A,
                0xFFC8C8C8,
                0xFFD4D4D4,
                1.0,
                1.0,
                2.0,
                "",
                11,
                {0xFF4EC9B0, 0xFF569CD6, 0xFFDCDCAA, 0xFFCE9178, 0xFFC586C0}};
    case QGraphPlotTheme::Preset::Scientific:
        return {0xFFFFFFFF,
                0xFFFFFFFF,
                0xFFB0B0B0,
                0xFF000000,
                0xFF000000,
                0.75,
                1.5,
                1.5,
                "Serif",
                12,
                {0xFF000000, 0xFFD62728, 0xFF1F77B4, 0xFF2CA02C, 0xFF9467BD}};
    case QGraphPlotTheme::Preset::Light:
        break;
    }
    return {0xFFFFFFFF,
            0xFFFFFFFF,
            0xFFDEE2E6,
            0xFF495057,
            0xFF495057,
            1.0,
            1.0,
            2.0,
            "",
            11,
            {0xFF007ACC, 0xFFE8590C, 0xFF2F9E44, 0xFFC2255C, 0xFF6741D9}};
}

}  // namespace

QGraphPlotTheme::QGraphPlotTheme(QObject* parent) : QObject(parent)
{
    applyPreset(Preset::Light);
}

void QGraphPlotTheme::applyPreset(Preset preset)
{
    const PresetValues values = valuesFor(preset);

    setBackgroundColor(QColor::fromRgba(values.background));
    setPlotAreaColor(QColor::fromRgba(values.plotArea));
    setGridColor(QColor::fromRgba(values.grid));
    setAxisColor(QColor::fromRgba(values.axis));
    setTextColor(QColor::fromRgba(values.text));

    setGridWidth(values.gridWidth);
    setAxisWidth(values.axisWidth);
    setSeriesLineWidth(values.seriesLineWidth);

    setFontFamily(QString::fromLatin1(values.fontFamily));
    setFontPixelSize(values.fontPixelSize);

    QList<QColor> palette;
    palette.reserve(static_cast<qsizetype>(values.palette.size()));
    for (const QRgb rgba : values.palette) {
        palette.append(QColor::fromRgba(rgba));
    }
    setSeriesPalette(palette);

    if (m_preset != preset) {
        m_preset = preset;
        emit presetChanged(m_preset);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setBackgroundColor(const QColor& color)
{
    if (!color.isValid()) {
        qWarning("QGraphPlotTheme::setBackgroundColor: invalid color ignored");
        return;
    }
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        emit backgroundColorChanged(m_backgroundColor);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setPlotAreaColor(const QColor& color)
{
    if (!color.isValid()) {
        qWarning("QGraphPlotTheme::setPlotAreaColor: invalid color ignored");
        return;
    }
    if (m_plotAreaColor != color) {
        m_plotAreaColor = color;
        emit plotAreaColorChanged(m_plotAreaColor);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setGridColor(const QColor& color)
{
    if (!color.isValid()) {
        qWarning("QGraphPlotTheme::setGridColor: invalid color ignored");
        return;
    }
    if (m_gridColor != color) {
        m_gridColor = color;
        emit gridColorChanged(m_gridColor);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setAxisColor(const QColor& color)
{
    if (!color.isValid()) {
        qWarning("QGraphPlotTheme::setAxisColor: invalid color ignored");
        return;
    }
    if (m_axisColor != color) {
        m_axisColor = color;
        emit axisColorChanged(m_axisColor);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setTextColor(const QColor& color)
{
    if (!color.isValid()) {
        qWarning("QGraphPlotTheme::setTextColor: invalid color ignored");
        return;
    }
    if (m_textColor != color) {
        m_textColor = color;
        emit textColorChanged(m_textColor);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setGridWidth(double width)
{
    if (!isPositiveFinite(width)) {
        qWarning("QGraphPlotTheme::setGridWidth: width must be finite and > 0");
        return;
    }
    if (!qFuzzyCompare(m_gridWidth, width)) {
        m_gridWidth = width;
        emit gridWidthChanged(m_gridWidth);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setAxisWidth(double width)
{
    if (!isPositiveFinite(width)) {
        qWarning("QGraphPlotTheme::setAxisWidth: width must be finite and > 0");
        return;
    }
    if (!qFuzzyCompare(m_axisWidth, width)) {
        m_axisWidth = width;
        emit axisWidthChanged(m_axisWidth);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setSeriesLineWidth(double width)
{
    if (!isPositiveFinite(width)) {
        qWarning("QGraphPlotTheme::setSeriesLineWidth: width must be finite and > 0");
        return;
    }
    if (!qFuzzyCompare(m_seriesLineWidth, width)) {
        m_seriesLineWidth = width;
        emit seriesLineWidthChanged(m_seriesLineWidth);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setFontFamily(const QString& family)
{
    if (m_fontFamily != family) {
        m_fontFamily = family;
        emit fontFamilyChanged(m_fontFamily);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setFontPixelSize(int pixelSize)
{
    if (pixelSize <= 0) {
        qWarning("QGraphPlotTheme::setFontPixelSize: pixelSize must be > 0");
        return;
    }
    if (m_fontPixelSize != pixelSize) {
        m_fontPixelSize = pixelSize;
        emit fontPixelSizeChanged(m_fontPixelSize);
        emit themeChanged();
    }
}

void QGraphPlotTheme::setSeriesPalette(const QList<QColor>& palette)
{
    if (palette.isEmpty()) {
        qWarning("QGraphPlotTheme::setSeriesPalette: empty palette ignored");
        return;
    }
    for (const QColor& color : palette) {
        if (!color.isValid()) {
            qWarning("QGraphPlotTheme::setSeriesPalette: palette contains an invalid color");
            return;
        }
    }
    if (m_seriesPalette != palette) {
        m_seriesPalette = palette;
        emit seriesPaletteChanged();
        emit themeChanged();
    }
}

QColor QGraphPlotTheme::seriesColor(int index) const
{
    if (m_seriesPalette.isEmpty()) {
        return m_axisColor;
    }
    const qsizetype size = m_seriesPalette.size();
    const qsizetype wrapped = ((static_cast<qsizetype>(index) % size) + size) % size;
    return m_seriesPalette.at(wrapped);
}

}  // namespace qgraphplot
