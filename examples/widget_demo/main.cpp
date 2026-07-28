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
//! @brief QWidget demo: Phase 1 rendering with a sine-wave line series (issue #70).

#include <cmath>

#include <QtCore/QList>
#include <QtWidgets/QApplication>

#include "model/QStaticSeriesModel.h"
#include "theme/QGraphPlotTheme.h"
#include "WidgetChartView.h"
#include "WidgetLineSeries.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    qgraphplot::WidgetChartView chart;

    qgraphplot::QGraphPlotTheme theme(&chart);
    theme.applyPreset(qgraphplot::QGraphPlotTheme::Preset::Dark);
    chart.setTheme(&theme);

    // Sine wave: x in [0, 2π], 200 points.
    auto* model = new qgraphplot::QStaticSeriesModel(&chart);
    const int N = 200;
    QList<QPointF> points;
    points.reserve(N);
    for (int i = 0; i < N; ++i) {
        const double x = i * 2.0 * M_PI / (N - 1);
        points.append({x, std::sin(x)});
    }
    model->setPoints(QSpan<const QPointF>(points.constData(), points.size()));

    auto* series = new qgraphplot::WidgetLineSeries(&chart);
    series->setColor(Qt::cyan);
    series->setModel(model);
    chart.addSeries(series);

    chart.setXMin(0.0);
    chart.setXMax(2.0 * M_PI);
    chart.setYMin(-1.2);
    chart.setYMax(1.2);

    chart.setWindowTitle("QGraphPlot Widget Demo — sine wave (Phase 1)");
    chart.resize(800, 600);
    chart.show();

    return app.exec();
}
