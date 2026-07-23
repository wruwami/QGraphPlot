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
- Project infrastructure: AI.md, VERSIONING.md, CONTRIBUTING.md, Code of Conduct
- 3-platform CI (Ubuntu / Windows / macOS × Qt 6.7.3)
- cppcheck + clang-format static analysis gates
- Code coverage collection (lcov → Codecov)

### Changed
- _(nothing yet)_

### BREAKING CHANGES
- _(none — API surface not yet public)_

### Fixed
- _(nothing yet)_

---

## [0.1.0] — Unreleased

First pre-1.0 milestone. Establishes the shared C++ core (model / transform /
series interface) and the QML frontend with a real-time 60fps line chart.
Widget frontend is a buildable stub; full rendering lands in Phase 1.

### Added

#### Core library (`src/core/`)
- `QAbstractSeriesModel`: pure data-source interface with range queries and
  fine-grained signals (`pointsInserted`, `pointsRemoved`, `dataChanged`,
  `boundsChanged`, `modelChanged`)
- `QRingBufferSeriesModel`: fixed-capacity FIFO for 60fps streaming with
  `appendBatch`, automatic eviction, optional thread safety (`ThreadSafety`)
- `QCoordinateTransform`: data ↔ pixel transform (linear + log scale)
- `QScaleEngine`: axis tick / label computation (nice-number algorithm)
- `QAbstractSeries`: pure series property interface (model / color / name);
  rendering is frontend-specific, not in core (Interface-First strategy)

#### QML frontend (`src/qml_frontend/`)
- `QmlChartView`: QQuickItem that hosts series and composes the scene graph
- `QmlLineSeries`: QSGGeometryNode-based line renderer with incremental
  geometry updates for real-time streaming
- `QmlAxis` / `QmlLegend`: axis ticks/labels and legend via QSGTextNode

#### Widget frontend (`src/widget_frontend/`)
- `WidgetChartView`: stub QWidget (builds and runs; rendering deferred to
  Phase 1, see `QGraphPlot_MVP_Plan.md`)

#### Examples
- `qml_demo`: line chart with real-time 60fps streaming
- `widget_demo`: minimal stub window

#### Tests
- `TestRingBuffer`, `TestCoordinate`, `TestScaleEngine`

### BREAKING CHANGES
- _(pre-1.0: API not yet declared stable)_

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
