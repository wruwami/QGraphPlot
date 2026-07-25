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

//! @file qgraphplot_global.h
//! @brief Library-wide macros and configuration for QGraphPlotCore.
//!
//! This header defines the @c QGRAPHPLOT_EXPORT macro used for symbol
//! visibility across shared library boundaries (Windows .dll / Linux .so /
//! macOS .dylib). All public API classes must annotate their declaration
//! with @c QGRAPHPLOT_EXPORT.

#ifndef QGRAPHPLOT_GLOBAL_H
#define QGRAPHPLOT_GLOBAL_H

#include <QtCore/QtGlobal>

// ─────────────────────────────────────────────────────────────
// QGRAPHPLOT_EXPORT: shared library import/export annotation.
// ─────────────────────────────────────────────────────────────
//
// When building QGraphPlotCore itself, the CMake target defines
// QGRAPHPLOT_BUILDING_LIB, which selects Q_DECL_EXPORT (symbols go out).
// When consuming the library as a dependency, the macro expands to
// Q_DECL_IMPORT on Windows and to nothing on platforms where it is
// unnecessary (ELF / Mach-O default to visible symbols).
//
#if defined(QGRAPHPLOT_BUILDING_LIB)
#define QGRAPHPLOT_EXPORT Q_DECL_EXPORT
#else
#define QGRAPHPLOT_EXPORT Q_DECL_IMPORT
#endif

// ─────────────────────────────────────────────────────────────
// QGRAPHPLOT_API_VERSION: short-hand for compile-time API checks.
//
// Encoded as 0xMMmmpp (MAJOR.MINOR.PATCH) using the values configured
// by CMake at build time (VERSIONING.md §3 single-source rule).
// ─────────────────────────────────────────────────────────────
#include "qgraphplot_version.h"

#define QGRAPHPLOT_API_VERSION \
    (QGRAPHPLOT_VERSION_MAJOR << 16 | QGRAPHPLOT_VERSION_MINOR << 8 | QGRAPHPLOT_VERSION_PATCH)

// ─────────────────────────────────────────────────────────────
// Namespace
// ─────────────────────────────────────────────────────────────
//
// Public API lives in the qgraphplot namespace. Frontends (QML/Widget)
// may introduce their own sub-namespaces under qgraphplot::qml and
// qgraphplot::widgets respectively.
namespace qgraphplot
{}  // namespace qgraphplot

#endif  // QGRAPHPLOT_GLOBAL_H
