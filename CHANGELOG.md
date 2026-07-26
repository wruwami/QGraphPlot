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

_(nothing yet)_

---

## [0.1.0] — 2026-07-26

First pre-1.0 milestone (Phase 0 MVP). Establishes the shared C++ core
(model / transform / series / theme) and the QML frontend with a real-time
60fps line chart. The Widget frontend is a buildable stub that already shares
the core theme; full rendering lands in Phase 1.

### Added

#### Core library (`src/core/`)
- `QAbstractSeriesModel`: pure data-source interface with range queries
  (`points()` returning a contiguous `QSpan`) and fine-grained signals
  (`pointsInserted`, `pointsRemoved`, `dataChanged`, `boundsChanged`,
  `modelChanged`) (#4)
- `QRingBufferSeriesModel`: fixed-capacity FIFO for 60fps streaming with
  `appendBatch`, automatic eviction and opt-in thread safety
  (`ThreadSafety`) (#4, #25, #39)
- `QCoordinateTransform`: data ↔ pixel transform (linear + log scale) (#13)
- `QScaleEngine`: axis tick / label computation (nice-number algorithm) (#13)
- `QAbstractSeries`: series property layer (model / color / name / visible /
  `lineWidth` / `dashPattern`); rendering stays frontend-specific rather than
  in core (Interface-First strategy) (#20, #24, #43)
- `QGraphPlotTheme`: property-only theme object with `Light` / `Dark` /
  `Scientific` presets (background, plot area, grid, axis and text colors,
  grid / axis / series widths, font, series palette). Defined in the core so
  the QML and Widget frontends consume identical values (#43)

#### QML frontend (`src/qml_frontend/`)
- `QmlChartView`: `QQuickItem` host that owns the coordinate transform, paints
  canvas / plot area from the theme and exposes `theme` and `plotArea` (#29, #43)
- `QmlLineSeries`: `QSGGeometryNode` line renderer with `color`, `lineWidth`
  and `dashPattern`, driven straight off the model's point span (#29, #42, #43)
- `QmlAxis`: ticks, baseline and grid on separate geometry nodes so
  `color` / `lineWidth` and `gridColor` / `gridWidth` are independent (#29, #41, #43)
- `ChartView.qml`: tick labels styled from the theme (color, pixel size,
  family) (#29, #43)
- `Theme` QML type re-exporting `qgraphplot::QGraphPlotTheme` via
  `QML_FOREIGN`, so the core stays free of a QtQml dependency (#43)

#### Widget frontend (`src/widget_frontend/`)
- `WidgetChartView`: stub `QWidget` that builds, runs and paints canvas /
  plot area from the same `theme` property as QML. Series, grid and axis
  rendering is deferred to Phase 1 (see `QGraphPlot_MVP_Plan.md`) (#32, #43)

#### Examples
- `qml_demo`: line chart with real-time 60fps streaming plus Light / Dark /
  Scientific switching and line-width / dash toggles (#30, #31, #43)
- `widget_demo`: minimal stub window using the Dark preset (#32, #43)

#### Tests
- `TestRingBuffer`, `TestCoordinate`, `TestScaleEngine`,
  `TestAbstractSeries`, `TestTheme`

#### Project infrastructure
- `AI.md`, `VERSIONING.md`, `CONTRIBUTING.md`, Code of Conduct, issue / PR
  templates (#5, #16, #17)
- 3-platform CI (Ubuntu / Windows / macOS × Qt 6.8) (#26, #28)
- cppcheck + clang-format static analysis gates (#17)
- Code coverage collection (lcov → Codecov) (#17)

### Changed
- Source files renamed so each file matches its class name in CamelCase
  (AI.md §1.5) (#19, #22)

### Fixed
- Wrapped reads and `clear()` behavior in `QRingBufferSeriesModel` (#25)
- Lost `wasEmpty` bounds seeding in `QRingBufferSeriesModel` (#23)
- `ThreadSafety::Enabled` locking gaps and a dangling `QmlChartView` pointer
  held by series (#39)
- `INT_MAX` vertex-count guards, float drift and `.gitignore` drift found in
  review (#40)
- `gridColor` being ignored by axis rendering, and `setXMin` / `setYMin`
  behaving inconsistently with the other `QmlChartView` setters (#41)

### Performance
- Render path iterates the model's contiguous point span instead of calling
  the virtual `pointAt()` per point, and skips no-op tick signal emissions (#42)
- O(1) amortized min/max bounds tracking in `QRingBufferSeriesModel` via
  monotonic deques (#44)

### BREAKING CHANGES
- _(pre-1.0: API not yet declared stable)_

### Security
- _(nothing)_

---

## History before 0.1.0

QGraphPlot was initialized on 2026-07-22 with project scaffolding only
(CMake, CI, docs). There are no tagged releases before `0.1.0`.

---

## Link references

[Unreleased]: https://github.com/wruwami/QGraphPlot/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/wruwami/QGraphPlot/releases/tag/v0.1.0
