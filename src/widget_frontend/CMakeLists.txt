# ═══════════════════════════════════════════════════════════════
# QGraphPlot widget_frontend — QWidget front-end stub
# ═══════════════════════════════════════════════════════════════

set(CMAKE_AUTOMOC ON)

qt_add_library(widget_frontend STATIC
    WidgetChartView.h
    WidgetChartView.cpp
    WidgetLineSeries.h
    WidgetLineSeries.cpp
    WidgetScatterSeries.h
    WidgetScatterSeries.cpp
)

set_target_properties(widget_frontend PROPERTIES
    AUTOMOC ON
    EXPORT_NAME Widget
)

target_link_libraries(widget_frontend
    PUBLIC
        QGraphPlot::Core
        Qt6::Widgets
)

target_include_directories(widget_frontend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/core>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qgraphplot>
)

if(NOT TARGET QGraphPlot::Widget)
    add_library(QGraphPlot::Widget ALIAS widget_frontend)
endif()

install(TARGETS widget_frontend
    EXPORT QGraphPlotTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES WidgetChartView.h WidgetLineSeries.h WidgetScatterSeries.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qgraphplot/widget_frontend
)
