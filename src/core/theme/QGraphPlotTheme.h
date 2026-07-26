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

//! @file QGraphPlotTheme.h
//! @brief Shared visual style (colors, line widths, font) for both frontends.

#ifndef QGRAPHPLOT_THEME_H
#define QGRAPHPLOT_THEME_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QColor>

#include "../qgraphplot_global.h"

namespace qgraphplot
{

//! @brief Pure property object describing how a chart looks.
//!
//! The theme lives in the shared core so the QML and Widget frontends read
//! the exact same values (AI.md §3.1 parity) — neither frontend defines its
//! own palette or default line widths.
//!
//! All numeric setters validate their input (AI.md §3.3): non-finite or
//! non-positive widths / font sizes and invalid QColors are rejected with a
//! qWarning instead of being silently stored, because a zero-width pen or an
//! invalid color renders as an invisible chart that is hard to debug.
class QGRAPHPLOT_EXPORT QGraphPlotTheme : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Preset preset READ preset NOTIFY presetChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)
    Q_PROPERTY(
        QColor plotAreaColor READ plotAreaColor WRITE setPlotAreaColor NOTIFY plotAreaColorChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY gridColorChanged)
    Q_PROPERTY(QColor axisColor READ axisColor WRITE setAxisColor NOTIFY axisColorChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(double gridWidth READ gridWidth WRITE setGridWidth NOTIFY gridWidthChanged)
    Q_PROPERTY(double axisWidth READ axisWidth WRITE setAxisWidth NOTIFY axisWidthChanged)
    Q_PROPERTY(double seriesLineWidth READ seriesLineWidth WRITE setSeriesLineWidth NOTIFY
                   seriesLineWidthChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(
        int fontPixelSize READ fontPixelSize WRITE setFontPixelSize NOTIFY fontPixelSizeChanged)
    Q_PROPERTY(QList<QColor> seriesPalette READ seriesPalette WRITE setSeriesPalette NOTIFY
                   seriesPaletteChanged)

public:
    //! Built-in themes. Defined once here so no frontend hardcodes color
    //! literals or theme name strings (AI.md §3.2).
    enum class Preset : unsigned char
    {
        Light,       //!< Default: white canvas, muted gray chrome.
        Dark,        //!< Dark canvas for dashboards / IDE-like hosts.
        Scientific,  //!< High-contrast black chrome, serif labels, thin lines.
    };
    Q_ENUM(Preset)

    //! Constructs a theme initialized to Preset::Light.
    explicit QGraphPlotTheme(QObject* parent = nullptr);
    ~QGraphPlotTheme() override = default;

    QGraphPlotTheme(const QGraphPlotTheme&) = delete;
    QGraphPlotTheme& operator=(const QGraphPlotTheme&) = delete;
    QGraphPlotTheme(QGraphPlotTheme&&) = delete;
    QGraphPlotTheme& operator=(QGraphPlotTheme&&) = delete;

    // ── Presets ──────────────────────────────────────────────

    //! Overwrites every property with @p preset's values.
    Q_INVOKABLE void applyPreset(Preset preset);

    //! The preset most recently passed to applyPreset(). Individual property
    //! setters do NOT change it — it records provenance, not current state.
    [[nodiscard]] Preset preset() const noexcept { return m_preset; }

    // ── Colors ───────────────────────────────────────────────

    [[nodiscard]] QColor backgroundColor() const noexcept { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    [[nodiscard]] QColor plotAreaColor() const noexcept { return m_plotAreaColor; }
    void setPlotAreaColor(const QColor& color);

    [[nodiscard]] QColor gridColor() const noexcept { return m_gridColor; }
    void setGridColor(const QColor& color);

    [[nodiscard]] QColor axisColor() const noexcept { return m_axisColor; }
    void setAxisColor(const QColor& color);

    [[nodiscard]] QColor textColor() const noexcept { return m_textColor; }
    void setTextColor(const QColor& color);

    // ── Line widths (device-independent pixels) ──────────────

    [[nodiscard]] double gridWidth() const noexcept { return m_gridWidth; }
    void setGridWidth(double width);

    [[nodiscard]] double axisWidth() const noexcept { return m_axisWidth; }
    void setAxisWidth(double width);

    //! Default stroke width for series that do not override it.
    [[nodiscard]] double seriesLineWidth() const noexcept { return m_seriesLineWidth; }
    void setSeriesLineWidth(double width);

    // ── Text ─────────────────────────────────────────────────

    //! Family for axis labels and legends. Empty means "application default".
    [[nodiscard]] QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& family);

    [[nodiscard]] int fontPixelSize() const noexcept { return m_fontPixelSize; }
    void setFontPixelSize(int pixelSize);

    // ── Series palette ───────────────────────────────────────

    [[nodiscard]] QList<QColor> seriesPalette() const { return m_seriesPalette; }
    void setSeriesPalette(const QList<QColor>& palette);

    //! Palette color for the @p index'th series, wrapping around the palette.
    //! Negative indices are folded into range, so callers can pass a raw
    //! child index without bounds checking.
    [[nodiscard]] Q_INVOKABLE QColor seriesColor(int index) const;

Q_SIGNALS:
    void presetChanged(Preset preset);
    void backgroundColorChanged(QColor color);
    void plotAreaColorChanged(QColor color);
    void gridColorChanged(QColor color);
    void axisColorChanged(QColor color);
    void textColorChanged(QColor color);
    void gridWidthChanged(double width);
    void axisWidthChanged(double width);
    void seriesLineWidthChanged(double width);
    void fontFamilyChanged(QString family);
    void fontPixelSizeChanged(int pixelSize);
    void seriesPaletteChanged();

    //! Emitted after any property change (including applyPreset()). C++
    //! consumers connect to this single signal instead of all of the above.
    void themeChanged();

private:
    Preset m_preset = Preset::Light;

    QColor m_backgroundColor;
    QColor m_plotAreaColor;
    QColor m_gridColor;
    QColor m_axisColor;
    QColor m_textColor;

    double m_gridWidth = 1.0;
    double m_axisWidth = 1.0;
    double m_seriesLineWidth = 2.0;

    QString m_fontFamily;
    int m_fontPixelSize = 11;

    QList<QColor> m_seriesPalette;
};

}  // namespace qgraphplot

#endif  // QGRAPHPLOT_THEME_H
