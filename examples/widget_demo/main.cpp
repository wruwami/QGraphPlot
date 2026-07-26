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

//! @file main.cpp
//! @brief Minimal QWidget demo for the WidgetChartView stub (Phase 0.8).

#include <QtWidgets/QApplication>

#include "WidgetChartView.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    qgraphplot::WidgetChartView chart;
    chart.setWindowTitle("QGraphPlot Widget Demo — Phase 0.8 Stub");
    chart.resize(800, 600);
    chart.show();

    return app.exec();
}
