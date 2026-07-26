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

//! @file QmlTheme.h
//! @brief Exposes the core qgraphplot::QGraphPlotTheme to QML as `Theme`.

#ifndef QMLTHEME_H
#define QMLTHEME_H

#include <QtQml/QQmlEngine>

#include "theme/QGraphPlotTheme.h"

//! @brief Registration shim only — has no state and is never instantiated.
//!
//! The theme itself lives in the shared core (QGraphPlotCore does not link
//! QtQml, so it cannot carry QML_ELEMENT itself). QML_FOREIGN re-exports the
//! core type under the QGraphPlot QML module, which keeps a single theme
//! implementation for both frontends (AI.md §3.1) instead of a QML-only copy.
//!
//! Usage from QML:
//! @code
//! Theme { id: theme }
//! theme.applyPreset(Theme.Dark)
//! @endcode
struct QmlTheme
{
    Q_GADGET
    QML_FOREIGN(qgraphplot::QGraphPlotTheme)
    QML_NAMED_ELEMENT(Theme)
};

#endif  // QMLTHEME_H
