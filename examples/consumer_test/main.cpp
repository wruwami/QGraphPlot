#include <iostream>
#include <qgraphplot/qgraphplot_global.h>
#include <qgraphplot/qgraphplot_version.h>
#include <qgraphplot/model/QRingBufferSeriesModel.h>
#include <qgraphplot/series/QLineSeries.h>
#include <qgraphplot/transform/QCoordinateTransform.h>
#include <qgraphplot/widget_frontend/WidgetChartView.h>

int main()
{
    std::cout << "Consumer Test Application for QGraphPlot v" << QGRAPHPLOT_VERSION_STRING << std::endl;

    qgraphplot::QRingBufferSeriesModel model(100);
    qgraphplot::QLineSeries series;
    series.setModel(&model);

    qgraphplot::QCoordinateTransform transform(
        QRectF(0, 0, 10, 10), QRectF(0, 0, 800, 600), false, false);

    std::cout << "Data center (5,5) maps to pixel: ("
              << transform.toPixel(QPointF(5, 5)).x() << ", "
              << transform.toPixel(QPointF(5, 5)).y() << ")" << std::endl;

    return 0;
}
