# QGraphPlot

[![CI](https://github.com/wruwami/QGraphPlot/actions/workflows/ci.yml/badge.svg)](https://github.com/wruwami/QGraphPlot/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/wruwami/QGraphPlot/branch/main/graph/badge.svg)](https://codecov.io/gh/wruwami/QGraphPlot)
![CodeRabbit Pull Request Reviews](https://img.shields.io/coderabbit/prs/github/wruwami/QGraphPlot?utm_source=oss&utm_medium=github&utm_campaign=wruwami%2FQGraphPlot&labelColor=171717&color=FF570A&link=https%3A%2F%2Fcoderabbit.ai&label=CodeRabbit+Reviews)

> **Notice:** This is a temporary personal project, developed with the
> assistance of AI. Feel free to try it out, but it's still early-stage
> and evolving quickly, so it's not really at the point of being a
> finished "product" yet.

A high-performance chart plotting library for Qt, licensed under Apache-2.0. It supports both QWidget and QML (Qt Quick) applications from the same C++ core (QGraphPlotCore), uses an Interface-First strategy, and is designed for 60fps real-time streaming data using a lock-free/mutex-controlled ring buffer.

This is an independent, clean-room implementation — see [`NOTICE`](NOTICE) (if available) for details on what that means and how this project relates to other chart plotting libraries.

## Features

- **Dual UI**: QQuickItem-based `QmlChartView` and QWidget-based `WidgetChartView` both consume the same core data models and coordinate transform logic.
- **High-Performance Models**: `QRingBufferSeriesModel` provides O(1) allocation-free data appending and streaming up to 60fps with optional thread safety.
- **Interface-First Architecture**: Core business logic (coordinate transform, axis ticks, models) is completely decoupled from rendering views.
- **Nice Axis Ticks**: Paul Heckbert's nice graph label algorithm computes optimal ticks and formats labels dynamically to prevent floating-point noise.
- **Logarithmic Scales**: Built-in support for log scale coordinate mapping and log tick calculations.

## Status

The core library, series models (`QAbstractSeriesModel`, `QRingBufferSeriesModel`), coordinate transform engine (`QCoordinateTransform`), tick generation engine (`QScaleEngine`), and unit test suites are fully implemented. Next steps include the abstract series interface and front-end rendering modules (QML and QWidget).

## Using the C++ Core / QWidget view

The core library exposes a C++ API for managing data feeds, calculating layout grids, and transforming coordinates. Here is how the core model and transform layers are wired up:

```cpp
#include "model/QRingBufferSeriesModel.h"
#include "transform/QCoordinateTransform.h"
#include "transform/QScaleEngine.h"

// 1. Create a 60fps ring buffer model with a capacity of 1000 points
auto* model = new qgraphplot::QRingBufferSeriesModel(1000, qgraphplot::ThreadSafety::Enabled);
model->append(QPointF(1.0, 10.0));

// 2. Fetch data bounds and map to screen pixels (Y-axis is automatically inverted)
QRectF dataBounds = model->bounds();
QRectF pixelRect(0, 0, 800, 600);
qgraphplot::QCoordinateTransform trans(dataBounds, pixelRect, false, false);
QPointF pixelPt = trans.toPixel(QPointF(1.0, 10.0));

// 3. Calculate nice ticks for the axis
auto ticks = qgraphplot::QScaleEngine::calculateTicks(
    dataBounds.left(), dataBounds.right(), 5, false
);
```

## Using the QML view

The QML frontend registers C++ core models and exposes elements like `ChartView` and `LineSeries`. A simple QML integration looks like this:

```qml
import QtQuick
import QGraphPlot

Item {
    width: 800
    height: 600

    ChartView {
        anchors.fill: parent

        LineSeries {
            id: lineSeries
            color: "cyan"
            // consumes a QRingBufferSeriesModel registered to QML
        }
    }
}
```

## Real-time 60fps Streaming

To support ultra-fast 60fps streaming updates:
- **Batch Append**: New points should be added via `appendBatch(QSpan<const QPointF>)` rather than individual `append()` calls. This optimizes memory copy operations and minimizes signal overhead.
- **Thread Safety**: By default, `QRingBufferSeriesModel` operates with `ThreadSafety::Disabled` (fastest) which is safe when the GUI thread blocks during QSG render updates. If the data producer pushes data from a separate thread concurrently, configure the model with `ThreadSafety::Enabled` to protect state using internal mutexes.

## Data Model API

Data sources are fully decoupled from rendering logic by implementing the `QAbstractSeriesModel` interface.
- **`pointCount()`**: Returns the number of points currently held by the model.
- **`pointAt(index)`**: Returns the point at the logical index (in oldest-first order).
- **`points(first, last)`**: Returns a contiguous `QSpan<const QPointF>` view of the data.
- **`bounds()`**: Returns the bounding box (`QRectF`) in data space, which is automatically cached and recomputed incrementally to support fast auto-scaling.

## Examples

Demonstration examples are located in `examples/qml_demo` and `examples/widget_demo` (to be fully realized in subsequent phases).

## Testing & CI

Build and test commands:
```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --build-config Release --output-on-failure
```
- `.github/workflows/ci.yml` runs a matrix on Linux, Windows, and macOS with Qt 6.7.3.
- Code coverage is captured on Linux/Qt6 via `lcov` and uploaded to Codecov.
- Static analysis and formatting checks:
```bash
# formatting
find src tests examples -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror
# static analysis
cppcheck @cppcheck.options src
```
- Formatting rules in `.clang-format`. Cppcheck suppressions in `.cppcheck-suppressions`.

## Repository layout

- `src/core/`: Core shared library target `QGraphPlotCore`.
  - `model/`: Data models (`QAbstractSeriesModel`, `QRingBufferSeriesModel`).
  - `transform/`: Viewport transformations (`QCoordinateTransform`) and tick generators (`QScaleEngine`).
- `src/qml_frontend/`: QML Quick Item rendering modules.
- `src/widget_frontend/`: QWidget front-end modules.
- `tests/`: Unit test suite.
- `examples/`: Minimal example projects.

## Versioning

Follows Semantic Versioning. Details in `VERSIONING.md` and release history in `CHANGELOG.md` (if available).

## License

Apache-2.0. Details in `LICENSE` and `NOTICE` (if available).
