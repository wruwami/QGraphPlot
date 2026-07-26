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
- _(nothing yet)_

### Changed
- _(nothing yet)_

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
- 3-platform CI (Ubuntu / Windows / macOS × Qt 6.7.3)
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
- `QmlAxis`: axis ticks/labels/grid via QSGGeometryNode, with independent
  `lineWidth` and `gridWidth` (#12)
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
