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

//! @file QLineSeries.cpp
//! @brief QLineSeries implementation.
//!
//! All property logic (validation, change-guarded signals, model lifecycle)
//! lives in QAbstractSeries; QLineSeries contributes only its SeriesType
//! identity, so there is nothing to do here beyond forwarding construction
//! to the base.

#include "QLineSeries.h"

namespace qgraphplot
{

QLineSeries::QLineSeries(QObject* parent) : QAbstractSeries(parent) {}

}  // namespace qgraphplot
