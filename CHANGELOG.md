# Changelog

All notable changes to QGraphPlot are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
See [`VERSIONING.md`](VERSIONING.md) for the detailed policy.

> **Pre-1.0 note**: While the major version is `0`, backwards-incompatible
> changes may land in MINOR bumps. They will always be listed under
> **BREAKING CHANGES** below.

---

## [Unreleased]

### Added
- `qgraphplot::QLineSeries` — first shipped concrete `QAbstractSeries`
  subclass (`type() == SeriesType::Line`); serves as the core composition
  backend for the frontend line renderers (#57).
- Unit tests covering `QLineSeries` identity/defaults and the previously
  untested `setLineWidth` / `setDashPattern` validation paths.
- `opacity` property on `QAbstractSeries` (double, `[0.0, 1.0]`, default
  `1.0`) — per-series stroke alpha folded into the renderer's color
  channel. Out-of-range / non-finite values are rejected with a
  `qWarning` (#71, #12 leftover).
- QML demo `opacity` cycle button (100% → 75% → 50% → 25% → 100%).
- Unit tests for `setOpacity` validation (reject / accept / no-op-on-same).

### Changed
- `QmlLineSeries` now composes a core `QLineSeries` instead of duplicating
  the property/validation/signal layer. All property state, change-guards
  and validation live in `QAbstractSeries` (single source of truth); QML
  Q_PROPERTY getters/setters delegate to the composed object, and NOTIFY
  signals are forwarded from it. QML property names and behavior are
  preserved (backward compatible) (#57).
- `QmlLineSeries::updatePaintNode` now honors `QAbstractSeries::isVisible()`
  (render skip); previously the core `visible` property had no effect in
  the QML frontend (#57).
- `QmlLineSeries` `opacity` Q_PROPERTY now shadows `QQuickItem::opacity`:
  it controls the series stroke alpha (via the core property) instead of
  item-level compositing, so overlapping series compose predictably. No
  existing QML set `LineSeries.opacity`, so the shadow is safe (#71).
- `qml_frontend` headers/sources renamed from lowercase to CamelCase
  (`qmlchartview.h` → `QmlChartView.h`, etc.) to match core's convention
  (AI.md §1.5, #18 leftover). Include guards updated to the
  `QGRAPHPLOT_*_H` pattern (#71).

### BREAKING CHANGES
- _(none — API surface not yet public)_

### Fixed
- _(nothing yet)_

---

## [0.1.0] — 2026-07-26

First pre-1.0 milestone. Establishes the shared C++ core (model / transform /
series interface), the QML frontend with a real-time 60fps line chart, and a
shared theme system. Widget frontend is a buildable stub with theme-driven
chrome; full data-path rendering lands in Phase 1.

### Added

#### Project infrastructure
- AI.md, VERSIONING.md, CONTRIBUTING.md, Code of Conduct
- 3-platform CI (Ubuntu / Windows / macOS × Qt 6.8.0)
- cppcheck + clang-format static analysis gates
- Code coverage collection (lcov → Codecov)

#### Core library (`src/core/`)
- `QAbstractSeriesModel`: pure data-source interface with range queries and
  fine-grained signals (`pointsInserted`, `pointsRemoved`, `dataChanged`,
  `boundsChanged`, `modelChanged`)
- `QRingBufferSeriesModel`: fixed-capacity FIFO for 60fps streaming with
  `appendBatch`, automatic eviction, optional thread safety (`ThreadSafety`)
- `QCoordinateTransform`: data ↔ pixel transform (linear + log scale)
- `QScaleEngine`: axis tick / label computation (nice-number algorithm)
- `QAbstractSeries`: pure series property interface (model / color / name /
  `lineWidth` / `dashPattern`); rendering is frontend-specific, not in core
  (Interface-First strategy)
- `QGraphPlotTheme`: shared Light / Dark / Scientific presets (background,
  plot-area, grid, axis, text colors; grid/axis/series line widths; font;
  series palette) consumed identically by both frontends (#12)

#### QML frontend (`src/qml_frontend/`)
- `QmlChartView`: QQuickItem that hosts series and composes the scene graph;
  `theme` and read-only `plotArea` properties, chrome painted from the theme
- `QmlLineSeries`: QSGGeometryNode-based line renderer with incremental
  geometry updates for real-time streaming; `lineWidth` and `dashPattern`
  (tessellated `DrawLines` segments, vertex-capped) (#12)
- `QmlAxis`: axis baseline/ticks/grid via QSGGeometryNode (tick labels are
  QML `Text` delegates in `ChartView.qml`), with independent `lineWidth`
  and `gridWidth` (#12)
- `Theme` QML type (`QML_FOREIGN` re-export of `QGraphPlotTheme`) with
  `Light` / `Dark` / `Scientific` presets and a demo theme-toggle button (#12)

#### Widget frontend (`src/widget_frontend/`)
- `WidgetChartView`: stub QWidget (builds and runs); `theme` property paints
  canvas + plot-area chrome from the shared theme. Series/grid/axis data-path
  rendering deferred to Phase 1, see `QGraphPlot_MVP_Plan.md`

#### Examples
- `qml_demo`: line chart with real-time 60fps streaming, theme switching,
  and line-width/dash-pattern toggles
- `widget_demo`: minimal stub window with the Dark theme applied

#### Tests
- `TestRingBuffer`, `TestCoordinate`, `TestScaleEngine`, `TestAbstractSeries`,
  `TestTheme`

### Changed
- Source files renamed so each file matches its class name in CamelCase
  (AI.md §1.5) (#18, #19)
- `QmlLineSeries::updatePaintNode()` reads the model's `points()` span once
  per frame instead of calling the virtual `pointAt()` per point (#36)
- `QmlAxis::updateTicks()` skips `ticksChanged()` (and the QML `Repeater`'s
  full delegate rebuild) when the tick set is unchanged (#36)
- `QRingBufferSeriesModel` bounds tracking replaced with O(1) amortized
  monotonic-deque min/max, removing the O(capacity) rescan on every eviction
  under sustained streaming (#37)

### BREAKING CHANGES
- _(pre-1.0: API not yet declared stable)_

### Fixed
- `ThreadSafety::Enabled` on `QRingBufferSeriesModel` now actually takes a
  mutex on every mutating/read call, as documented (#33)
- Dangling `ChartView` pointer: `QmlLineSeries` / `QmlAxis` tracked their
  parent chart in a shared `static` local instead of per-instance state (#34)
- `gridColor` had no visible effect (axis and grid lines shared one
  geometry node/material); `QmlChartView` setters now share one consistent
  near-zero-safe comparison rule (#35)
- Missing `INT_MAX` guard on `QmlLineSeries`'s node-reuse path; committed
  `.pyc` removed and `__pycache__/` ignored; floating-point drift in the
  streaming demo's timestamp fixed by deriving it from a sample counter (#38)

### Security
- _(nothing yet)_

---

## History before 0.1.0

QGraphPlot was initialized on 2026-07-22 with project scaffolding only
(CMake, CI, docs). There are no tagged releases before `0.1.0`.

---

## Link references

[Unreleased]: https://github.com/wruwami/QGraphPlot/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/wruwami/QGraphPlot/releases/tag/v0.1.0
